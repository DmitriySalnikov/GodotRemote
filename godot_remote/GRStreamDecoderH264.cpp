/* GRStreamDecoderH264.cpp */

#if !defined(NO_GODOTREMOTE_CLIENT) && defined(GODOT_REMOTE_H264_ENABLED)

//#define DEBUG_H264

#include "GRStreamDecoderH264.h"
#include "GRClient.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY
#include "core/os/thread_safe.h"
#else

using namespace godot;
#endif
using namespace GRUtils;

///////////////////////////////////////////////////////////////////////////////
// IMAGE SEQUENCE DECODER
///////////////////////////////////////////////////////////////////////////////

#ifndef GDNATIVE_LIBRARY

void GRStreamDecoderH264::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamDecoderH264::_processing_thread);
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_thread), "user_data"), &GRStreamDecoderH264::_update_thread);
}

#else

void GRStreamDecoderH264::_register_methods() {
	METHOD_REG(GRStreamDecoderH264, _processing_thread);
	METHOD_REG(GRStreamDecoderH264, _update_thread);
}

#endif

void GRStreamDecoderH264::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			GRStreamDecoder::_deinit();
			break;
	}
}

void GRStreamDecoderH264::push_packet_to_decode(std::shared_ptr<GRPacketStreamData> packet) {
	ZoneScopedNC("Push packet to decode", tracy::Color::Ivory);
	Scoped_lock(ts_lock);
	GRStreamDecoder::push_packet_to_decode(packet);

	shared_cast_def(GRPacketStreamDataH264, img_data, packet);
	if (img_data && packet->get_type() == GRPacket::StreamDataH264) {
		if (img_data->get_is_stream_end()) {
			images.pop();
			_commit_stream_end();
		}
	} else {
		_log("Got wrong packet data", LogLevel::LL_ERROR);
	}
}

int GRStreamDecoderH264::get_max_queued_frames() {
	return 32;
}

// TODO 0 - auto mode. for next version mb
void GRStreamDecoderH264::start_decoder_threads(int count) {
	if (count >= 0) {
		if (count > GR_STREAM_DECODER_H264_MAX_THREADS) {
			_log("threads count " + str(count) + " > " + str(GR_STREAM_DECODER_H264_MAX_THREADS) + ". Clamping.", LogLevel::LL_ERROR);
			count = GR_STREAM_DECODER_H264_MAX_THREADS;
		}

		Scoped_lock(ts_lock);
		int prop_val = count == 1 ? 0 : count;
		bool upd = threads_number != count;

		if (!is_thread_active) {
			is_thread_active = true;
			threads_number = prop_val;
			Thread_start(thread, this, _processing_thread, Variant());

			is_update_thread_active = true;
			Thread_start(update_thread, this, _update_thread, Variant());
		} else {
			if (upd) {
				threads_number = prop_val;
			}
		}
	} else {
		stop_decoder_threads();
	}
}

void GRStreamDecoderH264::stop_decoder_threads() {
	ZoneScopedNC("Stop Decoder Threads", tracy::Color::Firebrick3);
	is_thread_active = false;
	Thread_close(thread);

	is_update_thread_active = false;
	Thread_close(update_thread);

	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();
}

void GRStreamDecoderH264::_init() {
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRStreamDecoder::_init();
#endif
}

void GRStreamDecoderH264::_deinit() {
	LEAVE_IF_EDITOR();
	stop_decoder_threads();
}

void GRStreamDecoderH264::_commit_stream_end() {
	Scoped_lock(ts_lock);
	while (buffer.size())
		buffer.pop();

	auto img = BufferedImage(PoolByteArray(), 0, 0, 0, 0);
	img.is_end = true;

	buffer.push(img);
}

void GRStreamDecoderH264::_update_thread(Variant p_userdata) {
	Thread_set_name("H264 Update Thread");
	auto os = OS::get_singleton();
	auto wait_next_frame = [&]() {
		ts_lock.lock();
		uint64_t ft = buffer.size() ? buffer.front().frametime : 0;
		ts_lock.unlock();
		_sleep_waiting_next_frame(ft);
	};
	bool is_lost_image_sended = false;

	while (is_update_thread_active) {
		ZoneScopedNC("Update Image Sequence", tracy::Color::DeepSkyBlue1);
		ts_lock.lock();

		if (buffer.size() != 0) {
			auto buf = buffer.front();
			buffer.pop();

			uint64_t time = get_time_usec();
			if (buf.is_end) {
				gr_client->_image_lost();
				is_lost_image_sended = true;
			} else {
				ts_lock.unlock();
				is_lost_image_sended = false;
				prev_shown_frame_time = time;

				if (buf.img_data.size() != 0) {
					int64_t delay = _calc_delay(time, buf.frame_added_time, buf.frametime);
					if (delay < 0) {
						delay = 0;
					}

					gr_client->_display_new_image(buf.img_data, buf.img_width, buf.img_height, delay);
				} else {
					gr_client->_image_lost();
					is_lost_image_sended = true;
				}

				wait_next_frame();
				continue;
			}
		}
		ts_lock.unlock();

		// check if image displayed less then few seconds ago. if not then remove texture
		if ((get_time_usec() - prev_shown_frame_time > 1000_ms * image_loss_time) && !is_lost_image_sended) {
			gr_client->_image_lost();
			is_lost_image_sended = true;
		}

		wait_next_frame();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// H264 Decoder
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void GRStreamDecoderH264::FlushFrames(ISVCDecoder *h264_decoder, uint64_t start_time, uint64_t frametime) {
	ZoneScopedNC("Flush Frames", tracy::Color::Blue1);
	uint8_t *pData[3] = { NULL }; // WTF IS THIS??
	uint8_t remaining_frames = 0;
	SBufferInfo sDstBufInfo;

	h264_decoder->GetOption(DECODER_OPTION::DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER, &remaining_frames);
	while (remaining_frames > 0) {
		ZoneScopedNC("Flush Frame", tracy::Color::CadetBlue1);
		remaining_frames--;

		pData[0] = NULL;
		pData[1] = NULL;
		pData[2] = NULL;
		memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
		sDstBufInfo.uiInBsTimeStamp = get_time_usec();
		h264_decoder->FlushFrame(pData, &sDstBufInfo);

		ProcessFrame(&sDstBufInfo, start_time, frametime);
	}
}

bool GRStreamDecoderH264::ProcessFrame(SBufferInfo *info, uint64_t start_time, uint64_t frametime) {
	if (info->iBufferStatus == 1) {
		ZoneScopedNC("Process Frame", tracy::Color::Orange3);
		PoolByteArray data;

		Error e = GRUtilsH264Codec::_decode_yuv_to_image(&data, info->UsrData.sSystemBuffer.iWidth, info->UsrData.sSystemBuffer.iHeight, info->pDst[0], info->pDst[1], info->pDst[2], info->UsrData.sSystemBuffer.iStride);
		if (e != Error::OK) {
			_log("Can't decode image from YUV", LogLevel::LL_ERROR);
			return false;
		}

		Scoped_lock(ts_lock);
		while (buffer.size() > get_max_queued_frames())
			buffer.pop();

		if ((buffer.size() && buffer.front().is_end) || data.size() == 0) {
			return false;
		}

		auto buf_img = BufferedImage(data, info->UsrData.sSystemBuffer.iWidth, info->UsrData.sSystemBuffer.iHeight, start_time, frametime);
		buffer.push(buf_img);
	}
	return true;
}

void GRStreamDecoderH264::_processing_thread(Variant p_userdata) {
	Thread_set_name("Stream Decoder: H264");
	OS *os = OS::get_singleton();

	ISVCDecoder *h264_decoder = nullptr;
	SDecodingParam sDecParam;
	bool is_decoder_inited = false;
	SBufferInfo sDstBufInfo;
	int32_t iThreadCountInit = 0;

#ifndef DEBUG_ENABLED
	int32_t iTraceLevel = WELS_LOG_DEFAULT;
#else
	int32_t iTraceLevel = WELS_LOG_ERROR;
#endif

	int32_t iThreadCount = 1;
	uint8_t uLastSpsBuf[32];
	int32_t iLastSpsByteCount = 0;
	int32_t iSliceSize;
	uint8_t *pData[3] = { NULL }; // WTF IS THIS??
	bool is_decode_with_error = false;

#ifdef DEBUG_H264
	auto d = memnew(_Directory);
	d->remove("stream.264");
	auto file_open(f, "stream.264", _File::ModeFlags::WRITE);
#endif

	GRUtilsH264Codec::_decoder_create(&h264_decoder);

	sDecParam = { 0 };
	sDecParam.sVideoProperty.size = sizeof(sDecParam.sVideoProperty);
	sDecParam.uiTargetDqLayer = (uint8_t)-1;
	sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
	sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

	while (is_thread_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		Error err = Error::OK;
		shared_cast_def(GRPacketStreamDataH264, image_pack, images.front());
		images.pop();
		ts_lock.unlock();

		if (h264_decoder) {
			if (!is_decoder_inited) {
				is_decoder_inited = true;
				h264_decoder->SetOption(DECODER_OPTION_NUM_OF_THREADS, &iThreadCountInit);
				h264_decoder->SetOption(DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

				int err = h264_decoder->Initialize(&sDecParam);
				if (err != 0) {
					_log("Failed to initialize decoder. Code: " + str(err), LogLevel::LL_ERROR);
					is_decoder_inited = false;
					continue;
				}
				h264_decoder->GetOption(DECODER_OPTION_NUM_OF_THREADS, &iThreadCount);
			}

			if (is_decoder_inited) {
				//                                       \/ this needed for static_cast
				if (image_pack && image_pack->get_type() == GRPacket::StreamDataH264) {
					if (image_pack->get_is_stream_end()) {
						ZoneScopedNC("Stream End", tracy::Color::Firebrick1);
#ifdef DEBUG_H264
						f->close();
						memdelete(f);
						d->remove("stream.264");
						file_open(f, "stream.264", _File::ModeFlags::WRITE);
#endif
						is_decode_with_error = true;
						continue;
					}

					if (is_decode_with_error && image_pack->get_frame_type() != EVideoFrameType::videoFrameTypeIDR) {
						gr_client->_update_avg_fps(0);
						// wait next IDR
						continue;
					}
					is_decode_with_error = false;

					ZoneScopedNC("Image Decode", tracy::Color::Aquamarine4);
					std::vector<PoolByteArray> img_layers = image_pack->get_image_data();
					for (auto buf : img_layers) {
						iSliceSize = buf.size();
						if (iSliceSize < 4) { //too small size, no effective data, ignore
							continue;
						}

						if (iThreadCount >= 1) {
							uint8_t *uSpsPtr = NULL;
							int32_t iSpsByteCount = 0;
							if (iLastSpsByteCount > 0 && iSpsByteCount > 0) {
								if (iSpsByteCount != iLastSpsByteCount || memcmp(uSpsPtr, uLastSpsBuf, iLastSpsByteCount) != 0) {
									//whenever new sequence is different from preceding sequence. All pending frames must be flushed out before the new sequence can start to decode.
									FlushFrames(h264_decoder, 0, image_pack->get_frametime());
								}
							}
							if (iSpsByteCount > 0 && uSpsPtr != NULL) {
								if (iSpsByteCount > 32) iSpsByteCount = 32;
								iLastSpsByteCount = iSpsByteCount;
								memcpy(uLastSpsBuf, uSpsPtr, iSpsByteCount);
							}
						}

						auto wb = buf.read();
#ifdef DEBUG_H264
						f->store_buffer(put_data_from_array_pointer(buf));
#endif
						pData[0] = NULL;
						pData[1] = NULL;
						pData[2] = NULL;
						memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
						sDstBufInfo.uiInBsTimeStamp = image_pack->get_start_time();
						int err = h264_decoder->DecodeFrameNoDelay(wb.ptr(), buf.size(), pData, &sDstBufInfo);
						if (err != 0) {
							_log("OpenH264 Decode error. Code: " + str(err) + ". Waiting for the next IDR frame.", LogLevel::LL_ERROR);
							is_decode_with_error = true;
						}

						ProcessFrame(&sDstBufInfo, image_pack->get_start_time(), image_pack->get_frametime());
					}
				} else {
					_log("Wrong image package for this stream", LogLevel::LL_DEBUG);
				}
			}
		} else {
			sleep_usec(25_ms);
			// just wait thread closing...
		}
	}

	if (h264_decoder) {
		h264_decoder->Uninitialize();
		GRUtilsH264Codec::_decoder_free(h264_decoder);
	}
	h264_decoder = nullptr;

#ifdef DEBUG_H264
	f->close();
	memdelete(f);
	memdelete(d);
#endif
}

#endif // NO_GODOTREMOTE_CLIENT

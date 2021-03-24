/* GRStreamDecoderH264.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRStreamDecoderH264.h"
#include "GRClient.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRUtilsJPGCodec.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY

#include "core/os/os.h"
#include "core/os/thread_safe.h"
#include "scene/main/node.h"

#else

#include <Node.hpp>
#include <OS.hpp>

using namespace godot;
#endif
using namespace GRUtils;

///////////////////////////////////////////////////////////////////////////////
// IMAGE SEQUENCE DECODER
///////////////////////////////////////////////////////////////////////////////

#ifndef GDNATIVE_LIBRARY

void GRStreamDecoderH264::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamDecoderH264::_processing_thread);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamDecoderH264::_register_methods() {
	METHOD_REG(GRStreamDecoderH264, _processing_thread);
	//register_property<GRStreamDecoders, String>("password", &GRStreamDecoders::set_password, &GRStreamDecoders::get_password, "");
	//register_signal<GRStreamDecoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
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

void GRStreamDecoderH264::push_packet_to_decode(std::shared_ptr<GRPacket> packet) {
	ZoneScopedNC("Push packet to decode", tracy::Color::Ivory);
	GRStreamDecoder::push_packet_to_decode(packet);

	shared_cast_def(GRPacketImageData, img_data, packet);
	if (img_data) {
		if (img_data->get_is_stream_end()) {
			Scoped_lock(ts_lock);
			images.pop();

			while (buffer.size())
				buffer.pop();

			auto img = BufferedImage(0);
			img.is_end = true;

			buffer.push(img);
		}
	}
}

// TODO add a way to calculate delay of stream. based on sync time mb

void GRStreamDecoderH264::update() {
	ZoneScopedNC("Update Image Sequence", tracy::Color::DeepSkyBlue1);
	Scoped_lock(ts_lock);
	auto os = OS::get_singleton();

	if (buffer.size()) {
		auto buf = buffer.front();

		uint64_t time = os->get_ticks_usec();
		if (buf.is_end) {
			gr_client->_image_lost();
		} else {
			buffer.pop();

			prev_shown_frame_time = time;

			if (buf.img.is_valid() && !img_is_empty(buf.img)) {
				gr_client->_display_new_image(buf.img, buf.frame_send_time - abs(int64_t(gr_client->sync_time_server) - int64_t(gr_client->sync_time_client)));
			} else {
				gr_client->_image_lost();
			}
			return;
		}
	}

	// check if image displayed less then few seconds ago. if not then remove texture
	if (os->get_ticks_usec() > int64_t(prev_shown_frame_time + uint64_t(1000_ms * image_loss_time))) {
		gr_client->_image_lost();
	}
}

int GRStreamDecoderH264::get_max_queued_frames() {
	return 8;
}

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
	while (buffer.size())
		buffer.pop();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// H264 Encoder
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void GRStreamDecoderH264::FlushFrames(ISVCDecoder *h264_decoder, int64_t &iTotal, uint64_t &uiTimeStamp, int32_t &iWidth, int32_t &iHeight, uint64_t start_time) {
	uint8_t *pData[3] = { NULL }; // WTF IS THIS??
	uint8_t remaining_frames = 0;
	SBufferInfo sDstBufInfo;

	h264_decoder->GetOption(DECODER_OPTION::DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER, &remaining_frames);
	while (remaining_frames > 0) {
		remaining_frames--;

		Ref<Image> img = newref(Image);

		pData[0] = NULL;
		pData[1] = NULL;
		pData[2] = NULL;
		memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
		sDstBufInfo.uiInBsTimeStamp = OS::get_singleton()->get_ticks_usec();
		h264_decoder->FlushFrame(pData, &sDstBufInfo);

		if (sDstBufInfo.iBufferStatus == 1) {
			Error e = GRUtilsJPGCodec::_decode_yuv_to_image(img, sDstBufInfo.UsrData.sSystemBuffer.iWidth, sDstBufInfo.UsrData.sSystemBuffer.iHeight, sDstBufInfo.pDst[0], sDstBufInfo.pDst[1], sDstBufInfo.pDst[2]);
			if (e != Error::OK) {
				_log("Can't decode image from YUV", LogLevel::LL_ERROR);
				continue;
			}
		}

		Scoped_lock(ts_lock);
		while (buffer.size() > get_max_queued_frames())
			buffer.pop();

		if (buffer.size() && buffer.front().is_end && img_is_empty(img)) {
			continue;
		}

		auto buf_img = BufferedImage(start_time);
		buffer.push(buf_img);
		buf_img.img = img;
	}
}

void GRStreamDecoderH264::_processing_thread(Variant p_userdata) {
	Thread_set_name("Stream Decoder: H264");
	OS *os = OS::get_singleton();
	GRUtilsJPGCodec::yuv_buffer_data_to_decoder yuv_buffer;

	ISVCDecoder *h264_decoder = nullptr;
	SDecodingParam sDecParam;
	bool is_decoder_inited = false;
	SBufferInfo sDstBufInfo;
	int32_t iThreadCount = 1;
	uint8_t uLastSpsBuf[32];
	int32_t iLastSpsByteCount = 0;
	int32_t iSliceSize;
	// TODO check if encoder created

	GRUtilsH264Codec::_decoder_create(&h264_decoder);
	memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
	h264_decoder->GetOption(DECODER_OPTION_NUM_OF_THREADS, &iThreadCount);

	sDecParam = { 0 };
	sDecParam.sVideoProperty.size = sizeof(sDecParam.sVideoProperty);
	sDecParam.uiTargetDqLayer = (uint8_t)-1;
	sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
	sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

	is_decoder_inited = false;

	// TODO init in loop?
	int err = h264_decoder->Initialize(&sDecParam);
	if (err != 0) {
		_log("Failed to initialize decoder. Code: " + str(err), LogLevel::LL_ERROR);
	}

	while (is_thread_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		Error err = Error::OK;
		shared_cast_def(GRPacketH264, image_pack, images.front());
		images.pop();

		if (image_pack) {
			ZoneScopedNC("Image Decode", tracy::Color::Aquamarine4);
			ts_lock.unlock();

			/*
			if (iThreadCount >= 1) {
				uint8_t *uSpsPtr = NULL;
				int32_t iSpsByteCount = 0;
				iSliceSize = readPicture(pBuf, iFileSize, iBufPos, uSpsPtr, iSpsByteCount);
				if (iLastSpsByteCount > 0 && iSpsByteCount > 0) {
					if (iSpsByteCount != iLastSpsByteCount || memcmp(uSpsPtr, uLastSpsBuf, iLastSpsByteCount) != 0) {
						//whenever new sequence is different from preceding sequence. All pending frames must be flushed out before the new sequence can start to decode.
						FlushFrames(pDecoder, iTotal, pYuvFile, pOptionFile, iFrameCount, uiTimeStamp, iWidth, iHeight, iLastWidth,
								iLastHeight);
					}
				}
				if (iSpsByteCount > 0 && uSpsPtr != NULL) {
					if (iSpsByteCount > 32) iSpsByteCount = 32;
					iLastSpsByteCount = iSpsByteCount;
					memcpy(uLastSpsBuf, uSpsPtr, iSpsByteCount);
				}
			} else {
				int i = 0;
				for (i = 0; i < iFileSize; i++) {
					if ((pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0 && pBuf[iBufPos + i + 3] == 1 && i > 0) || (pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 1 && i > 0)) {
						break;
					}
				}
				iSliceSize = i;
			}

			if (iSliceSize < 4) { //too small size, no effective data, ignore
				iBufPos += iSliceSize;
				continue;
			}
			*/

			if (h264_decoder && is_decoder_inited) {

				uint8_t *buf = image_pack->get_image_data();
				uint64_t iSize = image_pack->get_image_data_size();
				_log("H264 Decoder buffered image size: " + str(iSize), LogLevel::LL_NORMAL);

				yuv_buffer.free_mem();
				int err = h264_decoder->DecodeFrameNoDelay(buf, iSize, yuv_buffer.buf, &sDstBufInfo);
				if (err != 0) {
					_log("OpenH264 Decode error. Code: " + str(err), LogLevel::LL_ERROR);
				}
			}
		} else {
			ts_lock.unlock();
			_log("Wrong image package for this stream", LogLevel::LL_DEBUG);
		}
	}

	h264_decoder->Uninitialize();
	GRUtilsH264Codec::_decoder_free(h264_decoder);
	h264_decoder = nullptr;
}

#endif // NO_GODOTREMOTE_CLIENT

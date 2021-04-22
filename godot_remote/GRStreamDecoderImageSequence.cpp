/* GRStreamDecoderImageSequence.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRStreamDecoderImageSequence.h"
#include "GRClient.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRUtils.h"
#include "GRUtilsJPGCodec.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY
#include "core/os/thread_safe.h"
#include "scene/main/node.h"
#else
#include <Node.hpp>
using namespace godot;
#endif

using namespace GRUtils;

///////////////////////////////////////////////////////////////////////////////
// IMAGE SEQUENCE DECODER
///////////////////////////////////////////////////////////////////////////////

#ifndef GDNATIVE_LIBRARY

void GRStreamDecoderImageSequence::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamDecoderImageSequence::_processing_thread);
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_thread), "user_data"), &GRStreamDecoderImageSequence::_update_thread);
}

#else

void GRStreamDecoderImageSequence::_register_methods() {
	METHOD_REG(GRStreamDecoderImageSequence, _processing_thread);
	METHOD_REG(GRStreamDecoderImageSequence, _update_thread);
}

#endif

void GRStreamDecoderImageSequence::_notification(int p_notification) {
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

void GRStreamDecoderImageSequence::push_packet_to_decode(std::shared_ptr<GRPacketStreamData> packet) {
	ZoneScopedNC("Push packet to decode", tracy::Color::Ivory);
	Scoped_lock(ts_lock);
	GRStreamDecoder::push_packet_to_decode(packet);

	shared_cast_def(GRPacketStreamDataImage, img_data, packet);
	if (img_data && packet->get_type() == GRPacket::StreamDataImage) {
		if (img_data->get_is_stream_end()) {
			images.pop();
			_commit_stream_end();
		}
	}
}

int GRStreamDecoderImageSequence::get_max_queued_frames() {
	Scoped_lock(ts_lock);
	return (int)threads.size() * 2;
}

// TODO 0 - auto mode. for next version mb
void GRStreamDecoderImageSequence::start_decoder_threads(int count) {
	if (threads.size() == count)
		return;

	stop_decoder_threads();
	is_threads_active = true;
	ZoneScopedNC("Start Decoder Threads", tracy::Color::Firebrick);

	if (count < 0) {
		_log("threads count < 0. Disabling decoder.", LogLevel::LL_DEBUG);
		return;
	}

	if (count > GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS) {
		_log("threads count " + str(count) + " > " + str(GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS) + ". Clamping.", LogLevel::LL_ERROR);
		count = GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS;
	}

	threads.resize(count);
	for (int i = 0; i < count; i++) {
		Thread_start(threads[i], this, _processing_thread, i);
	}
	if (count > 0) {
		is_update_thread_active = true;
		Thread_start(update_thread, this, _update_thread, Variant());
	}
}

void GRStreamDecoderImageSequence::stop_decoder_threads() {
	ZoneScopedNC("Stop Decoder Threads", tracy::Color::Firebrick3);
	is_threads_active = false;
	for (int i = 0; i < threads.size(); i++) {
		Thread_close(threads[i]);
	}
	is_update_thread_active = false;
	Thread_close(update_thread);

	threads.resize(0);

	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();
}

void GRStreamDecoderImageSequence::_init() {
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRStreamDecoder::_init();
#endif
}

void GRStreamDecoderImageSequence::_deinit() {
	LEAVE_IF_EDITOR();
	stop_decoder_threads();
}

void GRStreamDecoderImageSequence::_commit_stream_end() {
	Scoped_lock(ts_lock);
	while (buffer.size())
		buffer.pop();

	auto img = std::make_shared<BufferedImage>(0, 0);
	img->is_ready = true;
	img->is_end = true;

	buffer.push(img);
}

void GRStreamDecoderImageSequence::_update_thread(Variant p_userdata) {
	Thread_set_name("Image Sequence Update Thread");
	auto wait_next_frame = [&]() {
		ts_lock.lock();
		if (buffer.size() && !buffer.front()->is_end) {
			uint64_t ft = buffer.front()->frametime;
			ts_lock.unlock();
			_sleep_waiting_next_frame(ft);
		} else {
			ts_lock.unlock();
			sleep_usec(1_ms);
		}
	};
	bool is_lost_image_sended = false;

	while (is_update_thread_active) {
		//ZoneScopedNC("Update Image Sequence", tracy::Color::DeepSkyBlue1);
		ts_lock.lock();

		if (buffer.size() != 0) {
			int count = 0;
			for (auto d : buffer) {
				if (d->is_ready)
					count++;
				else
					break;
			}

			while (count > 1) {
				buffer.pop();
				count--;
			}

			auto buf = buffer.front();

			if (buf->is_ready) {
				uint64_t time = get_time_usec();
				uint64_t next_frame = prev_shown_frame_time + buf->frametime;
				if (buf->is_end) {
					TracyMessage("Buffered image marked as end of stream.", 40);
					buffer.pop();
					gr_client->_image_lost();
					is_lost_image_sended = true;
				} else {
					if (time >= next_frame) {
						buffer.pop();
						ts_lock.unlock();
						is_lost_image_sended = false;

						prev_shown_frame_time = time;

						if (buf->img_data.size() != 0) {
							int64_t delay = _calc_delay(time, buf->frame_added_time, buf->frametime);
							if (delay < 0) {
								delay = 0;
							}

							gr_client->_display_new_image(buf->img_data, buf->img_width, buf->img_height, delay);
						} else {
							TracyMessage("Image is broken. Ending stream..", 33);
							gr_client->_image_lost();
							is_lost_image_sended = true;
						}
						wait_next_frame();
						continue;
					} else {
						ts_lock.unlock();
						wait_next_frame();
						continue;
					}
				}
			} else {
				//ZoneScopedN("Not Ready");
			}
		}
		ts_lock.unlock();

		// check if image displayed less then few seconds ago. if not then remove texture
		if ((get_time_usec() - prev_shown_frame_time > 1000_ms * image_loss_time) && !is_lost_image_sended) {
			TracyMessage("Image is lost.", 15);
			gr_client->_image_lost();
			is_lost_image_sended = true;
			continue;
		}

		wait_next_frame();
	}
}

void GRStreamDecoderImageSequence::_processing_thread(Variant p_userdata) {
	int thread_idx = p_userdata;
	Thread_set_name(("Stream Decoder: Image Sequence " + str(thread_idx)).ascii().get_data());

	// jpg_buffer will be auto expanded if needed
	PoolByteArray jpg_buffer;
	jpg_buffer.resize((1024 * 1024) * 4);

#ifndef GODOT_REMOTE_LIBJPEG_TURBO_ENABLED
	Ref<Image> tmp_img = newref(Image);
#endif

	while (is_threads_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		Error err = Error::OK;
		shared_cast_def(GRPacketStreamDataImage, image_pack, images.front());
		images.pop();

		if (image_pack->get_is_stream_end()) {
			_commit_stream_end();
			continue;
		}

		//                                       \/ this needed for static_cast
		if (image_pack && image_pack->get_type() == GRPacket::StreamDataImage) {
			ZoneScopedNC("Image Decode", tracy::Color::Aquamarine4);

			while (buffer.size() > get_max_queued_frames())
				buffer.pop();

			PoolByteArray img_data = image_pack->get_image_data();
			PoolByteArray out_img_data;
			int out_width = 0;
			int out_height = 0;

			if ((buffer.size() && buffer.front()->is_end) || img_data.size() == 0) {
				ts_lock.unlock();
				continue;
			}

			auto buf_img = std::make_shared<BufferedImage>(image_pack->get_frametime(), image_pack->get_start_time());
			buffer.push(buf_img);

			ts_lock.unlock();

			GRDevice::ImageCompressionType type = (GRDevice::ImageCompressionType)image_pack->get_compression_type();

			switch (type) {
				case GRDevice::ImageCompressionType::COMPRESSION_JPG: {
#ifdef GODOT_REMOTE_LIBJPEG_TURBO_ENABLED
					{
						ZoneScopedN("Decompress: JPG Turbo");
						err = GRUtilsJPGCodec::_decompress_jpg_turbo(img_data, jpg_buffer, &out_img_data, &out_width, &out_height);
					}
#else
					{
						ZoneScopedN("Decompress: JPG Godot Internal");
						err = tmp_img->load_jpg_from_buffer(img_data);
						if (err == Error::OK) {
							out_img_data = tmp_img->get_data();
							out_width = tmp_img->get_width();
							out_height = tmp_img->get_height();
						}
					}
#endif

					if ((int)err || out_img_data.size() == 0) { // is NOT OK
						_log("Can't decode JPG image.", LogLevel::LL_ERROR);
						GRNotifications::add_notification("Stream Error", "Can't decode JPG image. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
					}
					break;
				}
				default:
					_log("Not implemented image decoder type: " + str((int)type), LogLevel::LL_ERROR);
					break;
			}

			buf_img->img_data = out_img_data;
			buf_img->img_width = out_width;
			buf_img->img_height = out_height;

			buf_img->is_ready = true;
		} else {
			ts_lock.unlock();
			_log("Wrong image package for this stream", LogLevel::LL_DEBUG);
		}
	}
}

#endif // NO_GODOTREMOTE_CLIENT

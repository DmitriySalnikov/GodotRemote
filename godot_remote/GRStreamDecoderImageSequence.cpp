/* GRStreamDecoderImageSequence.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRStreamDecoderImageSequence.h"
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

void GRStreamDecoderImageSequence::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamDecoderImageSequence::_processing_thread);
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_thread), "user_data"), &GRStreamDecoderImageSequence::_update_thread);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamDecoderImageSequence::_register_methods() {
	METHOD_REG(GRStreamDecoderImageSequence, _processing_thread);
	METHOD_REG(GRStreamDecoderImageSequence, _update_thread);
	//register_property<GRStreamDecoders, String>("password", &GRStreamDecoders::set_password, &GRStreamDecoders::get_password, "");
	//register_signal<GRStreamDecoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
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
	GRStreamDecoder::push_packet_to_decode(packet);

	shared_cast_def(GRPacketStreamDataImage, img_data, packet);
	if (img_data && packet->get_type() == GRPacket::StreamDataImage) {
		if (img_data->get_is_stream_end()) {
			Scoped_lock(ts_lock);
			images.pop();

			while (buffer.size())
				buffer.pop();

			auto img = std::make_shared<BufferedImage>(0, 0);
			img->is_ready = true;
			img->is_end = true;

			buffer.push(img);
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

// TODO add a way to calculate delay of stream. based on sync time mb
void GRStreamDecoderImageSequence::_update_thread(Variant p_userdata) {
	Thread_set_name("Image Sequence Update Thread");
	while (is_update_thread_active) {
		ZoneScopedNC("Update Image Sequence", tracy::Color::DeepSkyBlue1);
		ts_lock.lock();

		if (buffer.size()) {
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
				// TODO check for correctness. it looks like the fps changes in a wave
				uint64_t next_frame = prev_shown_frame_time + buf->frametime - 1_ms;
				if (buf->is_end) {
					gr_client->_image_lost();
				} else if (time > next_frame) {
					buffer.pop();

					prev_shown_frame_time = time;

					if (buf->img.is_valid() && !img_is_empty(buf->img)) {
						// TODO does not work
						gr_client->_display_new_image(buf->img, buf->frame_send_time - abs(int64_t(gr_client->sync_time_server) - int64_t(gr_client->sync_time_client)));
					} else {
						gr_client->_image_lost();
					}
					ts_lock.unlock();
					sleep_usec(1_ms);
					continue;
				}
			}
		}
		ts_lock.unlock();
		sleep_usec(1_ms);

		// check if image displayed less then few seconds ago. if not then remove texture
		if (get_time_usec() > int64_t(prev_shown_frame_time + uint64_t(1000_ms * image_loss_time))) {
			gr_client->_image_lost();
		}
	}
}

void GRStreamDecoderImageSequence::_processing_thread(Variant p_userdata) {
	int thread_idx = p_userdata;
	Thread_set_name(("Stream Decoder: Image Sequence " + str(thread_idx)).ascii().get_data());

	PoolByteArray jpg_buffer;
	jpg_buffer.resize((1024 * 1024) * 4);

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

		//                                       \/ this needed for static_cast
		if (image_pack && image_pack->get_type() == GRPacket::StreamDataImage) {
			ZoneScopedNC("Image Decode", tracy::Color::Aquamarine4);

			while (buffer.size() > get_max_queued_frames())
				buffer.pop();

			PoolByteArray img_data = image_pack->get_image_data();

			if (buffer.size() && buffer.front()->is_end && !img_data.size()) {
				ts_lock.unlock();
				continue;
			}

			auto buf_img = std::make_shared<BufferedImage>(image_pack->get_frametime(), image_pack->get_start_time());
			buffer.push(buf_img);

			ts_lock.unlock();

			Ref<Image> img = newref(Image);
			GRDevice::ImageCompressionType type = (GRDevice::ImageCompressionType)image_pack->get_compression_type();

			switch (type) {
				case GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED: {
#ifndef GDNATIVE_LIBRARY
					img->create((int)image_pack->get_size().x, (int)image_pack->get_size().y, false, (Image::Format)image_pack->get_format(), image_pack->get_image_data());
#else
					img->create_from_data((int)image_pack->get_size().x, (int)image_pack->get_size().y, false, (Image::Format)image_pack->get_format(), image_pack->get_image_data());
#endif
					if (img_is_empty(img)) { // is NOT OK
						err = Error::FAILED;
						_log("Incorrect uncompressed image data.", LogLevel::LL_ERROR);
						GRNotifications::add_notification("Stream Error", "Incorrect uncompressed image data.", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
					}
					break;
				}
				case GRDevice::ImageCompressionType::COMPRESSION_JPG: {
#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED
					err = GRUtilsJPGCodec::_decompress_jpg_turbo(img_data, jpg_buffer, &img);
#else
					err = img->load_jpg_from_buffer(img_data);
#endif

					if ((int)err || img_is_empty(img)) { // is NOT OK
						_log("Can't decode JPG image.", LogLevel::LL_ERROR);
						GRNotifications::add_notification("Stream Error", "Can't decode JPG image. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
					}
					break;
				}
				default:
					_log("Not implemented image decoder type: " + str((int)type), LogLevel::LL_ERROR);
					break;
			}

			buf_img->img = img;
			buf_img->is_ready = true;
		} else {
			ts_lock.unlock();
			_log("Wrong image package for this stream", LogLevel::LL_DEBUG);
		}
	}
}

#endif // NO_GODOTREMOTE_CLIENT

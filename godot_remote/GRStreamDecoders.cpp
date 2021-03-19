/* GRStreamDecoders.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRStreamDecoders.h"
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

#ifndef GDNATIVE_LIBRARY

///////////////////////////////////////////////////////////////////////////////
// MANAGER

void GRStreamDecodersManager::_bind_methods() {
	//ClassDB::bind_method(D_METHOD("_load_settings"), &GRStreamDecoders::_load_settings);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamDecodersManager::_register_methods() {
	METHOD_REG(GRStreamDecodersManager, _notification);
	//METHOD_REG(GRStreamDecoders, _notification);
	//register_property<GRStreamDecoders, String>("password", &GRStreamDecoders::set_password, &GRStreamDecoders::get_password, "");
	//register_signal<GRStreamDecoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
}

#endif

void GRStreamDecodersManager::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
	}
}

void GRStreamDecodersManager::_start_encoder(Ref<GRPacket> packet) {
	Ref<GRPacketImageData> img_data = packet;
	if (img_data.is_valid()) {
		GRDevice::ImageCompressionType comp = (GRDevice::ImageCompressionType)img_data->get_compression_type();
		active = true;

		switch (comp) {
			case GRDevice::COMPRESSION_UNCOMPRESSED:
			case GRDevice::COMPRESSION_JPG:
			case GRDevice::COMPRESSION_PNG: {
				GRStreamDecoderImageSequence *tmp_en = decoder ? cast_to<GRStreamDecoderImageSequence>(decoder) : nullptr;
				if (tmp_en) {
					tmp_en->start_decoder_threads(threads_count);
				} else {
					if (decoder) {
						memdelete(decoder);
					}

					tmp_en = memnew(GRStreamDecoderImageSequence);
					decoder = tmp_en;
					decoder->set_gr_client(gr_client);
					decoder->start_decoder_threads(threads_count);
				}
				break;
			}
			default: {
				active = false;
				if (decoder) {
					memdelete(decoder);
				}
				break;
			}
		}
	} else {
		_log("Got invalid Image data from server.", LogLevel::LL_ERROR);
	}
}

void GRStreamDecodersManager::update() {
	Scoped_lock(ts_lock);
	if (decoder) {
		return decoder->update();
	}
}

void GRStreamDecodersManager::set_gr_client(GRClient *client) {
	gr_client = client;
}

void GRStreamDecodersManager::push_packet_to_decode(Ref<GRPacket> packet) {
	Scoped_lock(ts_lock);
	_start_encoder(packet);
	if (decoder) {
		return decoder->push_packet_to_decode(packet);
	}
}

void GRStreamDecodersManager::set_threads_count(int count) {
	ERR_FAIL_COND(count < 1 || count > GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS);
	Scoped_lock(ts_lock);
	if (threads_count != count) {
		threads_count = count;
		if (decoder && active)
			decoder->start_decoder_threads(count);
	}
}

int GRStreamDecodersManager::get_threads_count() {
	return threads_count;
}

void GRStreamDecodersManager::set_active(bool state) {
	if (active != state) {
		active = state;
		if (active) {
			if (decoder)
				decoder->start_decoder_threads(threads_count);
		} else {
			if (decoder)
				decoder->start_decoder_threads(0);
		}
	}
}

void GRStreamDecodersManager::_init() {
	LEAVE_IF_EDITOR();
}

void GRStreamDecodersManager::_deinit() {
	LEAVE_IF_EDITOR();
	Scoped_lock(ts_lock);
	if (decoder)
		memdelete(decoder);
}

///////////////////////////////////////////////////////////////////////////////
// BASE ENCODER CLASS

#ifndef GDNATIVE_LIBRARY

void GRStreamDecoder::_bind_methods() {
	//ClassDB::bind_method(D_METHOD("_load_settings"), &GRStreamDecoders::_load_settings);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamDecoder::_register_methods() {
	METHOD_REG(GRStreamDecoder, _notification);
	//METHOD_REG(GRStreamDecoders, _notification);
	//register_property<GRStreamDecoders, String>("password", &GRStreamDecoders::set_password, &GRStreamDecoders::get_password, "");
	//register_signal<GRStreamDecoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
}

#endif

void GRStreamDecoder::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
	}
}

void GRStreamDecoder::set_gr_client(GRClient *client) {
	gr_client = client;
}

void GRStreamDecoder::push_packet_to_decode(Ref<GRPacket> packet) {
	if (packet.is_valid()) {
		Scoped_lock(ts_lock);
		while (images.size() > get_max_queued_frames()) {
			images.pop();
			TracyMessage("Removed Queued Packet", 22);
		}

		images.push(packet);
	} else {
		_log("Pushed empty packet to StreamDecoder. Ignoring.", LogLevel::LL_ERROR);
	}
}

void GRStreamDecoder::_init() {
	LEAVE_IF_EDITOR();
}

void GRStreamDecoder::_deinit() {
	LEAVE_IF_EDITOR();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// IMAGE SEQUENCE ENCODER
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef GDNATIVE_LIBRARY

void GRStreamDecoderImageSequence::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamDecoderImageSequence::_processing_thread);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamDecoderImageSequence::_register_methods() {
	METHOD_REG(GRStreamDecoderImageSequence, _processing_thread);
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

void GRStreamDecoderImageSequence::push_packet_to_decode(Ref<GRPacket> packet) {
	ZoneScopedNC("Push packet to decode", tracy::Color::Ivory);
	GRStreamDecoder::push_packet_to_decode(packet);

	Ref<GRPacketImageData> img_data = packet;
	if (img_data.is_valid()) {
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

// TODO add a way to calculate delay of stream. based on sync time mb

void GRStreamDecoderImageSequence::update() {
	ZoneScopedNC("Update Image Sequence", tracy::Color::DeepSkyBlue1);
	Scoped_lock(ts_lock);
	auto os = OS::get_singleton();

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
			uint64_t time = os->get_ticks_usec();
			// TODO check for correctness. it looks like the fps changes in a wave
			uint64_t next_frame = prev_shown_frame_time + buf->frametime - 1_ms;
			if (buf->is_end) {
				gr_client->_image_lost();
			} else if (time > next_frame) {
				buffer.pop();

				prev_shown_frame_time = time;

				if (buf->img.is_valid() && !buf->img->empty()) {
					// TODO does not work
					gr_client->_display_new_image(buf->img, buf->frame_send_time - abs(int64_t(gr_client->sync_time_server) - int64_t(gr_client->sync_time_client)));
				} else {
					gr_client->_image_lost();
				}
				return;
			}
		}
	}

	// check if image displayed less then few seconds ago. if not then remove texture
	if (os->get_ticks_usec() > int64_t(prev_shown_frame_time + uint64_t(1000_ms * image_loss_time))) {
		gr_client->_image_lost();
	}
}

int GRStreamDecoderImageSequence::get_max_queued_frames() {
	Scoped_lock(ts_lock);
	return (int)threads.size() * 2;
}

void GRStreamDecoderImageSequence::start_decoder_threads(int count) {
	if (threads.size() == count)
		return;

	stop_decoder_threads();
	is_threads_active = true;
	ZoneScopedNC("Start Decoder Threads", tracy::Color::Firebrick);

	if (count < 1) {
		_log("threads count < 1. Disabling decoder.", LogLevel::LL_DEBUG);
		return;
	}

	if (count > GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS) {
		_log("threads count " + str(count) + " > 16. Clamping.", LogLevel::LL_ERROR);
		count = GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS;
	}

	threads.resize(count);
	for (int i = 0; i < count; i++) {
		Thread_start(threads[i], this, _processing_thread, i);
	}
}

void GRStreamDecoderImageSequence::stop_decoder_threads() {
	ZoneScopedNC("Stop Decoder Threads", tracy::Color::Firebrick3);
	is_threads_active = false;
	for (int i = 0; i < threads.size(); i++) {
		Thread_close(threads[i]);
	}
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
	while (buffer.size())
		buffer.pop();
}

void GRStreamDecoderImageSequence::_processing_thread(Variant p_userdata) {
	int thread_idx = p_userdata;
	OS *os = OS::get_singleton();

	PoolByteArray jpg_buffer;
	jpg_buffer.resize((1024 * 1024) * 4);

	Thread_set_name(("Stream Decoder: Image Sequence " + str(thread_idx)).ascii().get_data());

	while (is_threads_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		Error err = Error::OK;
		Ref<GRPacketImageData> image_pack = images.front();
		images.pop();

		if (image_pack.is_valid()) {
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

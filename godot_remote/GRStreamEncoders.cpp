/* GRStreamEncoders.cpp */

#ifndef NO_GODOTREMOTE_SERVER

#include "GRStreamEncoders.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRServer.h"
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

void GRStreamEncodersManager::_bind_methods() {
	//ClassDB::bind_method(D_METHOD("_load_settings"), &GRStreamEncoders::_load_settings);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamEncodersManager::_register_methods() {
	METHOD_REG(GRStreamEncodersManager, _notification);
	//METHOD_REG(GRStreamEncoders, _notification);
	//register_property<GRStreamEncoders, String>("password", &GRStreamEncoders::set_password, &GRStreamEncoders::get_password, "");
	//register_signal<GRStreamEncoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
}

#endif

void GRStreamEncodersManager::_notification(int p_notification) {
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

void GRStreamEncodersManager::start(int compression, GRSViewport *vp) {
	Scoped_lock(ts_lock);
	if (vp && encoder && viewport && viewport != vp) {
		encoder->set_viewport(vp);
		viewport = vp;
	}
	active = true;

	GRDevice::ImageCompressionType comp = (GRDevice::ImageCompressionType)compression;
	switch (comp) {
		case GRDevice::COMPRESSION_UNCOMPRESSED:
		case GRDevice::COMPRESSION_JPG: {
			if (encoder) {
				memdelete(encoder);
			}

			encoder = memnew(GRStreamEncoderImageSequence);
			encoder->set_compression_type(compression);
			encoder->set_viewport(vp);
			viewport = vp;

			encoder->start_encoder_threads(threads_count);

			break;
		}
		case GRDevice::COMPRESSION_H264: {
			if (encoder) {
				memdelete(encoder);
			}

			encoder = memnew(GRStreamEncoderH264);
			encoder->set_compression_type(compression);
			encoder->set_viewport(vp);
			viewport = vp;
			break;
		}
		default: {
			active = false;
			if (encoder) {
				memdelete(encoder);
				encoder = nullptr;
			}
			break;
		}
	}
}

void GRStreamEncodersManager::set_compression_type(int compression, GRSViewport *vp) {
	Scoped_lock(ts_lock);
	GRDevice::ImageCompressionType comp = (GRDevice::ImageCompressionType)compression;
	switch (comp) {
		case GRDevice::COMPRESSION_UNCOMPRESSED:
		case GRDevice::COMPRESSION_JPG: {
			GRStreamEncoderImageSequence *tmp_en = encoder ? cast_to<GRStreamEncoderImageSequence>(encoder) : nullptr;
			if (tmp_en) {
				tmp_en->set_compression_type(compression);
			} else {
				start(compression, vp);
			}
			break;
		}
		case GRDevice::COMPRESSION_H264: {
			GRStreamEncoderH264 *tmp_en = encoder ? cast_to<GRStreamEncoderH264>(encoder) : nullptr;
			if (!tmp_en) {
				start(compression, vp);
			}
			break;
		}
	}
}

void GRStreamEncodersManager::commit_image(Ref<Image> img, uint64_t frametime) {
	Scoped_lock(ts_lock);
	if (encoder) {
		encoder->commit_image(img, frametime);
	}
}

void GRStreamEncodersManager::commit_stream_end() {
	Scoped_lock(ts_lock);
	if (encoder) {
		encoder->commit_stream_end();
	}
}

bool GRStreamEncodersManager::has_data_to_send() {
	Scoped_lock(ts_lock);
	if (encoder) {
		return encoder->has_data_to_send();
	}
	return false;
}

Ref<GRPacket> GRStreamEncodersManager::pop_data_to_send() {
	Scoped_lock(ts_lock);
	if (encoder) {
		return encoder->pop_data_to_send();
	}
	return Ref<GRPacket>();
}

void GRStreamEncodersManager::set_threads_count(int count) {
	ERR_FAIL_COND(count < 1 || count > GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS);
	Scoped_lock(ts_lock);
	if (threads_count != count) {
		threads_count = count;
		if (encoder && active)
			encoder->start_encoder_threads(count);
	}
}

int GRStreamEncodersManager::get_threads_count() {
	return threads_count;
}

void GRStreamEncodersManager::set_active(bool state) {
	if (active != state) {
		active = state;
		if (active) {
			if (encoder)
				encoder->start_encoder_threads(threads_count);
		} else {
			if (encoder)
				encoder->start_encoder_threads(0);
		}
	}
}

void GRStreamEncodersManager::_init() {
	LEAVE_IF_EDITOR();
	threads_count = GET_PS(GodotRemote::ps_server_image_encoder_threads_count_name);
}

void GRStreamEncodersManager::_deinit() {
	LEAVE_IF_EDITOR();
	Scoped_lock(ts_lock);
	if (encoder) {
		memdelete(encoder);
		encoder = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
// BASE ENCODER CLASS

#ifndef GDNATIVE_LIBRARY

void GRStreamEncoder::_bind_methods() {
	//ClassDB::bind_method(D_METHOD("_load_settings"), &GRStreamEncoders::_load_settings);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamEncoder::_register_methods() {
	METHOD_REG(GRStreamEncoder, _notification);
	//METHOD_REG(GRStreamEncoders, _notification);
	//register_property<GRStreamEncoders, String>("password", &GRStreamEncoders::set_password, &GRStreamEncoders::get_password, "");
	//register_signal<GRStreamEncoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
}

#endif

void GRStreamEncoder::_notification(int p_notification) {
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

void GRStreamEncoder::commit_image(Ref<Image> img, uint64_t frametime) {
	if (!img_is_empty(img)) {
		Scoped_lock(ts_lock);
		while (images.size() > get_max_queued_frames()) {
			images.pop();
			TracyMessage("Removed Queued Image", 21);
		}

		if (img->get_format() == Image::FORMAT_RGBA8 || img->get_format() == Image::FORMAT_RGB8) {
			images.push(CommitedImage(img, OS::get_singleton()->get_ticks_usec(), frametime));
		} else {
			_log("Committed image to StreamEncoder has a wrong format. Format of image: " + str(img->get_format()), LogLevel::LL_ERROR);
		}
	} else {
		_log("Committed empty image to StreamEncoder. Ignoring.", LogLevel::LL_ERROR);
	}
}

void GRStreamEncoder::_init() {
	LEAVE_IF_EDITOR();
}

void GRStreamEncoder::_deinit() {
	LEAVE_IF_EDITOR();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// IMAGE SEQUENCE ENCODER
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef GDNATIVE_LIBRARY

void GRStreamEncoderImageSequence::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamEncoderImageSequence::_processing_thread);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamEncoderImageSequence::_register_methods() {
	METHOD_REG(GRStreamEncoderImageSequence, _processing_thread);
	//register_property<GRStreamEncoders, String>("password", &GRStreamEncoders::set_password, &GRStreamEncoders::get_password, "");
	//register_signal<GRStreamEncoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
}

#endif

void GRStreamEncoderImageSequence::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			GRStreamEncoder::_deinit();
			break;
	}
}

void GRStreamEncoderImageSequence::commit_stream_end() {
	Scoped_lock(ts_lock);
	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();

	auto b = std::make_shared<BufferedImage>();
	auto p = newref(GRPacketImageData);
	p->set_is_stream_end(true);

	b->is_ready = true;
	b->pack = p;
	buffer.push(b);
}

bool GRStreamEncoderImageSequence::has_data_to_send() {
	Scoped_lock(ts_lock);
	return buffer.size() && buffer.front()->is_ready;
}

Ref<GRPacket> GRStreamEncoderImageSequence::pop_data_to_send() {
	Scoped_lock(ts_lock);
	auto buf_img = buffer.front();
	buffer.pop();
	return buf_img->pack;
}

int GRStreamEncoderImageSequence::get_max_queued_frames() {
	Scoped_lock(ts_lock);
	return (int)threads.size() * 2;
}

void GRStreamEncoderImageSequence::start_encoder_threads(int count) {
	if (threads.size() == count)
		return;

	stop_encoder_threads();
	is_threads_active = true;
	ZoneScopedNC("Start Encoder Threads", tracy::Color::Firebrick);

	if (count < 1) {
		_log("threads count < 1. Disabling encoder.", LogLevel::LL_DEBUG);
		return;
	}

	if (count > GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS) {
		_log("threads count " + str(count) + " > 16. Clamping.", LogLevel::LL_ERROR);
		count = GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS;
	}

	threads.resize(count);
	for (int i = 0; i < count; i++) {
		Thread_start(threads[i], this, _processing_thread, i);
	}
}

void GRStreamEncoderImageSequence::stop_encoder_threads() {
	ZoneScopedNC("Stop Encoder Threads", tracy::Color::Firebrick3);
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

void GRStreamEncoderImageSequence::_init() {
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRStreamEncoder::_init();
#endif
}

void GRStreamEncoderImageSequence::_deinit() {
	LEAVE_IF_EDITOR();
	stop_encoder_threads();
	while (buffer.size()) {
		buffer.pop();
	}
}

void GRStreamEncoderImageSequence::_processing_thread(Variant p_userdata) {
	int thread_idx = p_userdata;
	OS *os = OS::get_singleton();

	PoolByteArray jpg_buffer;
	jpg_buffer.resize((1024 * 1024) * ((int)GET_PS(GodotRemote::ps_server_jpg_buffer_mb_size_name)));

	Thread_set_name(("Stream Encoder: Image Sequence " + str(thread_idx)).ascii().get_data());

	while (is_threads_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		int jpg_quality = viewport->jpg_quality;
		CommitedImage com_image = images.front();
		images.pop();

		while (buffer.size() > get_max_queued_frames()) {
			buffer.pop();
		}

		if (buffer.size()) {
			Ref<GRPacketImageData> im = buffer.front()->pack;
			if (im.is_valid() && im->get_is_stream_end()) {
				ts_lock.unlock();
				continue;
			}
		}

		auto buf_img = std::make_shared<BufferedImage>();
		buffer.push(buf_img);

		ts_lock.unlock();

		Ref<Image> img = com_image.img;
		PoolByteArray img_data;
		Ref<GRPacketImageData> pack = newref(GRPacketImageData);
		int bytes_in_color = img->get_format() == Image::FORMAT_RGB8 ? 3 : 4;

		pack->set_compression_type(compression_type);
		pack->set_size(Size2((float)img->get_width(), (float)img->get_height()));
		pack->set_format(img->get_format());
		pack->set_frametime(com_image.frametime);
		pack->set_start_time(com_image.time_added);
		pack->set_is_stream_end(false);

		if (img->get_data().size() == 0)
			goto end;

		switch (pack->get_compression_type()) {
			case GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED: {
				ZoneScopedNC("Image processed: Uncompressed", tracy::Color::VioletRed3);
				img_data = img->get_data();
				break;
			}
			case GRDevice::ImageCompressionType::COMPRESSION_JPG: {
				ZoneScopedNC("Image processed: JPG", tracy::Color::VioletRed3);
				if (!img_is_empty(img)) {
					Error err = Error::OK;

#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED
					err = GRUtilsJPGCodec::_compress_jpg_turbo(img_data, img->get_data(), jpg_buffer, (int)img->get_width(), (int)img->get_height(), bytes_in_color, jpg_quality);
#else
					err = GRUtilsJPGCodec::_compress_jpg(img_data, img->get_data(), jpg_buffer, (int)img->get_width(), (int)img->get_height(), bytes_in_color, jpg_quality);
#endif

					if ((int)err) {
						_log("Can't compress stream image JPG. Code: " + str((int)err), LogLevel::LL_ERROR);
						GRNotifications::add_notification("Stream Error", "Can't compress stream image to JPG. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
						goto end;
					}
				}
				break;
			}
			default:
				_log("Not implemented compression type: " + str(pack->get_compression_type()), LogLevel::LL_ERROR);
				goto end;
				break;
		}
		pack->set_image_data(img_data);
	end:
		ZoneScopedNC("Push Image to Buffer", tracy::Color::Violet);
		ts_lock.lock();
		buf_img->pack = pack;
		buf_img->is_ready = true;
		ts_lock.unlock();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// H264 ENCODER
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef GDNATIVE_LIBRARY

void GRStreamEncoderH264::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamEncoderH264::_processing_thread);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamEncoderH264::_register_methods() {
	METHOD_REG(GRStreamEncoderH264, _processing_thread);
	//register_property<GRStreamEncoders, String>("password", &GRStreamEncoders::set_password, &GRStreamEncoders::get_password, "");
	//register_signal<GRStreamEncoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
}

#endif

void GRStreamEncoderH264::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			GRStreamEncoder::_deinit();
			break;
	}
}

int GRStreamEncoderH264::_setup(int width, int height, int bps, float fps, float interval) {
	return 0;
}

void GRStreamEncoderH264::_stop_encoder() {
	return;
}

void GRStreamEncoderH264::commit_stream_end() {
	Scoped_lock(ts_lock);
	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();

	auto b = std::make_shared<BufferedImage>();
	auto p = newref(GRPacketImageData);
	p->set_is_stream_end(true);

	b->is_ready = true;
	b->pack = p;
	buffer.push(b);
}

bool GRStreamEncoderH264::has_data_to_send() {
	Scoped_lock(ts_lock);
	return buffer.size() && buffer.front()->is_ready;
}

Ref<GRPacket> GRStreamEncoderH264::pop_data_to_send() {
	Scoped_lock(ts_lock);
	auto buf_img = buffer.front();
	buffer.pop();
	return buf_img->pack;
}

int GRStreamEncoderH264::get_max_queued_frames() {
	Scoped_lock(ts_lock);
	return 8;
}

void GRStreamEncoderH264::start_encoder_threads(int count) {
	if (is_encoder_active)
		return;

	stop_encoder_threads();
	is_thread_active = true;
	ZoneScopedNC("Start Encoder Threads", tracy::Color::Firebrick);

	if (count < 1) {
		_log("threads count < 1. Disabling encoder.", LogLevel::LL_DEBUG);
		return;
	}

	if (count > GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS) {
		_log("threads count " + str(count) + " > 16. Clamping.", LogLevel::LL_ERROR);
		count = GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS;
	}

	Thread_start(thread, this, _processing_thread, Variant());
}

void GRStreamEncoderH264::stop_encoder_threads() {
	ZoneScopedNC("Stop Encoder Threads", tracy::Color::Firebrick3);
	_stop_encoder();
	is_encoder_active = false;
	is_thread_active = false;
	Thread_close(thread);

	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();
}

void GRStreamEncoderH264::_init() {
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRStreamEncoder::_init();
#endif
}

void GRStreamEncoderH264::_deinit() {
	LEAVE_IF_EDITOR();
	stop_encoder_threads();
	while (buffer.size()) {
		buffer.pop();
	}
}

void GRStreamEncoderH264::_processing_thread(Variant p_userdata) {
	int thread_idx = p_userdata;
	OS *os = OS::get_singleton();

	PoolByteArray jpg_buffer;
	jpg_buffer.resize((1024 * 1024) * ((int)GET_PS(GodotRemote::ps_server_jpg_buffer_mb_size_name)));

	Thread_set_name(("Stream Encoder: Image Sequence " + str(thread_idx)).ascii().get_data());

	while (is_thread_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		int jpg_quality = viewport->jpg_quality;
		CommitedImage com_image = images.front();
		images.pop();

		while (buffer.size() > get_max_queued_frames()) {
			buffer.pop();
		}

		if (buffer.size()) {
			Ref<GRPacketImageData> im = buffer.front()->pack;
			if (im.is_valid() && im->get_is_stream_end()) {
				ts_lock.unlock();
				continue;
			}
		}

		auto buf_img = std::make_shared<BufferedImage>();
		buffer.push(buf_img);

		ts_lock.unlock();

		Ref<Image> img = com_image.img;
		PoolByteArray img_data;
		Ref<GRPacketImageData> pack = newref(GRPacketImageData);
		int bytes_in_color = img->get_format() == Image::FORMAT_RGB8 ? 3 : 4;

		pack->set_compression_type(compression_type);
		pack->set_size(Size2((float)img->get_width(), (float)img->get_height()));
		pack->set_format(img->get_format());
		pack->set_frametime(com_image.frametime);
		pack->set_is_stream_end(false);

		if (img->get_data().size() == 0)
			goto end;

		switch (pack->get_compression_type()) {
			case GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED: {
				ZoneScopedNC("Image processed: Uncompressed", tracy::Color::VioletRed3);
				img_data = img->get_data();
				break;
			}
			case GRDevice::ImageCompressionType::COMPRESSION_JPG: {
				ZoneScopedNC("Image processed: JPG", tracy::Color::VioletRed3);
				if (!img_is_empty(img)) {
					Error err = Error::OK;

#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED
					err = GRUtilsJPGCodec::_compress_jpg_turbo(img_data, img->get_data(), jpg_buffer, (int)img->get_width(), (int)img->get_height(), bytes_in_color, jpg_quality);
#else
					err = GRUtilsJPGCodec::_compress_jpg(img_data, img->get_data(), jpg_buffer, (int)img->get_width(), (int)img->get_height(), bytes_in_color, jpg_quality);
#endif

					if ((int)err) {
						_log("Can't compress stream image JPG. Code: " + str((int)err), LogLevel::LL_ERROR);
						GRNotifications::add_notification("Stream Error", "Can't compress stream image to JPG. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
						goto end;
					}
				}
				break;
			}
			default:
				_log("Not implemented compression type: " + str(pack->get_compression_type()), LogLevel::LL_ERROR);
				goto end;
				break;
		}
		pack->set_image_data(img_data);
	end:
		ZoneScopedNC("Push Image to Buffer", tracy::Color::Violet);
		ts_lock.lock();
		buf_img->pack = pack;
		buf_img->is_ready = true;
		ts_lock.unlock();
	}
}

#endif // !NO_GODOTREMOTE_SERVER

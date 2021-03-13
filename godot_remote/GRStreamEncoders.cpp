/* GRStreamEncoders.cpp */

#ifndef NO_GODOTREMOTE_SERVER

#include "GRStreamEncoders.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRProfiler.h"
#include "GRServer.h"
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
	if (vp && encoder && viewport && viewport != vp) {
		encoder->set_viewport(vp);
		viewport = vp;
	}

	GRDevice::ImageCompressionType comp = (GRDevice::ImageCompressionType)compression;
	switch (comp) {
		case GRDevice::COMPRESSION_UNCOMPRESSED:
		case GRDevice::COMPRESSION_JPG:
		case GRDevice::COMPRESSION_PNG: {
			GRStreamEncoderImageSequence *tmp_en = encoder ? cast_to<GRStreamEncoderImageSequence>(encoder) : nullptr;
			if (tmp_en) {
				tmp_en->set_compression_type(compression);
			} else {
				if (encoder) {
					memdelete(encoder);
				}

				tmp_en = memnew(GRStreamEncoderImageSequence);
				tmp_en->set_compression_type(compression);
				encoder = tmp_en;
				encoder->set_viewport(vp);
				viewport = vp;
			}
			break;
		}
		default:
			break;
	}
}

void GRStreamEncodersManager::commit_image(Ref<Image> img, uint64_t frametime) {
	if (encoder) {
		encoder->commit_image(img, frametime);
	}
}

bool GRStreamEncodersManager::has_data_to_send() {
	if (encoder) {
		return encoder->has_data_to_send();
	}
	return false;
}

Ref<GRPacket> GRStreamEncodersManager::pop_data_to_send() {
	if (encoder) {
		return encoder->pop_data_to_send();
	}
	return Ref<GRPacket>();
}

void GRStreamEncodersManager::_init() {
}

void GRStreamEncodersManager::_deinit() {
	if (encoder)
		memdelete(encoder);
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
		_TS_LOCK_;
		while (images.size() > get_max_queued_frames()) {
			images.pop();
		}

		images.push(CommitedImage(img, OS::get_singleton()->get_ticks_usec(), frametime));
		_TS_UNLOCK_;
	} else {
		_log("Commited empty image to StreamEncoder. Ignoring.", LogLevel::LL_ERROR);
	}
}

void GRStreamEncoder::_init() {
}

void GRStreamEncoder::_deinit() {
}

///////////////////////////////////////////////////////////////////////////////
// IMAGE SEQUENCE ENCODER

#ifndef GDNATIVE_LIBRARY

void GRStreamEncoderImageSequence::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_processing_thread", "user_data"), &GRStreamEncoderImageSequence::_processing_thread);
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

bool GRStreamEncoderImageSequence::has_data_to_send() const {
	_TS_LOCK_;
	bool ret = buffer.size();
	_TS_UNLOCK_;
	return ret;
}

Ref<GRPacket> GRStreamEncoderImageSequence::pop_data_to_send() {
	_TS_LOCK_;
	auto data = buffer.back();
	buffer.pop_back();
	_TS_UNLOCK_;
	return data.pack;
}

int GRStreamEncoderImageSequence::get_max_queued_frames() {
	_TS_METHOD_;
	int ret = (int)threads.size();
	_TS_UNLOCK_;
	return ret;
}

void GRStreamEncoderImageSequence::_init() {
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRStreamEncoder::_init();
#endif

	int count = GET_PS(GodotRemote::ps_server_image_sequence_buffer_size_name);
	if (count < 1) {
		count = 1;
		_log("threads count " + str(count) + " < 1. Clamping.", LogLevel::LL_ERROR);
	}
	if (count > GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS) {
		_log("threads count " + str(count) + " > 16. Clamping.", LogLevel::LL_ERROR);
		count = GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS;
	}

	threads.resize(count);
	for (int i = 0; i < count; i++) {
		Thread_start(threads[i], this, _processing_thread, this);
	}
	//_log("img processing thread " + str(0), LogLevel::LL_NORMAL);
}

void GRStreamEncoderImageSequence::_deinit() {
	LEAVE_IF_EDITOR();
	is_threads_active = false;
	for (int i = 0; i < threads.size(); i++) {
		Thread_close(threads[i]);
	}
	threads.resize(0);
}

void GRStreamEncoderImageSequence::_processing_thread(Variant p_userdata) {
	GRStreamEncoderImageSequence *p = VARIANT_OBJ_CAST_TO(p_userdata, GRStreamEncoderImageSequence);
	OS *os = OS::get_singleton();

	Thread_set_name("ImageProcessingThread");

	while (p->is_threads_active) {
		p->_TS_LOCK_;

		if (p->images.size() == 0) {
			p->_TS_UNLOCK_;
			sleep_usec(1_ms);
			continue;
		}

		CommitedImage com_image = p->images.front();
		p->images.pop();

		p->_TS_UNLOCK_;

		Ref<Image> img = com_image.img;

		PoolByteArray img_data;

		Ref<GRPacketImageData> pack(memnew(GRPacketImageData));
		int bytes_in_color = img->get_format() == Image::FORMAT_RGB8 ? 3 : 4;

		pack->set_compression_type(p->compression_type);
		pack->set_size(Size2((float)img->get_width(), (float)img->get_height()));
		pack->set_format(img->get_format());
		pack->set_start_time(os->get_ticks_usec());
		pack->set_frametime(com_image.frametime);
		pack->set_is_empty(false);

		if (!(img->get_format() == Image::FORMAT_RGBA8 || img->get_format() == Image::FORMAT_RGB8)) {
			ZoneScopedN("Convert Image Format");
			img->convert(Image::FORMAT_RGB8);
			pack->set_format(img->get_format());
			bytes_in_color = img->get_format() == Image::FORMAT_RGB8 ? 3 : 4;

			if (img->get_format() != Image::FORMAT_RGB8) {
				_log("Can't convert stream image to RGB8.", LogLevel::LL_ERROR);
				GRNotifications::add_notification("Stream Error", "Can't convert stream image to RGB8.", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				goto end;
			}
		}

		if (img->get_data().size() == 0)
			goto end;

		switch (pack->get_compression_type()) {
			case GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED: {
				ZoneScopedN("Image processed: Uncompressed");
				img_data = img->get_data();
				break;
			}
			case GRDevice::ImageCompressionType::COMPRESSION_JPG: {
				ZoneScopedN("Image processed: JPG");
				if (!img_is_empty(img)) {
					Error err = compress_jpg(img_data, img->get_data(), (int)img->get_width(), (int)img->get_height(), bytes_in_color, p->viewport->jpg_quality, GRServer::Subsampling::SUBSAMPLING_H2V2);
					if ((int)err) {
						_log("Can't compress stream image JPG. Code: " + str((int)err), LogLevel::LL_ERROR);
						GRNotifications::add_notification("Stream Error", "Can't compress stream image to JPG. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
						goto end;
					}
				}
				break;
			}
			case GRDevice::ImageCompressionType::COMPRESSION_PNG: {
				ZoneScopedN("Image processed: PNG");
				img_data = img->save_png_to_buffer();
				if (img_data.size() == 0) {
					_log("Can't compress stream image to PNG.", LogLevel::LL_ERROR);
					GRNotifications::add_notification("Stream Error", "Can't compress stream image to PNG.", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
					goto end;
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
		if (p->video_stream_enabled) {
			ZoneScopedN("Push Image to Buffer");
			p->_TS_LOCK_;
			while (p->buffer.size() > p->threads.size() - 1) {
				p->buffer.pop_back();
			}
			p->buffer.push_back(BufferedImage(pack, com_image.time));
			std::sort(p->buffer.begin(), p->buffer.end());
			p->_TS_UNLOCK_;
		}
	}
}
#undef THREAD_METHOD

#endif // !NO_GODOTREMOTE_SERVER

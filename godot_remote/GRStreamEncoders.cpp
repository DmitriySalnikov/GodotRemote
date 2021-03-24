/* GRStreamEncoders.cpp */

#ifndef NO_GODOTREMOTE_SERVER

#include "GRStreamEncoders.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRServer.h"
#include "GRStreamEncoderH264.h"
#include "GRStreamEncoderImageSequence.h"
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
	active = true;

	GRDevice::ImageCompressionType comp = (GRDevice::ImageCompressionType)compression;
	switch (comp) {
		case GRDevice::COMPRESSION_JPG: {
			if (encoder)
				memdelete(encoder);
			encoder = memnew(GRStreamEncoderImageSequence);
			break;
		}
		case GRDevice::COMPRESSION_H264: {
			if (encoder)
				memdelete(encoder);
			encoder = memnew(GRStreamEncoderH264);
			break;
		}
		default: {
			active = false;
			_log("Attempt to run an unsupported encoder type. Encoder type: " + str(compression), LogLevel::LL_ERROR);
			if (encoder) {
				memdelete(encoder);
				encoder = nullptr;
			}
			return;
		}
	}

	encoder->set_compression_type(compression);
	encoder->set_viewport(vp);
	viewport = vp;
	encoder->start_encoder_threads(threads_count);
}

void GRStreamEncodersManager::set_compression_type(int compression, GRSViewport *vp) {
	if (!active)
		return;

	Scoped_lock(ts_lock);
	GRDevice::ImageCompressionType comp = (GRDevice::ImageCompressionType)compression;
	switch (comp) {
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
		default: {
			_log("Not supported encoder type: " + str(compression), LogLevel::LL_ERROR);
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

std::shared_ptr<GRPacketStreamData> GRStreamEncodersManager::pop_data_to_send() {
	Scoped_lock(ts_lock);
	if (encoder) {
		return encoder->pop_data_to_send();
	}
	return std::shared_ptr<GRPacketStreamData>();
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
#endif // !NO_GODOTREMOTE_SERVER

/* GRStreamDecoders.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRStreamDecoders.h"
#include "GRClient.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRStreamDecoderH264.h"
#include "GRStreamDecoderImageSequence.h"
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

void GRStreamDecodersManager::_start_decoder(std::shared_ptr<GRPacketStreamData> packet) {
	active = true;

	if (packet->get_compression_type() == GRDevice::ImageCompressionType::COMPRESSION_JPG) {
		GRStreamDecoderImageSequence *tmp_en = decoder ? cast_to<GRStreamDecoderImageSequence>(decoder) : nullptr;
		if (tmp_en) {
			tmp_en->start_decoder_threads(threads_count);
		} else {
			if (decoder) {
				decoder->stop_decoder_threads();
				memdelete(decoder);
				decoder = nullptr;
			}

			decoder = memnew(GRStreamDecoderImageSequence);
			decoder->set_gr_client(gr_client);
			decoder->start_decoder_threads(threads_count);
		}
		return;
	}

#ifdef GODOTREMOTE_H264_ENABLED
	if (packet->get_compression_type() == GRDevice::ImageCompressionType::COMPRESSION_H264) {
		GRStreamDecoderH264 *tmp_en = decoder ? cast_to<GRStreamDecoderH264>(decoder) : nullptr;
		if (tmp_en) {
			tmp_en->start_decoder_threads(threads_count);
		} else {
			if (decoder) {
				decoder->stop_decoder_threads();
				memdelete(decoder);
				decoder = nullptr;
			}

			decoder = memnew(GRStreamDecoderH264);
			decoder->set_gr_client(gr_client);
			decoder->start_decoder_threads(threads_count);
		}
		return;
	}
#endif

	active = false;
	if (decoder) {
		memdelete(decoder);
		decoder = nullptr;
		_log("Decoder got not supported packet", LogLevel::LL_ERROR);
	}
}

void GRStreamDecodersManager::set_gr_client(GRClient *client) {
	gr_client = client;
}

void GRStreamDecodersManager::push_packet_to_decode(std::shared_ptr<GRPacketStreamData> packet) {
	Scoped_lock(ts_lock);
	_start_decoder(packet);
	if (decoder) {
		return decoder->push_packet_to_decode(packet);
	}
}

void GRStreamDecodersManager::set_threads_count(int count) {
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
				decoder->start_decoder_threads(-1);
		}
	}
}

void GRStreamDecodersManager::_init() {
	LEAVE_IF_EDITOR();
}

void GRStreamDecodersManager::_deinit() {
	LEAVE_IF_EDITOR();
	Scoped_lock(ts_lock);
	if (decoder) {
		memdelete(decoder);
		decoder = nullptr;
	}
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

void GRStreamDecoder::push_packet_to_decode(std::shared_ptr<GRPacketStreamData> packet) {
	if (packet) {
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

#endif // NO_GODOTREMOTE_CLIENT

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
#include "core/os/thread_safe.h"
#include "scene/main/node.h"
#else
#include <Node.hpp>
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY

///////////////////////////////////////////////////////////////////////////////
// MANAGER

void GRStreamDecodersManager::_bind_methods() {
}

#else

void GRStreamDecodersManager::_register_methods() {
	METHOD_REG(GRStreamDecodersManager, _notification);
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

#ifdef GODOT_REMOTE_H264_ENABLED
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
}

#else

void GRStreamDecoder::_register_methods() {
	METHOD_REG(GRStreamDecoder, _notification);
}

#endif

void GRStreamDecoder::_sleep_waiting_next_frame(uint64_t frametime) {
	if (frametime > 0) {
		int wait_time = (int(frametime - (get_time_usec() - prev_shown_frame_time))) - 500;
		if (wait_time > (int)1_ms && wait_time < (int)frametime) {
			ZoneScopedN("Waiting next frame");
			sleep_usec(wait_time);
		} else {
		}
	} else {
		ZoneScopedN("Waiting next frame 1 ms");
		sleep_usec(1_ms);
	}
}

int64_t GRStreamDecoder::_calc_delay(uint64_t time, uint64_t start_time, uint64_t frametime) {
	return time - start_time - gr_client->sync_time_delta + int64_t(1000000.0 / Engine::get_singleton()->get_frames_per_second()) + frametime * 2;
}

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
	Scoped_lock(ts_lock);
	if (packet) {
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

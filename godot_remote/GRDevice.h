/* GRDevice.h */
#pragma once

#include "GRAVGCounter.h"
#include "GRInputData.h"
#include "GRPacket.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#include "core/io/tcp_server.h"
#include "scene/main/node.h"
#else
#include <Node.hpp>
#include <PacketPeerStream.hpp>
using namespace godot;
#endif

class GRDevice : public Node {
	GD_CLASS(GRDevice, Node);

public:
	enum class AuthResult : int {
		OK = 0,
		Error = 1,
		Timeout = 2,
		TryToConnect = 3,
		RefuseConnection = 4,
		VersionMismatch = 5,
		IncorrectPassword = 6,
		PasswordRequired = 7,
	};

	enum WorkingStatus : int {
		STATUS_STOPPED = 0,
		STATUS_WORKING = 1,
		STATUS_STOPPING = 2,
		STATUS_STARTING = 3,
	};

	enum TypesOfServerSettings : int {
		SERVER_SETTINGS_USE_INTERNAL = 0,
		SERVER_SETTINGS_VIDEO_STREAM_ENABLED = 1,
		SERVER_SETTINGS_COMPRESSION_TYPE = 2,
		SERVER_SETTINGS_STREAM_QUALITY = 3,
		SERVER_SETTINGS_SKIP_FRAMES = 4,
		SERVER_SETTINGS_RENDER_SCALE = 5,
		SERVER_SETTINGS_TARGET_FPS = 6,
		SERVER_SETTINGS_THREADS_NUMBER = 7,
	};

	enum ImageCompressionType : int {
		COMPRESSION_UNCOMPRESSED = 0,
		COMPRESSION_JPG = 1,
		COMPRESSION_PNG = 2,
		COMPRESSION_H264 = 3,
	};

private:
	WorkingStatus working_status = WorkingStatus::STATUS_STOPPED;

	uint64_t prev_traffic_counter_time = 0;
	uint32_t current_sec_bytes_received = 0;
	uint32_t current_sec_bytes_sended = 0;

	uint64_t total_bytes_received = 0;
	uint64_t total_bytes_sended = 0;

protected:
	template <class T>
	std::shared_ptr<T> _find_queued_packet_by_type(GRPacket::PacketType type) {
		for (int i = 0; i < send_queue.size(); i++) {
			auto o = send_queue[i];
			if (o->get_type() == type) {
				return shared_cast(T, o);
			}
		}
		return std::shared_ptr<T>();
	}

	GRAVGCounter<uint64_t, real_t> fps_counter = GRAVGCounter<uint64_t, real_t>([](real_t i) -> real_t { if (i > 0) return real_t(1000000.0 / i); else return 0; });
	GRAVGCounter<uint64_t, real_t> ping_counter = GRAVGCounter<uint64_t, real_t>([](real_t i) -> real_t { return real_t(i * 0.001); }, 20);
	// traffic in MB
	GRAVGCounter<uint64_t, real_t> traffic_recv_counter = GRAVGCounter<uint64_t, real_t>([](real_t i) -> real_t { return real_t(i / (double)MB_SIZE); }, 5);
	GRAVGCounter<uint64_t, real_t> traffic_send_counter = GRAVGCounter<uint64_t, real_t>([](real_t i) -> real_t { return real_t(i / (double)MB_SIZE); }, 5);

	Mutex_define(send_queue_mutex, "Device Send Queue Lock");
	std::vector<std::shared_ptr<GRPacket> > send_queue;

	void set_status(WorkingStatus status);
	Error send_data_to(RefStd(PacketPeerStream) ppeer, PoolByteArray data);
	Error recv_data_from(RefStd(PacketPeerStream) ppeer, PoolByteArray *data);
	void _update_avg_ping(uint64_t ping);
	void _update_avg_fps(uint64_t frametime);
	void _update_avg_traffic(uint32_t bytes_sended, uint32_t bytes_received);
	void _send_queue_resize(int new_size);
	std::shared_ptr<GRPacket> _send_queue_pop_front();

	virtual void _reset_counters();
	virtual void _internal_call_only_deffered_start(){};
	virtual void _internal_call_only_deffered_stop(){};

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	uint16_t static_port = PORT_STATIC_CONNECTION;
	uint16_t auto_connection_port = PORT_AUTO_CONNECTION;

	real_t get_avg_ping();
	real_t get_min_ping();
	real_t get_max_ping();
	real_t get_avg_fps();
	real_t get_min_fps();
	real_t get_max_fps();

	real_t get_avg_recv_mbyte();
	real_t get_min_recv_mbyte();
	real_t get_max_recv_mbyte();
	real_t get_avg_send_mbyte();
	real_t get_min_send_mbyte();
	real_t get_max_send_mbyte();

	real_t get_total_sended_mbytes();
	real_t get_total_received_mbytes();

	virtual uint16_t get_port();
	virtual void set_port(uint16_t _port);
	virtual uint16_t get_auto_connection_port();
	virtual void set_auto_connection_port(uint16_t _port);

	void send_packet(std::shared_ptr<GRPacket> packet);
	void send_user_data(Variant packet_id, Variant user_data, bool full_objects = false);

	void start();
	void stop();
	void restart();
	void _internal_call_only_deffered_restart();

	virtual WorkingStatus get_status();

	void _init();
	void _deinit();
};

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(GRDevice::WorkingStatus)
VARIANT_ENUM_CAST(GRDevice::ImageCompressionType)
VARIANT_ENUM_CAST(GRDevice::TypesOfServerSettings)
#endif

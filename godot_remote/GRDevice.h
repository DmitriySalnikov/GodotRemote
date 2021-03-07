/* GRDevice.h */
#pragma once

#include "GRInputData.h"
#include "GRPacket.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#include "scene/main/node.h"
#else
#include <Array.hpp>
#include <Godot.hpp>
#include <Node.hpp>
#include <PoolArrays.hpp>
#include <Ref.hpp>
#include <Reference.hpp>
#include <String.hpp>
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
		SERVER_SETTINGS_JPG_QUALITY = 3,
		SERVER_SETTINGS_SKIP_FRAMES = 4,
		SERVER_SETTINGS_RENDER_SCALE = 5,
	};

	enum Subsampling : int {
		SUBSAMPLING_Y_ONLY = 0,
		SUBSAMPLING_H1V1 = 1,
		SUBSAMPLING_H2V1 = 2,
		SUBSAMPLING_H2V2 = 3
	};

	enum ImageCompressionType : int {
		COMPRESSION_UNCOMPRESSED = 0,
		COMPRESSION_JPG = 1,
		COMPRESSION_PNG = 2,
	};

private:
	WorkingStatus working_status = WorkingStatus::STATUS_STOPPED;

protected:
	template <class T>
	T _find_queued_packet_by_type() {
		for (int i = 0; i < send_queue.size(); i++) {
			T o = send_queue[i];
			if (o.is_valid()) {
				return o;
			}
		}
		return T();
	}

	GRUtils::iterable_queue<uint64_t> fps_queue;
	GRUtils::iterable_queue<uint64_t> ping_queue;
	float avg_ping = 0, min_ping = 0, max_ping = 0;
	float avg_fps = 0, min_fps = 0, max_fps = 0;
	uint32_t avg_ping_max_count = 100;

	Mutex_define(send_queue_mutex);
	std::vector<Ref<GRPacket> > send_queue;

	void set_status(WorkingStatus status);
	void _update_avg_ping(uint64_t ping);
	void _update_avg_fps(uint64_t frametime);
	static float _ping_calc_modifier(double i);
	static float _fps_calc_modifier(double i);
	void _send_queue_resize(int new_size);
	Ref<GRPacket> _send_queue_pop_front();

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
	uint16_t port = 52341;

	float get_avg_ping();
	float get_min_ping();
	float get_max_ping();
	float get_avg_fps();
	float get_min_fps();
	float get_max_fps();
	uint16_t get_port();
	void set_port(uint16_t _port);

	void send_packet(Ref<GRPacket> packet);
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
VARIANT_ENUM_CAST(GRDevice::Subsampling)
VARIANT_ENUM_CAST(GRDevice::ImageCompressionType)
VARIANT_ENUM_CAST(GRDevice::TypesOfServerSettings)
#endif

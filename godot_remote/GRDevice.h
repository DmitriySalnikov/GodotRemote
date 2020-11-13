/* GRDevice.h */
#pragma once

#include "GRInputData.h"
#include "GRUtils.h"
#include "GRPacket.h"

#ifndef GDNATIVE_LIBRARY
#include "scene/main/node.h"
#else
#include <Node.hpp>
#include <Godot.hpp>
#include <Array.hpp>
#include <PoolArrays.hpp>
#include <Reference.hpp>
#include <Ref.hpp>
#include <String.hpp>
using namespace godot;
#endif

enum WorkingStatus : int {
	Stopped,
	Working,
	Stopping,
	Starting,
};

enum ConnectionType : int {
	CONNECTION_WiFi = 0,
	CONNECTION_ADB = 1,
};

enum StretchMode : int {
	STRETCH_KEEP_ASPECT = 0,
	STRETCH_FILL = 1,
};

enum StreamState : int {
	STREAM_NO_SIGNAL = 0,
	STREAM_ACTIVE = 1,
	STREAM_NO_IMAGE = 2,
};

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

private:
	WorkingStatus working_status = WorkingStatus::Stopped;

protected:
	float avg_ping = 0;
	float avg_fps = 0;
	float avg_ping_smoothing = 0.5f;
	float avg_fps_smoothing = 0.9f;

	void set_status(WorkingStatus status);
	void _update_avg_ping(uint64_t ping);
	void _update_avg_fps(uint64_t frametime);

	virtual void _reset_counters();
	virtual void _internal_call_only_deffered_start() {};
	virtual void _internal_call_only_deffered_stop() {};

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
	float get_avg_fps();
	uint16_t get_port();
	void set_port(uint16_t _port);

	void start();
	void stop();
	void restart();
	void _internal_call_only_deffered_restart();

	virtual WorkingStatus get_status();

	void _init();
	void _deinit();
};

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(WorkingStatus)
#endif

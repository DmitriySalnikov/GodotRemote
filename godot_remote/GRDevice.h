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

	CONST_FAKE_SET();
	CONST_GET(WorkingStatus, Starting);
	CONST_GET(WorkingStatus, Stopped);
	CONST_GET(WorkingStatus, Stopping);
	CONST_GET(WorkingStatus, Working);
	
	CONST_GET(InputType, _NoneIT);
	CONST_GET(InputType, _InputDeviceSensors);
	CONST_GET(InputType, _InputEvent);
	CONST_GET(InputType, _InputEventAction);
	CONST_GET(InputType, _InputEventGesture);
	CONST_GET(InputType, _InputEventJoypadButton);
	CONST_GET(InputType, _InputEventJoypadMotion);
	CONST_GET(InputType, _InputEventKey);
	CONST_GET(InputType, _InputEventMagnifyGesture);
	CONST_GET(InputType, _InputEventMIDI);
	CONST_GET(InputType, _InputEventMouse);
	CONST_GET(InputType, _InputEventMouseButton);
	CONST_GET(InputType, _InputEventMouseMotion);
	CONST_GET(InputType, _InputEventPanGesture);
	CONST_GET(InputType, _InputEventScreenDrag);
	CONST_GET(InputType, _InputEventScreenTouch);
	CONST_GET(InputType, _InputEventWithModifiers);
	CONST_GET(InputType, _InputEventMAX);
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

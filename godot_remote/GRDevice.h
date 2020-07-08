/* GRDevice.h */
#pragma once

#include "scene/main/node.h"
#include "GRUtils.h"

class GRDevice : public Node {
	GDCLASS(GRDevice, Node);

public:
	enum InputType {
		_InputDeviceSensors = 0,
		_InputEvent = 1,
		_InputEventAction = 2,
		_InputEventGesture = 3,
		_InputEventJoypadButton = 4,
		_InputEventJoypadMotion = 5,
		_InputEventKey = 6,
		_InputEventMagnifyGesture = 7,
		_InputEventMIDI = 8,
		_InputEventMouse = 9,
		_InputEventMouseButton = 10,
		_InputEventMouseMotion = 11,
		_InputEventPanGesture = 12,
		_InputEventScreenDrag = 13,
		_InputEventScreenTouch = 14,
		_InputEventWithModifiers = 15,
	};


protected:
	class GodotRemote *godot_remote = nullptr;
	bool working = false;
	float recv_avg_ping = 0;
	float recv_avg_fps = 0;
	float avg_ping_smoothing = 0.4f;
	float avg_fps_smoothing = 0.8f;

	virtual void _reset_counters();
	void _update_avg_ping(int ping);
	void _update_avg_fps(int frametime);

	static void _bind_methods();

public:
	uint16_t port = 52341;

	float get_avg_ping();
	float get_avg_fps();
	uint16_t get_port();
	void set_port(uint16_t _port);

	virtual bool start();
	virtual void stop() = 0;
	virtual bool is_working();

	GRDevice();
};

VARIANT_ENUM_CAST(GRDevice::InputType)

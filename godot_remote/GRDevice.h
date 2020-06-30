/* GRDevice.h */
#pragma once

#include "core/io/marshalls.h"
#include "core/pool_vector.h"
#include "scene/main/node.h"
#include "GRUtils.h"

class GRDevice : public Node {
	GDCLASS(GRDevice, Node);

public:
	enum PacketType {
		InitData = 0,
		ImageData = 1,
		InputData = 2,
	};

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

	static void _bind_methods();

public:
	uint16_t port = 52341;

	uint16_t get_port();
	void set_port(uint16_t _port);

	virtual bool start();
	virtual void stop() = 0;
	virtual bool is_working();

	GRDevice();
};

VARIANT_ENUM_CAST(GRDevice::PacketType)
VARIANT_ENUM_CAST(GRDevice::InputType)

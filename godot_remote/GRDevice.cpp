/* GRDevice.cpp */
#include "GRDevice.h"
#include "GodotRemote.h"

void GRDevice::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start"), &GRDevice::start);
	ClassDB::bind_method(D_METHOD("stop"), &GRDevice::stop);

	BIND_ENUM_CONSTANT(InitData);
	BIND_ENUM_CONSTANT(ImageData);
	BIND_ENUM_CONSTANT(InputData);

	BIND_ENUM_CONSTANT(_InputDeviceSensors)
	BIND_ENUM_CONSTANT(_InputEvent)
	BIND_ENUM_CONSTANT(_InputEventAction)
	BIND_ENUM_CONSTANT(_InputEventGesture)
	BIND_ENUM_CONSTANT(_InputEventJoypadButton)
	BIND_ENUM_CONSTANT(_InputEventJoypadMotion)
	BIND_ENUM_CONSTANT(_InputEventKey)
	BIND_ENUM_CONSTANT(_InputEventMagnifyGesture)
	BIND_ENUM_CONSTANT(_InputEventMIDI)
	BIND_ENUM_CONSTANT(_InputEventMouse)
	BIND_ENUM_CONSTANT(_InputEventMouseButton)
	BIND_ENUM_CONSTANT(_InputEventMouseMotion)
	BIND_ENUM_CONSTANT(_InputEventPanGesture)
	BIND_ENUM_CONSTANT(_InputEventScreenDrag)
	BIND_ENUM_CONSTANT(_InputEventScreenTouch)
	BIND_ENUM_CONSTANT(_InputEventWithModifiers)
}

bool GRDevice::start() {
	ERR_FAIL_COND_V_MSG(!godot_remote, false, "GRDevice created incorrectly!");
	return false;
}

GRDevice::GRDevice() {
	godot_remote = GodotRemote::get_singleton();
}

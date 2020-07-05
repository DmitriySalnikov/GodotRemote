/* GRDevice.cpp */
#include "GRDevice.h"
#include "GodotRemote.h"

void GRDevice::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_avg_ping"), &GRDevice::get_avg_ping);
	ClassDB::bind_method(D_METHOD("get_avg_fps"), &GRDevice::get_avg_fps);

	ClassDB::bind_method(D_METHOD("get_port"), &GRDevice::get_port);
	ClassDB::bind_method(D_METHOD("set_port", "port"), &GRDevice::set_port, DEFVAL(52341));

	ClassDB::bind_method(D_METHOD("start"), &GRDevice::start);
	ClassDB::bind_method(D_METHOD("stop"), &GRDevice::stop);
	ClassDB::bind_method(D_METHOD("is_working"), &GRDevice::is_working);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "port"), "set_port", "get_port");

	BIND_ENUM_CONSTANT(InitData);
	BIND_ENUM_CONSTANT(ImageData);
	BIND_ENUM_CONSTANT(InputData);
	BIND_ENUM_CONSTANT(Ping);

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

void GRDevice::_reset_counters() {
	recv_avg_fps = 0;
	recv_avg_ping = 0;
}

void GRDevice::_update_avg_ping(int ping) {
	recv_avg_ping = (recv_avg_ping * avg_ping_smoothing) + ((float)ping * (1.f - avg_ping_smoothing));
}

void GRDevice::_update_avg_fps(int frametime) {
	if (!frametime) {
		recv_avg_fps = (recv_avg_fps * avg_fps_smoothing) + (0 * (1.f - avg_fps_smoothing));
		return;
	}
	recv_avg_fps = (recv_avg_fps * avg_fps_smoothing) + ((1000.f / frametime) * (1.f - avg_fps_smoothing));
}

float GRDevice::get_avg_ping() {
	return recv_avg_ping;
}

float GRDevice::get_avg_fps() {
	return recv_avg_fps;
}

uint16_t GRDevice::get_port() {
	return port;
}

void GRDevice::set_port(uint16_t _port) {
	bool old = is_working();
	if (old)
		stop();

	port = _port;

	if (old)
		start();
}

bool GRDevice::start() {
	ERR_FAIL_COND_V_MSG(!godot_remote, false, "GRDevice created incorrectly!");
	return false;
}

bool GRDevice::is_working() {
	return working;
}

GRDevice::GRDevice() {
	godot_remote = GodotRemote::get_singleton();
}

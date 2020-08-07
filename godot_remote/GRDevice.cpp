/* GRDevice.cpp */
#include "GRDevice.h"
#include "GodotRemote.h"

using namespace GRUtils;

void GRDevice::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_internal_call_only_deffered_start"), &GRDevice::_internal_call_only_deffered_start);
	ClassDB::bind_method(D_METHOD("_internal_call_only_deffered_stop"), &GRDevice::_internal_call_only_deffered_stop);

	ClassDB::bind_method(D_METHOD("_internal_call_only_deffered_restart"), &GRDevice::_internal_call_only_deffered_restart);

	ClassDB::bind_method(D_METHOD("get_avg_ping"), &GRDevice::get_avg_ping);
	ClassDB::bind_method(D_METHOD("get_avg_fps"), &GRDevice::get_avg_fps);

	ClassDB::bind_method(D_METHOD("get_port"), &GRDevice::get_port);
	ClassDB::bind_method(D_METHOD("set_port", "port"), &GRDevice::set_port, DEFVAL(52341));

	ClassDB::bind_method(D_METHOD("start"), &GRDevice::start);
	ClassDB::bind_method(D_METHOD("stop"), &GRDevice::stop);
	ClassDB::bind_method(D_METHOD("get_status"), &GRDevice::get_status);

	ADD_SIGNAL(MethodInfo("status_changed", PropertyInfo(Variant::INT, "status")));
	
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "port"), "set_port", "get_port");

	BIND_ENUM_CONSTANT_CUSTOM(WorkingStatus::Starting, "STATUS_STARTING");
	BIND_ENUM_CONSTANT_CUSTOM(WorkingStatus::Stopping, "STATUS_STOPPING");
	BIND_ENUM_CONSTANT_CUSTOM(WorkingStatus::Working, "STATUS_WORKING");
	BIND_ENUM_CONSTANT_CUSTOM(WorkingStatus::Stopped, "STATUS_STOPPED");

	BIND_ENUM_CONSTANT_CUSTOM(InputType::_NoneIT, "InputTypeNone")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputDeviceSensors, "InputDeviceSensors")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEvent, "InputEvent")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventAction, "InputEventAction")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventGesture, "InputEventGesture")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventJoypadButton, "InputEventJoypadButton")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventJoypadMotion, "InputEventJoypadMotion")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventKey, "InputEventKey")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventMagnifyGesture, "InputEventMagnifyGesture")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventMIDI, "InputEventMIDI")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventMouse, "InputEventMouse")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventMouseButton, "InputEventMouseButton")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventMouseMotion, "InputEventMouseMotion")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventPanGesture, "InputEventPanGesture")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventScreenDrag, "InputEventScreenDrag")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventScreenTouch, "InputEventScreenTouch")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventWithModifiers, "InputEventWithModifiers")
	BIND_ENUM_CONSTANT_CUSTOM(InputType::_InputEventMAX, "InputEventMAX")
}

void GRDevice::_reset_counters() {
	avg_fps = 0;
	avg_ping = 0;
}

void GRDevice::_update_avg_ping(uint64_t ping) {
	if (!ping) {
		avg_ping = 0;
		return;
	}
	avg_ping = (avg_ping * avg_ping_smoothing) + ((float)(ping / 1000.f) * (1.f - avg_ping_smoothing));
}

void GRDevice::_update_avg_fps(uint64_t frametime) {
	if (!frametime) {
		avg_fps = 0;
		return;
	}
	avg_fps = (avg_fps * avg_fps_smoothing) + ((float)(1000000.f / frametime) * (1.f - avg_fps_smoothing));
}

void GRDevice::set_status(WorkingStatus status) {
	working_status = status;
	emit_signal("status_changed", working_status);
}

float GRDevice::get_avg_ping() {
	return avg_ping;
}

float GRDevice::get_avg_fps() {
	return avg_fps;
}

uint16_t GRDevice::get_port() {
	return port;
}

void GRDevice::set_port(uint16_t _port) {
	port = _port;
	restart();
}

void GRDevice::start() {
	call_deferred("_internal_call_only_deffered_start");
}

void GRDevice::stop() {
	call_deferred("_internal_call_only_deffered_stop");
}

void GRDevice::restart() {
	call_deferred("_internal_call_only_deffered_restart");
}

void GRDevice::_internal_call_only_deffered_restart() {
	if (get_status() == (int)WorkingStatus::Working) {
		_internal_call_only_deffered_stop();
		_internal_call_only_deffered_start();
	}
}

WorkingStatus GRDevice::get_status() {
	return working_status;
}

GRDevice::GRDevice() {
	port = GET_PS(GodotRemote::ps_general_port_name);
}

GRDevice::~GRDevice() {
	if (GodotRemote::get_singleton()) {
		GodotRemote::get_singleton()->device = nullptr;
	}
}

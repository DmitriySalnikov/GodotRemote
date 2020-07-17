/* GRDevice.h */
#pragma once

#include "scene/main/node.h"
#include "GRUtils.h"

class GRDevice : public Node {
	GDCLASS(GRDevice, Node);

public:
	enum class InputType {
		InputDeviceSensors = 0,
		InputEvent = 1,
		InputEventAction = 2,
		InputEventGesture = 3,
		InputEventJoypadButton = 4,
		InputEventJoypadMotion = 5,
		InputEventKey = 6,
		InputEventMagnifyGesture = 7,
		InputEventMIDI = 8,
		InputEventMouse = 9,
		InputEventMouseButton = 10,
		InputEventMouseMotion = 11,
		InputEventPanGesture = 12,
		InputEventScreenDrag = 13,
		InputEventScreenTouch = 14,
		InputEventWithModifiers = 15,
	};

	enum class WorkingStatus {
		Stopped,
		Working,
		Stopping,
		Starting,
	};

	enum class AuthResult {
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
	virtual void _internal_call_only_deffered_start() = 0;
	virtual void _internal_call_only_deffered_stop() = 0;

	static void _bind_methods();

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

	virtual int get_status();

	GRDevice();
	~GRDevice();
};

VARIANT_ENUM_CAST(GRDevice::WorkingStatus)
VARIANT_ENUM_CAST(GRDevice::InputType)

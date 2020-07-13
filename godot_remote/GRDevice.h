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

private:
	WorkingStatus working_status = WorkingStatus::Stopped;

protected:
	float avg_ping = 0;
	float avg_fps = 0;
	float avg_ping_smoothing = 0.4f;
	float avg_fps_smoothing = 0.8f;

	void set_status(WorkingStatus status);
	void _update_avg_ping(uint32_t ping);
	void _update_avg_fps(uint32_t frametime);

	virtual void _reset_counters();

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

	virtual void _internal_call_only_deffered_start() = 0;
	virtual void _internal_call_only_deffered_stop() = 0;
	virtual int get_status();

	GRDevice();
};

VARIANT_ENUM_CAST(GRDevice::WorkingStatus)
VARIANT_ENUM_CAST(GRDevice::InputType)

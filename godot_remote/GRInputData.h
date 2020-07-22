/* GRInputData.h */
#pragma once

#include "GRUtils.h"
#include "core/io/stream_peer.h"
#include "core/os/input_event.h"
#include "core/reference.h"

enum class InputType {
	None = 0,
	// Custom Input Data
	InputDeviceSensors = 1,

	// Input Events
	InputEvent = 64,
	InputEventAction = 65,
	InputEventGesture = 66,
	InputEventJoypadButton = 67,
	InputEventJoypadMotion = 68,
	InputEventKey = 69,
	InputEventMagnifyGesture = 70,
	InputEventMIDI = 71,
	InputEventMouse = 72,
	InputEventMouseButton = 73,
	InputEventMouseMotion = 74,
	InputEventPanGesture = 75,
	InputEventScreenDrag = 76,
	InputEventScreenTouch = 77,
	InputEventWithModifiers = 78,
	InputEventMAX,
};

VARIANT_ENUM_CAST(InputType)
//////////////////////////////////////////////////////////////////////////
// BASE CLASS

// GodotRemoteInputData
class GRInputData : public Reference {
	GDCLASS(GRInputData, Reference);
	friend class GRInputDeviceSensorsData;

protected:
	Ref<StreamPeerBuffer> data;
	virtual InputType _get_type() { return InputType::None; };

public:
	GRInputData() {
		data = Ref<StreamPeerBuffer>(memnew(StreamPeerBuffer));
	}

	PoolByteArray get_data() {
		return data->get_data_array();
	}
	void set_data(PoolByteArray &_data) {
		data->set_data_array(_data);
	}
	virtual InputType get_type() {
		if (data->get_size()) {
			data->seek(0);
			return (InputType)data->get_8();
		} else {
			return _get_type();
		}
	};
	static Ref<GRInputData> create(const PoolByteArray &buf);
};

//////////////////////////////////////////////////////////////////////////
// TODO for now all custom classes must add first 8 bits for type
// data->put_8((uint8_t)get_type())

//////////////////////////////////////////////////////////////////////////
// ADDITIONAL CLASSES

// Device Sensors
class GRInputDeviceSensorsData : public GRInputData {
	GDCLASS(GRInputDeviceSensorsData, GRInputData);

protected:
	virtual InputType _get_type() override { return InputType::InputDeviceSensors; };

public:
	virtual void set_sensors(PoolVector3Array _sensors);
	virtual PoolVector3Array get_sensors();
};

//////////////////////////////////////////////////////////////////////////
// INPUT EVENTS

// GodotRemoteInputEventData
class GRInputDataEvent : public GRInputData {
	GDCLASS(GRInputDataEvent, GRInputData);

protected:
	virtual Ref<InputEvent> _construct_event(Ref<InputEvent> ev, const Rect2 &rect) {
		data->seek(0);
		data->get_8();
		ev->set_device(data->get_32());
		return data;
	};
	virtual void _parse_event(const Ref<InputEvent> &ev, const Rect2 &rect) {
		data->resize(0);
		data->put_8((uint8_t)get_type());
		data->put_32(ev->get_device());
	};
	virtual InputType _get_type() override { return InputType::None; };

public:
	Ref<InputEvent> construct_event(const Rect2 &rect = Rect2());
	static Ref<GRInputDataEvent> parse_event(const Ref<InputEvent> &ev, const Rect2 &rect);
};

#define INPUT_EVENT_DATA(__class, _parent, _type)                                                 \
	class __class : public _parent {                                                              \
		GDCLASS(__class, _parent);                                                                \
		friend GRInputDataEvent;                                                                  \
		friend GRInputData;                                                                       \
                                                                                                  \
	protected:                                                                                    \
		virtual Ref<InputEvent> _construct_event(Ref<InputEvent> ev, const Rect2 &rect) override; \
		virtual void _parse_event(const Ref<InputEvent> &ev, const Rect2 &rect) override;         \
		virtual InputType _get_type() override { return _type; };                                 \
                                                                                                  \
	public:                                                                                       \
	}

INPUT_EVENT_DATA(GRIEDataWithModifiers, GRInputDataEvent, InputType::InputEventWithModifiers);
INPUT_EVENT_DATA(GRIEDataMouse, GRIEDataWithModifiers, InputType::InputEventMouse);
INPUT_EVENT_DATA(GRIEDataGesture, GRIEDataWithModifiers, InputType::InputEventGesture);

INPUT_EVENT_DATA(GRIEDataKey, GRIEDataWithModifiers, InputType::InputEventKey);
INPUT_EVENT_DATA(GRIEDataMouseButton, GRIEDataMouse, InputType::InputEventMouseButton);
INPUT_EVENT_DATA(GRIEDataMouseMotion, GRIEDataMouse, InputType::InputEventMouseMotion);
INPUT_EVENT_DATA(GRIEDataScreenTouch, GRInputDataEvent, InputType::InputEventScreenTouch);
INPUT_EVENT_DATA(GRIEDataScreenDrag, GRInputDataEvent, InputType::InputEventScreenDrag);
INPUT_EVENT_DATA(GRIEDataMagnifyGesture, GRIEDataGesture, InputType::InputEventMagnifyGesture);
INPUT_EVENT_DATA(GRIEDataPanGesture, GRIEDataGesture, InputType::InputEventPanGesture);
INPUT_EVENT_DATA(GRIEDataJoypadButton, GRInputDataEvent, InputType::InputEventJoypadButton);
INPUT_EVENT_DATA(GRIEDataJoypadMotion, GRInputDataEvent, InputType::InputEventJoypadMotion);
INPUT_EVENT_DATA(GRIEDataAction, GRInputDataEvent, InputType::InputEventAction);
INPUT_EVENT_DATA(GRIEDataMIDI, GRInputDataEvent, InputType::InputEventMIDI);

#undef INPUT_EVENT_DATA

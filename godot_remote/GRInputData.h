/* GRInputData.h */
#pragma once

#include "GRUtils.h"
#ifndef GDNATIVE_LIBRARY
#include "core/io/stream_peer.h"
#include "core/os/input_event.h"
#include "core/reference.h"
#else
#include <Array.hpp>
#include <Godot.hpp>
#include <InputEvent.hpp>
#include <InputEventAction.hpp>
#include <InputEventGesture.hpp>
#include <InputEventJoypadButton.hpp>
#include <InputEventJoypadMotion.hpp>
#include <InputEventKey.hpp>
#include <InputEventMIDI.hpp>
#include <InputEventMagnifyGesture.hpp>
#include <InputEventMouse.hpp>
#include <InputEventMouseButton.hpp>
#include <InputEventMouseMotion.hpp>
#include <InputEventPanGesture.hpp>
#include <InputEventScreenDrag.hpp>
#include <InputEventScreenTouch.hpp>
#include <InputEventWithModifiers.hpp>
#include <PoolArrays.hpp>
#include <Ref.hpp>
#include <Reference.hpp>
#include <StreamPeer.hpp>
#include <StreamPeerBuffer.hpp>
#include <String.hpp>
using namespace godot;
#endif

//////////////////////////////////////////////////////////////////////////
// BASE CLASS

// GodotRemoteInputData
class GRInputData : public Reference {
	GD_CLASS(GRInputData, Reference);
	friend class GRInputDeviceSensorsData;

public:
	enum InputType : int {
		_NoneIT = 0,
		// Custom Input Data
		_InputDeviceSensors = 1,

		// Input Events
		_InputEvent = 64,
		_InputEventAction = 65,
		_InputEventGesture = 66,
		_InputEventJoypadButton = 67,
		_InputEventJoypadMotion = 68,
		_InputEventKey = 69,
		_InputEventMagnifyGesture = 70,
		_InputEventMIDI = 71,
		_InputEventMouse = 72,
		_InputEventMouseButton = 73,
		_InputEventMouseMotion = 74,
		_InputEventPanGesture = 75,
		_InputEventScreenDrag = 76,
		_InputEventScreenTouch = 77,
		_InputEventWithModifiers = 78,
		_InputEventMAX,
	};

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods() {
		BIND_ENUM_CONSTANT(_NoneIT);
		BIND_ENUM_CONSTANT(_InputDeviceSensors);

		BIND_ENUM_CONSTANT(_InputEvent);
		BIND_ENUM_CONSTANT(_InputEventAction);
		BIND_ENUM_CONSTANT(_InputEventGesture);
		BIND_ENUM_CONSTANT(_InputEventJoypadButton);
		BIND_ENUM_CONSTANT(_InputEventJoypadMotion);
		BIND_ENUM_CONSTANT(_InputEventKey);
		BIND_ENUM_CONSTANT(_InputEventMagnifyGesture);
		BIND_ENUM_CONSTANT(_InputEventMIDI);
		BIND_ENUM_CONSTANT(_InputEventMouse);
		BIND_ENUM_CONSTANT(_InputEventMouseButton);
		BIND_ENUM_CONSTANT(_InputEventMouseMotion);
		BIND_ENUM_CONSTANT(_InputEventPanGesture);
		BIND_ENUM_CONSTANT(_InputEventScreenDrag);
		BIND_ENUM_CONSTANT(_InputEventScreenTouch);
		BIND_ENUM_CONSTANT(_InputEventWithModifiers);
		BIND_ENUM_CONSTANT(_InputEventMAX);
	}
#else
public:
	void _init(){};
	static void _register_methods(){};
protected:
#endif

	Ref<StreamPeerBuffer> data;
	virtual InputType _get_type() { return InputType::_NoneIT; };

public:
	GRInputData() {
		data = Ref<StreamPeerBuffer>(memnew(StreamPeerBuffer));
	}

	~GRInputData() {
		data->resize(0);
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
	GD_S_CLASS(GRInputDeviceSensorsData, GRInputData);

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual InputType _get_type() override { return InputType::_InputDeviceSensors; };

public:
	virtual void set_sensors(PoolVector3Array _sensors);
	virtual PoolVector3Array get_sensors();
};

//////////////////////////////////////////////////////////////////////////
// INPUT EVENTS

// GodotRemoteInputEventData
class GRInputDataEvent : public GRInputData {
	GD_S_CLASS(GRInputDataEvent, GRInputData);

protected:
	GDNATIVE_BASIC_REGISTER;

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
	virtual InputType _get_type() override { return InputType::_NoneIT; };

public:
	Ref<InputEvent> construct_event(const Rect2 &rect = Rect2());
	static Ref<GRInputDataEvent> parse_event(const Ref<InputEvent> &ev, const Rect2 &rect);
};

#define INPUT_EVENT_DATA(__class, _parent, _type)                                                 \
	class __class : public _parent {                                                              \
		GD_S_CLASS(__class, _parent);                                                             \
		friend GRInputDataEvent;                                                                  \
		friend GRInputData;                                                                       \
                                                                                                  \
	protected:                                                                                    \
		GDNATIVE_BASIC_REGISTER;                                                                  \
                                                                                                  \
		virtual Ref<InputEvent> _construct_event(Ref<InputEvent> ev, const Rect2 &rect) override; \
		virtual void _parse_event(const Ref<InputEvent> &ev, const Rect2 &rect) override;         \
		virtual InputType _get_type() override { return _type; };                                 \
                                                                                                  \
	public:                                                                                       \
	}

INPUT_EVENT_DATA(GRIEDataWithModifiers, GRInputDataEvent, InputType::_InputEventWithModifiers);
INPUT_EVENT_DATA(GRIEDataMouse, GRIEDataWithModifiers, InputType::_InputEventMouse);
INPUT_EVENT_DATA(GRIEDataGesture, GRIEDataWithModifiers, InputType::_InputEventGesture);

INPUT_EVENT_DATA(GRIEDataKey, GRIEDataWithModifiers, InputType::_InputEventKey);
INPUT_EVENT_DATA(GRIEDataMouseButton, GRIEDataMouse, InputType::_InputEventMouseButton);
INPUT_EVENT_DATA(GRIEDataMouseMotion, GRIEDataMouse, InputType::_InputEventMouseMotion);
INPUT_EVENT_DATA(GRIEDataScreenTouch, GRInputDataEvent, InputType::_InputEventScreenTouch);
INPUT_EVENT_DATA(GRIEDataScreenDrag, GRInputDataEvent, InputType::_InputEventScreenDrag);
INPUT_EVENT_DATA(GRIEDataMagnifyGesture, GRIEDataGesture, InputType::_InputEventMagnifyGesture);
INPUT_EVENT_DATA(GRIEDataPanGesture, GRIEDataGesture, InputType::_InputEventPanGesture);
INPUT_EVENT_DATA(GRIEDataJoypadButton, GRInputDataEvent, InputType::_InputEventJoypadButton);
INPUT_EVENT_DATA(GRIEDataJoypadMotion, GRInputDataEvent, InputType::_InputEventJoypadMotion);
INPUT_EVENT_DATA(GRIEDataAction, GRInputDataEvent, InputType::_InputEventAction);
INPUT_EVENT_DATA(GRIEDataMIDI, GRInputDataEvent, InputType::_InputEventMIDI);

#undef INPUT_EVENT_DATA

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(GRInputData::InputType)
#endif

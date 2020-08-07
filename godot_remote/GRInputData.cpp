/* GRInputData.cpp */
#include "GRInputData.h"
#include "GRPacket.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"

using namespace GRUtils;

void GRInputDeviceSensorsData::set_sensors(PoolVector3Array _sensors) {
	data->resize(0);
	data->put_8((uint8_t)get_type());
	data->put_var(_sensors);
}

PoolVector3Array GRInputDeviceSensorsData::get_sensors() {
	data->seek(0);
	data->get_8();
	return data->get_var();
}

Ref<GRInputData> GRInputData::create(const PoolByteArray &buf) {
#define CREATE(_d)                     \
	{                                  \
		Ref<_d> id(memnew(_d));        \
		id->data->set_data_array(buf); \
		return id;                     \
	}

	InputType type = (InputType)buf[0];
	switch (type) {
		case InputType::_NoneIT:
			ERR_PRINT("Can't create GRInputData with type 'None'!");
			break;
			// ADDITIONAL CLASSES
		case InputType::_InputDeviceSensors:
			CREATE(GRInputDeviceSensorsData);

			// INPUT EVENTS
		case InputType::_InputEvent:
		case InputType::_InputEventWithModifiers:
		case InputType::_InputEventMouse:
		case InputType::_InputEventGesture:
			ERR_PRINT("Can't create GRInputData for abstract InputEvent! Type index: " + str((int)type));
			break;
		case InputType::_InputEventAction:
			CREATE(GRIEDataAction);
		case InputType::_InputEventJoypadButton:
			CREATE(GRIEDataJoypadButton);
		case InputType::_InputEventJoypadMotion:
			CREATE(GRIEDataJoypadMotion);
		case InputType::_InputEventKey:
			CREATE(GRIEDataKey);
		case InputType::_InputEventMagnifyGesture:
			CREATE(GRIEDataMagnifyGesture);
		case InputType::_InputEventMIDI:
			CREATE(GRIEDataMIDI);
		case InputType::_InputEventMouseButton:
			CREATE(GRIEDataMouseButton);
		case InputType::_InputEventMouseMotion:
			CREATE(GRIEDataMouseMotion);
		case InputType::_InputEventPanGesture:
			CREATE(GRIEDataPanGesture);
		case InputType::_InputEventScreenDrag:
			CREATE(GRIEDataScreenDrag);
		case InputType::_InputEventScreenTouch:
			CREATE(GRIEDataScreenTouch);
	}
#undef CREATE

	ERR_PRINT("Can't create unsupported GRInputData! Type index: " + str((int)type));
	return Ref<GRInputData>();
}

Ref<GRInputDataEvent> GRInputDataEvent::parse_event(const Ref<InputEvent> &ev, const Rect2 &rect) {
	if (ev.is_null())
		ERR_FAIL_COND_V(ev.is_null(), Ref<GRInputDataEvent>());

#define PARSE(_i, _d)                     \
	{                                     \
		Ref<_i> ie = ev;                  \
		if (ie.is_valid()) {              \
			Ref<_d> data(memnew(_d));     \
			data->_parse_event(ie, rect); \
			return data;                  \
		}                                 \
	}

	PARSE(InputEventKey, GRIEDataKey);
	PARSE(InputEventMouseButton, GRIEDataMouseButton);
	PARSE(InputEventMouseMotion, GRIEDataMouseMotion);
	PARSE(InputEventScreenTouch, GRIEDataScreenTouch);
	PARSE(InputEventScreenDrag, GRIEDataScreenDrag);
	PARSE(InputEventMagnifyGesture, GRIEDataMagnifyGesture);
	PARSE(InputEventPanGesture, GRIEDataPanGesture);
	PARSE(InputEventJoypadButton, GRIEDataJoypadButton);
	PARSE(InputEventJoypadMotion, GRIEDataJoypadMotion);
	PARSE(InputEventAction, GRIEDataAction);
	PARSE(InputEventMIDI, GRIEDataMIDI);

#undef PARSE

	ERR_PRINT("Not supported InputEvent type: " + str(ev));
	return Ref<GRInputDataEvent>();
}

Ref<InputEvent> GRInputDataEvent::construct_event(const Rect2 &rect) {
	ERR_FAIL_COND_V(!data->get_size(), Ref<InputEvent>());

#define CONSTRUCT(_i)                         \
	{                                         \
		Ref<_i> ev(memnew(_i));               \
		return _construct_event(ev, vp_size); \
	}

	InputType type = _get_type();
	ERR_FAIL_COND_V_MSG(type < InputType::_InputEvent || type >= InputType::_InputEventMAX, Ref<GRInputDataEvent>(), "Not InputEvent");

	Rect2 vp_size = rect;
	if (vp_size.size.x == 0 && vp_size.size.y == 0 &&
			vp_size.position.x == 0 && vp_size.position.y == 0) {
		if (SceneTree::get_singleton() && SceneTree::get_singleton()->get_root()) {
			//vp_size = SceneTree::get_singleton()->get_root()->get_visible_rect();
			vp_size = Rect2(OS::get_singleton()->get_window_size(), SceneTree::get_singleton()->get_root()->get_size());
		}
	}

	switch (type) {
		case InputType::_NoneIT:
			ERR_PRINT("Can't create GRInputDataEvent with type 'None'!");
			break;
		case InputType::_InputEvent:
		case InputType::_InputEventWithModifiers:
		case InputType::_InputEventMouse:
		case InputType::_InputEventGesture:
			ERR_PRINT("Can't create GRInputDataEvent for abstract InputEvent! Type index: " + str((int)type));
			break;
		case InputType::_InputEventAction:
			CONSTRUCT(InputEventAction);
		case InputType::_InputEventJoypadButton:
			CONSTRUCT(InputEventJoypadButton);
		case InputType::_InputEventJoypadMotion:
			CONSTRUCT(InputEventJoypadMotion);
		case InputType::_InputEventKey:
			CONSTRUCT(InputEventKey);
		case InputType::_InputEventMagnifyGesture:
			CONSTRUCT(InputEventMagnifyGesture);
		case InputType::_InputEventMIDI:
			CONSTRUCT(InputEventMIDI);
		case InputType::_InputEventMouseButton:
			CONSTRUCT(InputEventMouseButton);
		case InputType::_InputEventMouseMotion:
			CONSTRUCT(InputEventMouseMotion);
		case InputType::_InputEventPanGesture:
			CONSTRUCT(InputEventPanGesture);
		case InputType::_InputEventScreenDrag:
			CONSTRUCT(InputEventScreenDrag);
		case InputType::_InputEventScreenTouch:
			CONSTRUCT(InputEventScreenTouch);
	}

#undef CONSTRUCT

	return Ref<InputEvent>();
}

#define fix(_e) ((Vector2(_e) - rect.position) / rect.size)
#define fix_rel(_e) (Vector2(_e) / rect.size)

#define restore(_e) ((Vector2(_e) * rect.size) + ((rect.position - rect.size) / 2.f))
#define restore_rel(_e) (Vector2(_e) * rect.size)

#define CONSTRUCT(_type) Ref<InputEvent> _type::_construct_event(Ref<InputEvent> ev, const Rect2 &rect)
#define PARSE(_type) void _type::_parse_event(const Ref<InputEvent> &ev, const Rect2 &rect)

//////////////////////////////////////////////////////////////////////////
// InputEventWithModifiers
CONSTRUCT(GRIEDataWithModifiers) {
	GRInputDataEvent::_construct_event(ev, rect);
	Ref<InputEventWithModifiers> iewm = ev;
	uint8_t flags = data->get_8();
	iewm->set_alt(flags & (1 << 0));
	iewm->set_shift(flags & (1 << 1));
	iewm->set_control(flags & (1 << 2));
	iewm->set_metakey(flags & (1 << 3));
	iewm->set_command(flags & (1 << 4));
	return iewm;
}

PARSE(GRIEDataWithModifiers) {
	GRInputDataEvent::_parse_event(ev, rect);
	Ref<InputEventWithModifiers> iewm = ev;
	data->put_8((uint8_t)iewm->get_alt() | (uint8_t)iewm->get_shift() << 1 | (uint8_t)iewm->get_control() << 2 |
				(uint8_t)iewm->get_metakey() << 3 | (uint8_t)iewm->get_command() << 4);
}

//////////////////////////////////////////////////////////////////////////
// InputEventMouse
CONSTRUCT(GRIEDataMouse) {
	GRIEDataWithModifiers::_construct_event(ev, rect);
	Ref<InputEventMouse> iem = ev;
	iem->set_button_mask(data->get_32());
	iem->set_position(restore(data->get_var()));
	iem->set_global_position(restore(data->get_var()));
	return iem;
}

PARSE(GRIEDataMouse) {
	GRIEDataWithModifiers::_parse_event(ev, rect);
	Ref<InputEventMouse> iem = ev;
	data->put_32(iem->get_button_mask());
	data->put_var(fix(iem->get_position()));
	data->put_var(fix(iem->get_global_position()));
}

//////////////////////////////////////////////////////////////////////////
// InputEventGesture
CONSTRUCT(GRIEDataGesture) {
	GRIEDataWithModifiers::_construct_event(ev, rect);
	Ref<InputEventGesture> ieg = ev;
	ieg->set_position(restore(data->get_var()));
	return ieg;
}

PARSE(GRIEDataGesture) {
	GRIEDataWithModifiers::_parse_event(ev, rect);
	Ref<InputEventGesture> ieg = ev;
	data->put_var(fix(ieg->get_position()));
}

//////////////////////////////////////////////////////////////////////////
// InputEventKey
CONSTRUCT(GRIEDataKey) {
	GRIEDataWithModifiers::_construct_event(ev, rect);
	Ref<InputEventKey> iek = ev;
	uint8_t flags = data->get_8();
	iek->set_pressed(flags & 1);
	iek->set_echo((flags >> 1) & 1);
	iek->set_scancode(data->get_32());
	iek->set_unicode(data->get_32());
	return iek;
}

PARSE(GRIEDataKey) {
	GRIEDataWithModifiers::_parse_event(ev, rect);
	Ref<InputEventKey> iek = ev;
	data->put_8((uint8_t)iek->is_pressed() | (uint8_t)iek->is_echo() << 1);
	data->put_32(iek->get_scancode());
	data->put_32(iek->get_unicode());
}

//////////////////////////////////////////////////////////////////////////
// InputEventMouseButton
CONSTRUCT(GRIEDataMouseButton) {
	GRIEDataMouse::_construct_event(ev, rect);
	Ref<InputEventMouseButton> iemb = ev;
	iemb->set_factor(data->get_float());
	iemb->set_button_index(data->get_16());
	uint8_t flags = data->get_8();
	iemb->set_pressed(flags & 1);
	iemb->set_doubleclick((flags >> 1) & 1);
	return iemb;
}

PARSE(GRIEDataMouseButton) {
	GRIEDataMouse::_parse_event(ev, rect);
	Ref<InputEventMouseButton> iemb = ev;
	data->put_float(iemb->get_factor());
	data->put_16(iemb->get_button_index());
	data->put_8((uint8_t)iemb->is_pressed() | (uint8_t)iemb->is_doubleclick() << 1);
}

//////////////////////////////////////////////////////////////////////////
// InputEventMouseMotion
CONSTRUCT(GRIEDataMouseMotion) {
	GRIEDataMouse::_construct_event(ev, rect);
	Ref<InputEventMouseMotion> iemm = ev;
	iemm->set_pressure(data->get_float());
	iemm->set_tilt(data->get_var());
	iemm->set_relative(restore_rel(data->get_var()));
	iemm->set_speed(restore_rel(data->get_var()));
	return iemm;
}

PARSE(GRIEDataMouseMotion) {
	GRIEDataMouse::_parse_event(ev, rect);
	Ref<InputEventMouseMotion> iemm = ev;
	data->put_float(iemm->get_pressure());
	data->put_var(iemm->get_tilt());
	data->put_var(fix_rel(iemm->get_relative()));
	data->put_var(fix_rel(iemm->get_speed()));
}

//////////////////////////////////////////////////////////////////////////
// InputEventScreenTouch
CONSTRUCT(GRIEDataScreenTouch) {
	GRInputDataEvent::_construct_event(ev, rect);
	Ref<InputEventScreenTouch> iest = ev;
	iest->set_index(data->get_8());
	iest->set_pressed(data->get_8());
	iest->set_position(restore(data->get_var()));
	return iest;
}

PARSE(GRIEDataScreenTouch) {
	GRInputDataEvent::_parse_event(ev, rect);
	Ref<InputEventScreenTouch> iest = ev;
	data->put_8(iest->get_index());
	data->put_8(iest->is_pressed());
	data->put_var(fix(iest->get_position()));
}

//////////////////////////////////////////////////////////////////////////
// InputEventScreenDrag
CONSTRUCT(GRIEDataScreenDrag) {
	GRInputDataEvent::_construct_event(ev, rect);
	Ref<InputEventScreenDrag> iesd = ev;
	iesd->set_index(data->get_8());
	iesd->set_position(restore(data->get_var()));
	iesd->set_relative(restore_rel(data->get_var()));
	iesd->set_speed(restore_rel(data->get_var()));
	return iesd;
}

PARSE(GRIEDataScreenDrag) {
	GRInputDataEvent::_parse_event(ev, rect);
	Ref<InputEventScreenDrag> iesd = ev;
	data->put_8(iesd->get_index());
	data->put_var(fix(iesd->get_position()));
	data->put_var(fix_rel(iesd->get_relative()));
	data->put_var(fix_rel(iesd->get_speed()));
}

//////////////////////////////////////////////////////////////////////////
// InputEventMagnifyGesture
CONSTRUCT(GRIEDataMagnifyGesture) {
	GRIEDataGesture::_construct_event(ev, rect);
	Ref<InputEventMagnifyGesture> iemg = ev;
	iemg->set_factor(data->get_float());
	return iemg;
}

PARSE(GRIEDataMagnifyGesture) {
	GRIEDataGesture::_parse_event(ev, rect);
	Ref<InputEventMagnifyGesture> iemg = ev;
	data->put_float(iemg->get_factor());
}

//////////////////////////////////////////////////////////////////////////
// InputEventPanGesture
CONSTRUCT(GRIEDataPanGesture) {
	GRIEDataGesture::_construct_event(ev, rect);
	Ref<InputEventPanGesture> iepg = ev;
	iepg->set_delta(restore_rel(data->get_var()));
	return iepg;
}

PARSE(GRIEDataPanGesture) {
	GRIEDataGesture::_parse_event(ev, rect);
	Ref<InputEventPanGesture> iepg = ev;
	data->put_var(fix_rel(iepg->get_delta()));
}

//////////////////////////////////////////////////////////////////////////
// InputEventPanGesture
CONSTRUCT(GRIEDataJoypadButton) {
	GRInputDataEvent::_construct_event(ev, rect);
	Ref<InputEventJoypadButton> iejb = ev;
	iejb->set_button_index(data->get_32());
	iejb->set_pressure(data->get_float());
	iejb->set_pressed(data->get_8());
	return iejb;
}

PARSE(GRIEDataJoypadButton) {
	GRInputDataEvent::_parse_event(ev, rect);
	Ref<InputEventJoypadButton> iejb = ev;
	data->put_32(iejb->get_button_index());
	data->put_float(iejb->get_pressure());
	data->put_8(iejb->is_pressed());
}

//////////////////////////////////////////////////////////////////////////
// InputEventJoypadMotion
CONSTRUCT(GRIEDataJoypadMotion) {
	GRInputDataEvent::_construct_event(ev, rect);
	Ref<InputEventJoypadMotion> iejm = ev;
	iejm->set_axis(data->get_32());
	iejm->set_axis_value(data->get_float());
	return iejm;
}

PARSE(GRIEDataJoypadMotion) {
	GRInputDataEvent::_parse_event(ev, rect);
	Ref<InputEventJoypadMotion> iejm = ev;
	data->put_32(iejm->get_axis());
	data->put_float(iejm->get_axis_value());
}

//////////////////////////////////////////////////////////////////////////
// InputEventAction
CONSTRUCT(GRIEDataAction) {
	GRInputDataEvent::_construct_event(ev, rect);
	Ref<InputEventAction> iea = ev;
	iea->set_action(data->get_var());
	iea->set_strength(data->get_float());
	iea->set_pressed(data->get_8());
	return iea;
}

PARSE(GRIEDataAction) {
	GRInputDataEvent::_parse_event(ev, rect);
	Ref<InputEventAction> iea = ev;
	data->put_var(iea->get_action());
	data->put_float(iea->get_strength());
	data->put_8(iea->is_pressed());
}

//////////////////////////////////////////////////////////////////////////
// InputEventAction
CONSTRUCT(GRIEDataMIDI) {
	GRInputDataEvent::_construct_event(ev, rect);
	Ref<InputEventMIDI> iemidi = ev;
	iemidi->set_channel(data->get_32());
	iemidi->set_message(data->get_32());
	iemidi->set_pitch(data->get_32());
	iemidi->set_velocity(data->get_32());
	iemidi->set_instrument(data->get_32());
	iemidi->set_pressure(data->get_32());
	iemidi->set_controller_number(data->get_32());
	iemidi->set_controller_value(data->get_32());
	return iemidi;
}

PARSE(GRIEDataMIDI) {
	GRInputDataEvent::_parse_event(ev, rect);
	Ref<InputEventMIDI> iemidi = ev;
	data->put_32(iemidi->get_channel());
	data->put_32(iemidi->get_message());
	data->put_32(iemidi->get_pitch());
	data->put_32(iemidi->get_velocity());
	data->put_32(iemidi->get_instrument());
	data->put_32(iemidi->get_pressure());
	data->put_32(iemidi->get_controller_number());
	data->put_32(iemidi->get_controller_value());
}

#undef fix
#undef fix_rel
#undef restore
#undef CONSTRUCT
#undef PARSE

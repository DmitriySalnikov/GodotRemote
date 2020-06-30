/* GRDeviceStandalone.cpp */
#include "GRDeviceStandalone.h"
#include "GodotRemote.h"
#include "core/input_map.h"
#include "core/io/tcp_server.h"
#include "core/os/input_event.h"
#include "core/os/thread_safe.h"
#include "main/input_default.h"
#include "scene/2d/sprite.h"
#include "scene/gui/control.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/texture.h"
#include <memory>

using namespace GRUtils;

void GRDeviceStandalone::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_control_to_show_in", "control_node", "position_in_node"), &GRDeviceStandalone::set_control_to_show_in, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("_update_texture_from_iamge", "image"), &GRDeviceStandalone::_update_texture_from_iamge);

	ClassDB::bind_method(D_METHOD("set_capture_on_focus", "val"), &GRDeviceStandalone::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD("set_capture_when_hover", "val"), &GRDeviceStandalone::set_capture_when_hover);

	ClassDB::bind_method(D_METHOD("is_capture_on_focus"), &GRDeviceStandalone::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD("is_capture_when_hover"), &GRDeviceStandalone::is_capture_when_hover);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), "set_capture_on_focus", "is_capture_on_focus");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), "set_capture_when_hover", "is_capture_when_hover");
}

void GRDeviceStandalone::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_EXIT_TREE: {
			stop();
			break;
		}
		case NOTIFICATION_PREDELETE: {
			stop();
			break;
		}
	}
}

GRDeviceStandalone::GRDeviceStandalone() :
		GRDevice() {

	set_name("GodotRemoteClient");
	peer_send.instance();
	peer_recv.instance();
}

void GRDeviceStandalone::set_control_to_show_in(Control *ctrl, int position_in_node) {
	if (sprite_shows_stream && !sprite_shows_stream->is_queued_for_deletion()) {
		sprite_shows_stream->queue_delete();
	}
	if (input_collector && !input_collector->is_queued_for_deletion()) {
		input_collector->queue_delete();
	}

	control_to_show_in = ctrl;

	if (control_to_show_in && !control_to_show_in->is_queued_for_deletion()) {
		sprite_shows_stream = new Sprite();
		input_collector = new GRInputCollector();

		sprite_shows_stream->set_name("GodotRemoteStreamSprite");
		input_collector->set_name("GodotRemoteInputCollector");

		control_to_show_in->add_child(sprite_shows_stream);
		control_to_show_in->move_child(sprite_shows_stream, position_in_node);
		control_to_show_in->add_child(input_collector);

		input_collector->set_capture_on_focus(capture_only_when_control_in_focus);
	}
}

bool GRDeviceStandalone::is_capture_on_focus() {
	return capture_only_when_control_in_focus;
}

void GRDeviceStandalone::set_capture_on_focus(bool value) {
	capture_only_when_control_in_focus = value;

	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_on_focus(capture_only_when_control_in_focus);
}

bool GRDeviceStandalone::is_capture_when_hover() {
	return capture_pointer_only_when_hover_control;
}

void GRDeviceStandalone::set_capture_when_hover(bool value) {
	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_when_hover(capture_pointer_only_when_hover_control);
}

bool GRDeviceStandalone::start() {
	GRDevice::start();

	if (working) {
		ERR_FAIL_V_MSG(false, "Can't start already working Godot Remote Server");
	}

	log("Starting GodotRemote client");

	working = true;
	stop_device = false;
	break_connection = false;
	thread_send_data = Thread::create(&_thread_send_recv_data, new StartThreadArgs(this, false));
	thread_send_data->set_name("GRemote_send_data_thread");
	thread_recv_data = Thread::create(&_thread_send_recv_data, new StartThreadArgs(this, true));
	thread_recv_data->set_name("GRemote_recv_data_thread");
	return true;
}

void GRDeviceStandalone::stop() {
	if (!working)
		return;
	working = false;

	stop_device = true;
	break_connection = true;

	if (sprite_shows_stream && !sprite_shows_stream->is_queued_for_deletion()) {
		sprite_shows_stream->queue_delete();
	}
	if (input_collector && !input_collector->is_queued_for_deletion()) {
		input_collector->queue_delete();
	}

	if (thread_send_data) {
		Thread::wait_to_finish(thread_send_data);
	}
	if (thread_recv_data) {
		Thread::wait_to_finish(thread_recv_data);
	}
	thread_send_data = nullptr;
	thread_recv_data = nullptr;

	return;
}

void GRDeviceStandalone::_update_texture_from_iamge(Ref<Image> img) {
	Ref<ImageTexture> tex;
	tex.instance();
	tex->create_from_image(img);

	if (control_to_show_in && !control_to_show_in->is_queued_for_deletion()) {
		Vector2 size = control_to_show_in->get_rect().size;

		Sprite *s = sprite_shows_stream;
		if (s && !s->is_queued_for_deletion()) {
			s->set_texture(tex);

			Vector2 new_pos = size / 2;
			if (s->get_position() != new_pos)
				s->set_position(new_pos);

			Vector2 new_scale = (Vector2(1, 1) / Vector2(tex->get_width(), tex->get_height())) * size;
			if (s->get_scale() != new_scale)
				s->set_scale(new_scale);
		}
	}
}

bool GRDeviceStandalone::_auth_on_server(Ref<StreamPeerTCP> con, bool is_recv) {
	String type = is_recv ? "Receive thread " : "Send thread ";

	// Auth Packet
	// 4bytes GodotRemote Header
	// 4bytes Body Size
	// 3byte  Version
	// 1byte  Is receiving client type
	// ....
	//
	// total body size = 4
	//
	// must receive
	// 4bytes GodotRemote Header
	// 1byte  error code

	PoolByteArray data;
	data.append_array(get_packet_header()); // header

	/* everything after "body size" */
	PoolByteArray auth;
	auth.append_array(get_version()); // version
	auth.append(is_recv); // is recv

	/* finalize */
	data.append_array(int322bytes(auth.size())); // body size
	data.append_array(auth);

	auto r = data.read();
	Error err = con->put_data(r.ptr(), data.size());
	r.release();

	if (err == OK) {
		PoolByteArray h;
		h.resize(4);
		auto w = h.write();
		err = con->get_data(w.ptr(), 5);
		w.release();

		if (err != OK)
			return false;

		r = h.read();
		if (validate_packet(r.ptr())) {
			AuthErrorCode e = (AuthErrorCode)r[4];

			switch (e) {
				case GRUtils::AuthErrorCode::OK:
					return true;
				case GRUtils::AuthErrorCode::VersionMismatch:
					log(type + "can't connect to server. Version mismatch.");
					return false;
			}
		}
	} else {
		return false;
	}

	return false;
}

void GRDeviceStandalone::_send_data(GRDeviceStandalone *dev, Ref<StreamPeerTCP> con) {
	OS *os = OS::get_singleton();

	uint32_t prev_time = os->get_ticks_msec();
	while (!dev->break_connection && con->is_connected_to_host()) {
		uint32_t time = os->get_ticks_msec();
		if ((time - prev_time) > (1000 / dev->send_data_fps)) {
			prev_time = time;

			PoolByteArray d = _process_input_data(dev);

			if (d.size() == 0)
				continue;

			PoolByteArray data;
			data.append_array(get_packet_header());
			data.append(PacketType::InputData);
			data.append_array(var2bytes(d.size()));
			data.append_array(d);

			auto r = data.read();
			Error err = con->put_data(r.ptr(), data.size());
			r.release();

			if (err) {
				log("Put input data failed with code: " + str(err));
			}
		}

		os->delay_usec(1_ms);
	}

	dev->break_connection = true;
}

void GRDeviceStandalone::_recv_data(GRDeviceStandalone *dev, Ref<StreamPeerTCP> con) {
	Error err = Error::OK;

	while (!dev->break_connection && con->is_connected_to_host()) {
		uint8_t res[5];
		err = con->get_data(res, 5);

		if (err)
			continue;

		if (validate_packet(res)) {
			PacketType type = (PacketType)res[4];

			switch (type) {
				case GRDevice::InitData:
					break;
				case GRDevice::ImageData: {
					uint8_t i_res[4];
					err = con->get_data(i_res, 4);

					if (err != OK) {
						log("Cant get_data image header", LogLevel::LL_Error);
						continue;
					}

					int size = decode_uint32(i_res);

					PoolByteArray data;
					data.resize(size);
					auto dw = data.write();

					err = con->get_data(dw.ptr(), size);
					dw.release();
					if (err != OK) {
						log("Cant get_data image body");
						continue;
					}

					Ref<Image> img;
					img.instance();

					if (img->load_jpg_from_buffer(data) == OK) {
						dev->call_deferred("_update_texture_from_iamge", img);
					}

					break;
				}
				case GRDevice::InputData: {
					break;
				}
				default:
					break;
			}
		} else {
			log("Packet is not a valid!", LogLevel::LL_Error);
		}
	}

	dev->break_connection = true;
}

#define fix(e) (e - offset) / vp_size

#define data_append_defaults()                                                                                       \
	data.append_array(uint322bytes(ie->get_device()));                                                               \
	if (iewm.is_valid()) {                                                                                           \
		data.append((uint8_t)iewm->get_alt() | (uint8_t)iewm->get_shift() << 1 | (uint8_t)iewm->get_control() << 2 | \
					(uint8_t)iewm->get_metakey() << 3 | (uint8_t)iewm->get_command() << 4);                          \
	}                                                                                                                \
	if (iem.is_valid()) {                                                                                            \
		data.append_array(uint322bytes(iem->get_button_mask()));                                                     \
		data.append_array(var2bytes(fix(iem->get_position())));                                                      \
		data.append_array(var2bytes(fix(iem->get_global_position())));                                               \
	}                                                                                                                \
	if (ieg.is_valid()) {                                                                                            \
		data.append_array(var2bytes(fix(ieg->get_position())));                                                      \
	}

PoolByteArray GRDeviceStandalone::_process_input_data(GRDeviceStandalone *dev) {
	PoolByteArray res;

	if (!dev->control_to_show_in->is_inside_tree())
		return res;

	Array collected_input = dev->input_collector->get_collected_input();

	PoolByteArray data;
	PoolVector3Array sensors;

	sensors.append(Input::get_singleton()->get_accelerometer());
	sensors.append(Input::get_singleton()->get_gravity());
	sensors.append(Input::get_singleton()->get_gyroscope());
	sensors.append(Input::get_singleton()->get_magnetometer());

	data.append(InputType::_InputDeviceSensors);
	data.append_array(var2bytes(sensors));
	sensors.resize(0);

	res.append_array(uint322bytes(data.size() + 4));
	res.append_array(data);

	Vector2 vp_size = dev->control_to_show_in->get_size();
	Vector2 offset = dev->control_to_show_in->get_global_position();

	for (int i = 0; i < collected_input.size(); i++) {
		data.resize(0);
		Ref<InputEvent> ie = ((Ref<InputEvent>)collected_input[i]);

		Ref<InputEventMouse> iem = ie;
		Ref<InputEventGesture> ieg = ie;
		Ref<InputEventWithModifiers> iewm = ie;

		if (ie.is_null()) {
			log("InputEvent is null", LogLevel::LL_Error);
			continue;
		}

		{
			Ref<InputEventKey> iek = ie;
			if (iek.is_valid()) {
				data.append(InputType::_InputEventKey);
				data_append_defaults();

				data.append((uint8_t)iek->is_pressed() | (uint8_t)iek->is_echo() << 1);
				data.append_array(uint322bytes(iek->get_scancode()));
				data.append_array(uint322bytes(iek->get_unicode()));

				goto end;
			}
		}

		{
			Ref<InputEventMouseButton> iemb = ie;
			if (iemb.is_valid()) {
				data.append(InputType::_InputEventMouseButton);
				data_append_defaults();

				data.append_array(float2bytes(iemb->get_factor()));
				data.append_array(uint162bytes(iemb->get_button_index()));
				data.append((uint8_t)iemb->is_pressed() | (uint8_t)iemb->is_doubleclick() << 1);

				goto end;
			}
		}

		{
			Ref<InputEventMouseMotion> iemm = ie;
			if (iemm.is_valid()) {
				data.append(InputType::_InputEventMouseMotion);
				data_append_defaults();

				data.append_array(float2bytes(iemm->get_pressure()));
				data.append_array(var2bytes(iemm->get_tilt()));
				data.append_array(var2bytes(fix(iemm->get_relative())));
				data.append_array(var2bytes(fix(iemm->get_speed())));

				goto end;
			}
		}

		{
			Ref<InputEventScreenTouch> iest = ie;
			if (iest.is_valid()) {
				data.append(InputType::_InputEventScreenTouch);
				data_append_defaults();

				data.append(iest->get_index());
				data.append_array(var2bytes(fix(iest->get_position())));
				data.append(iest->is_pressed());

				goto end;
			}
		}

		{
			Ref<InputEventScreenDrag> iesd = ie;
			if (iesd.is_valid()) {
				data.append(InputType::_InputEventScreenDrag);
				data_append_defaults();

				data.append(iesd->get_index());
				data.append_array(var2bytes(fix(iesd->get_position())));
				data.append_array(var2bytes(fix(iesd->get_relative())));
				data.append_array(var2bytes(fix(iesd->get_speed())));

				goto end;
			}
		}

		{
			Ref<InputEventMagnifyGesture> iemg = ie;
			if (iemg.is_valid()) {
				data.append(InputType::_InputEventMagnifyGesture);
				data_append_defaults();

				data.append_array(float2bytes(iemg->get_factor()));

				goto end;
			}
		}

		{
			Ref<InputEventPanGesture> iepg = ie;
			if (iepg.is_valid()) {
				data.append(InputType::_InputEventPanGesture);
				data_append_defaults();

				data.append_array(var2bytes(fix(iepg->get_delta())));

				goto end;
			}
		}

		{
			Ref<InputEventJoypadButton> iejb = ie;
			if (iejb.is_valid()) {
				data.append(InputType::_InputEventJoypadButton);
				data_append_defaults();

				data.append_array(uint322bytes(iejb->get_button_index()));
				data.append_array(float2bytes(iejb->get_pressure()));
				data.append(iejb->is_pressed());

				goto end;
			}
		}

		{
			Ref<InputEventJoypadMotion> iejm = ie;
			if (iejm.is_valid()) {
				data.append(InputType::_InputEventJoypadMotion);
				data_append_defaults();

				data.append_array(uint322bytes(iejm->get_axis()));
				data.append_array(float2bytes(iejm->get_axis_value()));

				goto end;
			}
		}

		{
			Ref<InputEventAction> iea = ie;
			if (iea.is_valid()) {
				data.append(InputType::_InputEventAction);
				data_append_defaults();

				PoolByteArray sd = var2bytes((String)iea->get_action());

				data.append_array(uint322bytes(sd.size()));
				data.append_array(sd);
				data.append_array(float2bytes(iea->get_strength()));
				data.append(iea->is_pressed());

				goto end;
			}
		}

		{
			Ref<InputEventMIDI> iemidi = ie;
			if (iemidi.is_valid()) {
				data.append(InputType::_InputEventMIDI);
				data_append_defaults();

				PoolIntArray midi;
				midi.append(iemidi->get_channel());
				midi.append(iemidi->get_message());
				midi.append(iemidi->get_pitch());
				midi.append(iemidi->get_velocity());
				midi.append(iemidi->get_instrument());
				midi.append(iemidi->get_pressure());
				midi.append(iemidi->get_controller_number());
				midi.append(iemidi->get_controller_value());

				data.append_array(var2bytes(midi));

				goto end;
			}
		}

		log("InputEvent type " + str(ie) + " not supported!", LogLevel::LL_Error);
		continue;

	end:

		res.append_array(uint322bytes(data.size() + 4)); // block size
		res.append_array(data);
	}

	return res;
}

#undef fix
#undef data_append_defaults

void GRDeviceStandalone::_thread_send_recv_data(void *p_userdata) {
	StartThreadArgs *args = (StartThreadArgs *)p_userdata;
	GRDeviceStandalone *dev = args->dev;
	bool is_recv = args->is_recv;
	delete args;

	Ref<StreamPeerTCP> con = is_recv ? dev->peer_recv : dev->peer_send;
	OS *os = OS::get_singleton();
	String type = is_recv ? "Receive" : "Send";

	while (!dev->stop_device) {
		if (con->get_status() == StreamPeerTCP::STATUS_CONNECTED || con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			con->disconnect_from_host();
		}

		String address = (String)dev->server_address + ":" + str(dev->port);
		Error err = con->connect_to_host(dev->server_address, dev->port);

		if (err == OK) {
			log(type + " thread connecting to " + address);
		} else {
			switch (err) {
				case FAILED:
					log("Failed to open " + type.to_lower() + " socket or can't connect to host", LogLevel::LL_Error);
					break;
				case ERR_UNAVAILABLE:
					log(type + " socket is unavailable", LogLevel::LL_Error);
					break;
				case ERR_INVALID_PARAMETER:
					log("Host address is invalid", LogLevel::LL_Error);
					break;
				case ERR_ALREADY_EXISTS:
					log(type + " socket already in use", LogLevel::LL_Error);
					break;
			}
			os->delay_usec(250_ms);
			continue;
		}

		while (con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			os->delay_usec(1_ms);
		}

		if (con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			log(type + " thread timeout! Connection to " + address + " failed.");
			os->delay_usec(50_ms);
			continue;
		}

		con->set_no_delay(true);
		dev->break_connection = false;

		if (_auth_on_server(con, is_recv)) {
			log(type + " thread successful connected to " + address);
			if (is_recv)
				_recv_data(dev, con);
			else
				_send_data(dev, con);
		}

		if (con->is_connected_to_host())
			con->disconnect_from_host();
	}

	if (con.is_valid() && con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
		con.unref();
	}
}

//////////////////////////////////////////////
///////////// INPUT COLLECTOR ////////////////
//////////////////////////////////////////////

void GRInputCollector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_input", "input_event"), &GRInputCollector::_input);

	ClassDB::bind_method(D_METHOD("is_capture_on_focus"), &GRInputCollector::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD("set_capture_on_focus", "value"), &GRInputCollector::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD("is_capture_when_hover"), &GRInputCollector::is_capture_when_hover);
	ClassDB::bind_method(D_METHOD("set_capture_when_hover", "value"), &GRInputCollector::set_capture_when_hover);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), "set_capture_on_focus", "is_capture_on_focus");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), "set_capture_when_hover", "is_capture_when_hover");
}

void GRInputCollector::_input(Ref<InputEvent> ie) {
	if ((capture_only_when_control_in_focus && parent && !parent->has_focus()) || (grdev && !grdev->is_working())) {
		return;
	}

	if (collected_input.size() > 200) {
		collected_input.resize(0);
	}

	Rect2 rect = Rect2(parent->get_global_position(), parent->get_size());

	{
		Ref<InputEventMouseButton> iemb = ie;
		if (iemb.is_valid()) {
			if (!rect.has_point(iemb->get_position()) && capture_pointer_only_when_hover_control) {
				int idx = iemb->get_button_index();
				if (idx == BUTTON_WHEEL_UP || idx == BUTTON_WHEEL_DOWN ||
						idx == BUTTON_WHEEL_LEFT || idx == BUTTON_WHEEL_RIGHT) {
					return;
				} else {
					if (iemb->is_pressed())
						return;
				}
			}
			goto end;
		}
	}

	{
		Ref<InputEventMouseMotion> iemm = ie;
		if (iemm.is_valid()) {
			if (!rect.has_point(iemm->get_position()) && capture_pointer_only_when_hover_control)
				return;
			goto end;
		}
	}

	{
		Ref<InputEventScreenTouch> iest = ie;
		if (iest.is_valid()) {
			if (!rect.has_point(iest->get_position()) && capture_pointer_only_when_hover_control) {
				if (iest->is_pressed())
					return;
			}
			goto end;
		}
	}

	{
		Ref<InputEventScreenDrag> iesd = ie;
		if (iesd.is_valid()) {
			if (!rect.has_point(iesd->get_position()) && capture_pointer_only_when_hover_control)
				return;
			goto end;
		}
	}

	{
		Ref<InputEventMagnifyGesture> iemg = ie;
		if (iemg.is_valid()) {
			if (!rect.has_point(iemg->get_position()) && capture_pointer_only_when_hover_control)
				return;
			goto end;
		}
	}

	{
		Ref<InputEventPanGesture> iepg = ie;
		if (iepg.is_valid()) {
			if (!rect.has_point(iepg->get_position()) && capture_pointer_only_when_hover_control)
				return;
			goto end;
		}
	}

end:

	collected_input.append(ie);
}

void GRInputCollector::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE: {
			parent = cast_to<Control>(get_parent());
		}
		default:
			break;
	}
}

bool GRInputCollector::is_capture_on_focus() {
	return capture_only_when_control_in_focus;
}

void GRInputCollector::set_capture_on_focus(bool value) {
	capture_only_when_control_in_focus = value;
}

bool GRInputCollector::is_capture_when_hover() {
	return capture_pointer_only_when_hover_control;
}

void GRInputCollector::set_capture_when_hover(bool value) {
	capture_pointer_only_when_hover_control = value;
}

void GRInputCollector::set_gr_device(GRDeviceStandalone *dev) {
	grdev = dev;
}

Array GRInputCollector::get_collected_input() {
	Array res = collected_input.duplicate();
	collected_input.clear();
	return res;
}

GRInputCollector::GRInputCollector() {
	set_process_input(true);
}

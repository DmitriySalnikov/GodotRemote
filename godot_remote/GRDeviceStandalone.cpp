/* GRDeviceStandalone.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRDeviceStandalone.h"
#include "GRResources.h"
#include "GodotRemote.h"
#include "core/input_map.h"
#include "core/io/tcp_server.h"
#include "core/os/input_event.h"
#include "core/os/thread_safe.h"
#include "main/input_default.h"
#include "modules/regex/regex.h"
#include "scene/gui/control.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"

using namespace GRUtils;

void GRDeviceStandalone::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_texture_from_iamge", "image"), &GRDeviceStandalone::_update_texture_from_iamge);
	ClassDB::bind_method(D_METHOD("set_control_to_show_in", "control_node", "position_in_node"), &GRDeviceStandalone::set_control_to_show_in, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_texture", "texture"), &GRDeviceStandalone::set_custom_no_signal_texture);
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_material", "material"), &GRDeviceStandalone::set_custom_no_signal_material);
	ClassDB::bind_method(D_METHOD("set_address", "ip", "port", "ipv4"), &GRDeviceStandalone::set_address, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_ip", "ip", "ipv4"), &GRDeviceStandalone::set_ip, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_ip"), &GRDeviceStandalone::get_ip);

	ADD_SIGNAL(MethodInfo("signal_state_changed", PropertyInfo(Variant::BOOL, "is_connected")));

	// SETGET
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

	ip_validator.instance();
	ip_validator->compile(ip_validator_pattern);

	set_name("GodotRemoteClient");
	ips = new ImgProcessingStorage(this);
	peer.instance();
	port = GLOBAL_GET("debug/godot_remote/general/port");

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	Ref<Image> no_signal_image;
	no_signal_image.instance();
	no_signal_texture.instance();
	GetPoolVectorFromBin(tmp_no_signal, GRResources::Bin_NoSignalPNG);
	no_signal_image->load_png_from_buffer(tmp_no_signal);
	no_signal_texture->create_from_image(no_signal_image);

	Ref<Shader> shader;
	shader.instance();
	shader->set_code(GRResources::Txt_CRT_Shader);
	no_signal_mat.instance();
	no_signal_mat->set_shader(shader);
#endif
}

GRDeviceStandalone::~GRDeviceStandalone() {
	stop();
	delete ips;

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		tex_shows_stream->queue_delete();
		tex_shows_stream = nullptr;
	}
	if (input_collector && !input_collector->is_queued_for_deletion()) {
		input_collector->queue_delete();
		input_collector = nullptr;
	}
}

void GRDeviceStandalone::set_control_to_show_in(Control *ctrl, int position_in_node) {
	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		tex_shows_stream->queue_delete();
		tex_shows_stream = nullptr;
	}
	if (input_collector && !input_collector->is_queued_for_deletion()) {
		input_collector->queue_delete();
		input_collector = nullptr;
	}

	control_to_show_in = ctrl;

	if (control_to_show_in && !control_to_show_in->is_queued_for_deletion()) {
		tex_shows_stream = memnew(TextureRect);
		input_collector = memnew(GRInputCollector);

		tex_shows_stream->set_name("GodotRemoteStreamSprite");
		input_collector->set_name("GodotRemoteInputCollector");

		tex_shows_stream->set_expand(true);
		tex_shows_stream->set_anchor(MARGIN_RIGHT, 1.f);
		tex_shows_stream->set_anchor(MARGIN_BOTTOM, 1.f);

		prev_signal_connection_state = true; // force execute update function
		_update_stream_texture_state(false);

		control_to_show_in->add_child(tex_shows_stream);
		control_to_show_in->move_child(tex_shows_stream, position_in_node);
		control_to_show_in->add_child(input_collector);

		input_collector->set_capture_on_focus(capture_only_when_control_in_focus);
	}
}

void GRDeviceStandalone::set_custom_no_signal_texture(Ref<Texture> custom_tex) {
	custom_no_signal_texture = custom_tex;
}

void GRDeviceStandalone::set_custom_no_signal_material(Ref<Material> custom_mat) {
	custom_no_signal_material = custom_mat;
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

String GRDeviceStandalone::get_ip() {
	return (String)server_address;
}

void GRDeviceStandalone::set_ip(String ip, bool ipv4) {
	bool old = is_working();
	String prev = (String)server_address;

	if (old)
		stop();

	bool is_invalid = false;

	if (ipv4) {
		auto res = ip_validator->search(ip);
		if (res.is_null()) {
			is_invalid = true;
			goto end;
		}
	}

	server_address = ip;

end:
	if (!server_address.is_valid() || is_invalid) {
		ERR_PRINT(ip + " is an invalid address!");
		log(ip + " is an invalid address!");
		server_address = IP_Address(prev);
	}

	if (old)
		start();
}

void GRDeviceStandalone::set_address(String ip, uint16_t _port, bool ipv4) {
	bool old = is_working();
	if (old)
		stop();
	String prev = (String)server_address;
	int prevPort = port;

	bool is_invalid = false;

	if (ipv4) {
		auto res = ip_validator->search(ip);
		if (res.is_null()) {
			is_invalid = true;
			goto end;
		}
	}

	server_address = ip;
	port = _port;

end:
	if (!server_address.is_valid() || is_invalid) {
		ERR_PRINT(ip + " is an invalid address!");
		log(ip + " is an invalid address!");
		server_address = IP_Address(prev);
		port = prevPort;
	}

	if (old)
		start();
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

	thread_connection = Thread::create(&_thread_connection, new StartThreadArgs(this));
	thread_image_decoder = Thread::create(&_thread_image_decoder, ips);
	return true;
}

void GRDeviceStandalone::stop() {
	if (!working)
		return;

	log("Stopping GodotRemote client");

	working = false;
	stop_device = true;
	break_connection = true;
	_update_stream_texture_state(false);

	if (thread_connection) {
		Thread::wait_to_finish(thread_connection);
	}
	if (thread_image_decoder) {
		Thread::wait_to_finish(thread_image_decoder);
	}
	thread_connection = nullptr;
	thread_image_decoder = nullptr;
}

void GRDeviceStandalone::_update_texture_from_iamge(Ref<Image> img) {
	Ref<ImageTexture> tex;
	tex.instance();
	tex->create_from_image(img);

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion())
		tex_shows_stream->set_texture(tex);
}

void GRDeviceStandalone::_update_stream_texture_state(bool is_has_signal) {
	if (prev_signal_connection_state == is_has_signal)
		return;

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		if (is_has_signal) {
			prev_signal_connection_state = true;
			emit_signal("signal_state_changed", true);
			tex_shows_stream->set_material(nullptr);
		} else {
			prev_signal_connection_state = false;
			emit_signal("signal_state_changed", false);
			if (custom_no_signal_texture.is_valid()) {
				tex_shows_stream->set_texture(custom_no_signal_texture);
			}
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
			else {
				tex_shows_stream->set_texture(no_signal_texture);
				tex_shows_stream->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			}
#endif
			if (custom_no_signal_material.is_valid()) {
				tex_shows_stream->set_material(custom_no_signal_material);
			}
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
			else {
				tex_shows_stream->set_material(no_signal_mat);
			}
#endif
		}
	}
}

//////////////////////////////////////////////
////////////////// STATIC ////////////////////
//////////////////////////////////////////////

void GRDeviceStandalone::_thread_connection(void *p_userdata) {
	StartThreadArgs *args = (StartThreadArgs *)p_userdata;
	GRDeviceStandalone *dev = args->dev;
	delete args;

	Ref<StreamPeerTCP> con = dev->peer;
	OS *os = OS::get_singleton();
	Thread::set_name("GRemote_connection");

	while (!dev->stop_device) {
		if (os->get_ticks_msec() - dev->prev_valid_connection_time > 1000) {
			dev->_update_stream_texture_state(false);
		}

		if (con->get_status() == StreamPeerTCP::STATUS_CONNECTED || con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			con->disconnect_from_host();
		}

		IP_Address ip = GodotRemote::get_singleton()->get_connection_type() == GodotRemote::ConnectionType::CONNECTION_ADB ? IP_Address("127.0.0.1") : dev->server_address;

		String address = (String)ip + ":" + str(dev->port);
		Error err = con->connect_to_host(ip, dev->port);

		if (err == OK) {
			log("Connecting to " + address);
		} else {
			switch (err) {
				case FAILED:
					log("Failed to open socket or can't connect to host", LogLevel::LL_Error);
					break;
				case ERR_UNAVAILABLE:
					log("Socket is unavailable", LogLevel::LL_Error);
					break;
				case ERR_INVALID_PARAMETER:
					log("Host address is invalid", LogLevel::LL_Error);
					break;
				case ERR_ALREADY_EXISTS:
					log("Socket already in use", LogLevel::LL_Error);
					break;
			}
			os->delay_usec(250_ms);
			continue;
		}

		while (con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			os->delay_usec(1_ms);
		}

		if (con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			log("Timeout! Connect to " + address + " failed.");
			os->delay_usec(50_ms);
			continue;
		}

		con->set_no_delay(true);
		dev->break_connection = false;

		if (_auth_on_server(con)) {
			log("Successful connected to " + address);
			dev->_update_stream_texture_state(true);
			_connection_loop(dev, con);
		}

		if (con->is_connected_to_host())
			con->disconnect_from_host();
	}

	if (con.is_valid() && con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
		con.unref();
	}
}

void GRDeviceStandalone::_connection_loop(GRDeviceStandalone *dev, Ref<StreamPeerTCP> con) {
	ImgProcessingStorage *ips = dev->ips;
	OS *os = OS::get_singleton();
	Error err = Error::OK;

	uint32_t prev_time = os->get_ticks_msec();
	while (!dev->break_connection && con->is_connected_to_host()) {
		uint32_t time = os->get_ticks_msec();
		dev->prev_valid_connection_time = time;

		// SENDING
		if ((time - prev_time) > (1000 / dev->send_data_fps)) {
			prev_time = time;

			PoolByteArray d = _process_input_data(dev);

			if (d.size() == 0)
				goto end_send;

			PoolByteArray data;
			data.append(PacketType::InputData);
			data.append_array(uint322bytes(d.size()));
			data.append_array(d);

			auto r = data.read();
			Error err = con->put_data(r.ptr(), data.size());
			r.release();

			if (err) {
				log("Put input data failed with code: " + str(err));
			}
		}
	end_send:

		// RECEIVING
		if (con->get_available_bytes()) {
			uint8_t res;
			err = con->get_data(&res, 1);

			if (err)
				goto end_recv;

			PacketType type = (PacketType)res;

			switch (type) {
				case GRDevice::InitData:
					break;
				case GRDevice::ImageData: {
					uint8_t i_res[4];
					err = con->get_data(i_res, 4);

					if (err != OK) {
						log("Cant get_data image header", LogLevel::LL_Error);
						goto end_recv;
					}

					int size = decode_uint32(i_res);

					PoolByteArray data;
					data.resize(size);
					auto dw = data.write();

					err = con->get_data(dw.ptr(), size);
					dw.release();
					if (err != OK) {
						log("Cant get_data image body");
						goto end_recv;
					}
					if (data.size() == 0) {
						log("Received image with zero size!");
						goto end_recv;
					}

					if (!ips->is_new_data) {
						ips->tex_data = data;
						ips->is_new_data = true;
					}
					break;
				}
				case GRDevice::InputData: {
					break;
				}
				default:
					break;
			}
		}
	end_recv:

		os->delay_usec(1_ms);
	}

	dev->break_connection = true;
}

void GRDeviceStandalone::_thread_image_decoder(void *p_userdata) {
	Thread::set_name("GR_image_encoding");

	ImgProcessingStorage *ips = (ImgProcessingStorage *)p_userdata;
	GRDeviceStandalone *dev = ips->dev;
	OS *os = OS::get_singleton();

	while (!dev->stop_device) {
		if (ips->is_new_data) {
			Ref<Image> img;
			img.instance();

			if (img->load_jpg_from_buffer(ips->tex_data) == OK) {
				dev->call_deferred("_update_texture_from_iamge", img);
			}
			ips->is_new_data = false;
		}
		os->delay_usec(1_ms);
	}
}

bool GRDeviceStandalone::_auth_on_server(Ref<StreamPeerTCP> con) {
	// Auth Packet
	// 4bytes GodotRemote Header
	// 4bytes Body Size
	// 3byte  Version
	// ....
	//
	// total body size = 3
	//
	// must receive
	// 4bytes GodotRemote Header
	// 1byte  error code

	PoolByteArray data;
	data.append_array(get_packet_header()); // header

	/* everything after "body size" */
	PoolByteArray auth;
	auth.append_array(get_version()); // version

	/* finalize */
	data.append_array(int322bytes(auth.size())); // body size
	data.append_array(auth);

	auto r = data.read();
	Error err = con->put_data(r.ptr(), data.size());
	r.release();

	if (err == OK) {
		PoolByteArray h;
		h.resize(5);
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
					log("Can't connect to server. Version mismatch.");
					return false;
			}
		}
	} else {
		return false;
	}

	return false;
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
	PoolVector3Array sensors = dev->input_collector->get_sensors();

	PoolByteArray data;
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
	if (!parent || (capture_only_when_control_in_focus && !parent->has_focus()) || (grdev && !grdev->is_working())) {
		return;
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

	if (collected_input.size() > 200) {
		collected_input.resize(0);
	}

	collected_input.append(ie);

	sensors[0] = Input::get_singleton()->get_accelerometer();
	sensors[1] = Input::get_singleton()->get_gravity();
	sensors[2] = Input::get_singleton()->get_gyroscope();
	sensors[3] = Input::get_singleton()->get_magnetometer();
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

PoolVector3Array GRInputCollector::get_sensors() {
	PoolVector3Array res;
	res.append_array(sensors);
	return res;
}

GRInputCollector::GRInputCollector() {
	parent = nullptr;
	sensors.resize(4);
	set_process_input(true);
}

GRInputCollector::~GRInputCollector() {
}

#endif // !NO_GODOTREMOTE_CLIENT

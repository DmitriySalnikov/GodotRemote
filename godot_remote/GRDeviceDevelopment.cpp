/* GRDeviceDevelopment.cpp */
#include "GRDeviceDevelopment.h"
#include "GodotRemote.h"
#include "core/input_map.h"
#include "core/os/input_event.h"
#include "core/os/thread_safe.h"
#include "main/input_default.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include <memory>

using namespace GRUtils;

void GRDeviceDevelopment::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_settings_node"), &GRDeviceDevelopment::get_settings_node);
	ClassDB::bind_method(D_METHOD("get_gr_viewport"), &GRDeviceDevelopment::get_gr_viewport);

	//BIND_ENUM_CONSTANT(InitData);
}

void GRDeviceDevelopment::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_CRASH:
		case NOTIFICATION_EXIT_TREE:
		case NOTIFICATION_PREDELETE: {
			stop();
			break;
		}
	}
}

GRDeviceDevelopment::GRDeviceDevelopment() :
		GRDevice() {
	set_name("GodotRemoteServer");
	tcp_server.instance();
}

GRDeviceDevelopment::~GRDeviceDevelopment() {
	stop();
}

bool GRDeviceDevelopment::start() {
	GRDevice::start();

	if (server_thread_listen) {
		ERR_FAIL_V_MSG(false, "Can't start already working Godot Remote Server");
	}

	log("Starting GodotRemote server");

	working = true;
	stop_device = false;
	tcp_server->listen(port);
	server_thread_listen = Thread::create(&_thread_listen, this);

	if (!resize_viewport) {
		resize_viewport = new GRDDViewport();
		add_child(resize_viewport);
	}

	return true;
}

void GRDeviceDevelopment::stop() {
	if (!working)
		return;

	working = false;
	stop_device = true;
	break_connection = true;

	if (server_thread_listen) {
		Thread::wait_to_finish(server_thread_listen);
	}
	//delete server_thread_listen;
	server_thread_listen = nullptr;

	if (resize_viewport && !resize_viewport->is_queued_for_deletion()) {
		remove_child(resize_viewport);
		resize_viewport->queue_delete();
		resize_viewport = nullptr;
	}
}

GRDDViewport *GRDeviceDevelopment::get_gr_viewport() {
	return resize_viewport;
}

Node *GRDeviceDevelopment::get_settings_node() {
	return settings_menu_node;
}

GRDeviceDevelopment::AuthResult GRDeviceDevelopment::_auth_client(GRDeviceDevelopment *dev, Ref<StreamPeerTCP> con) {
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
	data.resize(8);
	auto w = data.write();
	Error err = con->get_data(w.ptr(), 8);
	w.release();

	if (err == OK) {
		auto r = data.read();
		if (!validate_packet(r.ptr()))
			return AuthResult::Error;

		int size = bytes2int32(r.ptr() + 4);
		r.release();

		if (size != 4) // body size must be equal to 4
			return AuthResult::Error;

		PoolByteArray body;
		body.resize(size);
		w = body.write();

		err = con->get_data(w.ptr(), size);
		if (err != OK)
			return AuthResult::Error;
		w.release();

		PoolByteArray ret;
		ret.append_array(get_packet_header());

		r = body.read();
		if (!validate_version(r.ptr())) {
			ret.append((int)AuthErrorCode::VersionMismatch);
			auto rr = ret.read();
			con->put_data(rr.ptr(), ret.size());
			return AuthResult::VersionMismatch;
		}

		bool is_recv = r[3];
		r.release();

		// send back final validation packet
		ret.append((int)AuthErrorCode::OK);
		r = ret.read();
		con->put_data(r.ptr(), ret.size());
		r.release();

		if (is_recv)
			return AuthResult::RecvClient;
		else
			return AuthResult::SendClient;
	}

	return AuthResult::Error;
}

void GRDeviceDevelopment::_process_input_data(const PoolByteArray &p_data) {
	InputDefault *id = (InputDefault *)Input::get_singleton();
	InputMap *im = InputMap::get_singleton();
	OS *os = OS::get_singleton();

	auto data_r = p_data.read();
	const uint8_t *data = data_r.ptr();
	const int size = p_data.size();

	Vector2 vp_size = SceneTree::get_singleton()->get_root()->get_visible_rect().get_size();

	const uint8_t *end_offset = data + size;
	while (data < end_offset) {
		Ref<InputEvent> ev;

		int length = decode_uint32(data); // block size
		const uint8_t *next = data + length;
		InputType type = (InputType)data[4];
		data += 5;

		switch (type) {
			case GRDevice::_InputDeviceSensors: {
				PoolVector3Array vecs = bytes2var(data, 12 * 4 + 8);

				if (id && vecs.size() == 4) {
					id->set_accelerometer(vecs[0]);
					id->set_gravity(vecs[1]);
					id->set_gyroscope(vecs[2]);
					id->set_magnetometer(vecs[3]);
				}

				break;
			}
			case GRDevice::_InputEventAction: {
				Ref<InputEventAction> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				int len = decode_uint32(data);
				e->set_action(String::utf8((const char *)data + 4, len));
				data += int64_t(4) + len;

				if (im->has_action(e->get_action())) {
					e->set_strength(decode_float(data));
					e->set_pressed(*(data + 1));
					ev = e;
				}
				break;
			}
			case GRDevice::_InputEventJoypadButton: {
				Ref<InputEventJoypadButton> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_button_index(decode_uint32(data));
				e->set_pressure(decode_float(data + 4));
				e->set_pressed(*(data + 8));
				ev = e;
				break;
			}
			case GRDevice::_InputEventJoypadMotion: {
				Ref<InputEventJoypadMotion> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_axis(decode_uint32(data));
				e->set_axis_value(decode_float(data + 4));
				ev = e;
				break;
			}
			case GRDevice::_InputEventKey: {
				Ref<InputEventKey> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_pressed((*(data)) & 1);
				e->set_echo((*(data) >> 1) & 1);
				e->set_scancode(decode_uint32(data + 1));
				e->set_unicode(decode_uint32(data + 5));
				ev = e;
				break;
			}
			case GRDevice::_InputEventMagnifyGesture: {
				Ref<InputEventMagnifyGesture> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_factor(decode_float(data));
				ev = e;
				break;
			}
			case GRDevice::_InputEventMIDI: {
				Ref<InputEventMIDI> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				PoolIntArray p = bytes2var(data, 4 * 8 + 8);
				e->set_channel(p[0]);
				e->set_message(p[1]);
				e->set_pitch(p[2]);
				e->set_velocity(p[3]);
				e->set_instrument(p[4]);
				e->set_pressure(p[5]);
				e->set_controller_number(p[6]);
				e->set_controller_value(p[7]);
				ev = e;
				break;
			}
			case GRDevice::_InputEventMouseButton: {
				Ref<InputEventMouseButton> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_factor(decode_float(data));
				e->set_button_index(decode_uint16(data + 4));
				e->set_pressed((*(data + 6)) & 1);
				e->set_doubleclick((*(data + 6) >> 1) & 1);
				ev = e;
				break;
			}
			case GRDevice::_InputEventMouseMotion: {
				Ref<InputEventMouseMotion> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_pressure(decode_float(data));
				e->set_tilt(bytes2var<Vector2>(data + 4, 12));
				e->set_relative(bytes2var<Vector2>(data + 16, 12) * vp_size);
				e->set_speed(bytes2var<Vector2>(data + 28, 12) * vp_size);
				ev = e;
				break;
			}
			case GRDevice::_InputEventPanGesture: {
				Ref<InputEventPanGesture> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_delta(bytes2var<Vector2>(data, 12) * vp_size);
				ev = e;
				break;
			}
			case GRDevice::_InputEventScreenDrag: {
				Ref<InputEventScreenDrag> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_index(*(data));
				e->set_position(bytes2var<Vector2>(data + 1, 12) * vp_size);
				e->set_relative(bytes2var<Vector2>(data + 13, 12) * vp_size);
				e->set_speed(bytes2var<Vector2>(data + 25, 12) * vp_size);
				ev = e;
				break;
			}
			case GRDevice::_InputEventScreenTouch: {
				Ref<InputEventScreenTouch> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_index(*(data));
				e->set_position(bytes2var<Vector2>(data + 1, 12) * vp_size);
				e->set_pressed(*(data + 13));
				ev = e;
				break;
			}
			default: {
				log(String("Not supported InputEvent type: ") + String::num_int64(type));
				break;
			}
		}

		data = next;
		if (ev.is_valid()) {
			id->call_deferred("parse_input_event", ev);
		}
	}
}

const uint8_t *GRDeviceDevelopment::_read_abstract_input_data(InputEvent *ie, const Vector2 vs, const uint8_t *data) {
	ie->set_device(decode_uint32(data));
	// TODO add ability to set custom device id or just -1/0 if we emulating input.
	data += 4;

	auto iewm = cast_to<InputEventWithModifiers>(ie);
	if (iewm) {
		iewm->set_alt(*data & (1 << 0));
		iewm->set_shift(*data & (1 << 1));
		iewm->set_control(*data & (1 << 2));
		iewm->set_metakey(*data & (1 << 3));
		iewm->set_command(*data & (1 << 4));
		data += 1;
	}

	auto iem = cast_to<InputEventMouse>(ie);
	if (iem) {
		iem->set_button_mask(decode_uint32(data));
		iem->set_position(vs * bytes2var<Vector2>(data + 4, 12));
		iem->set_global_position(vs * bytes2var<Vector2>(data + 16, 12));
		data += 28;
	}

	auto ieg = cast_to<InputEventGesture>(ie);
	if (ieg) {
		ieg->set_position(vs * bytes2var<Vector2>(data, 12));
		data += 12;
	}

	return data;
}

//////////////////////////////////////////////
////////////////// THREADS ///////////////////
//////////////////////////////////////////////

void GRDeviceDevelopment::_thread_listen(void *p_userdata) {
	Thread::set_name("GR_listen_thread");
	GRDeviceDevelopment *dev = (GRDeviceDevelopment *)p_userdata;
	Ref<TCP_Server> srv = dev->tcp_server;
	Thread *t_send_data = nullptr;
	Thread *t_recieve_input = nullptr;
	OS *os = OS::get_singleton();
	bool connectedRecv = false;
	bool connectedSend = false;
	Array connections;

	while (!dev->stop_device && srv->is_listening()) {
		for (int i = connections.size() - 1; i >= 0; i--) {
			Ref<StreamPeerTCP> con = connections[i];
			if (con.is_valid()) {
				if (con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
					dev->break_connection = true;
					connections.remove(i);
				}
			}
		}

		if (dev->break_connection) {
			if (t_send_data) {
				Thread::wait_to_finish(t_send_data);
				t_send_data = nullptr;
			}
			if (t_recieve_input) {
				Thread::wait_to_finish(t_recieve_input);
				t_recieve_input = nullptr;
			}
			connectedRecv = false;
			connectedSend = false;
		}

		if (srv->is_connection_available()) {
			if (!connectedRecv || !connectedSend) {
				Ref<StreamPeerTCP> con = srv->take_connection();
				con->set_no_delay(true);
				dev->break_connection = false;

				String address = str(con->get_connected_host()) + ":" + str(con->get_connected_port());

				AuthResult res = _auth_client(dev, con);
				switch (res) {
					case AuthResult::Error:
						log("Refusing connection. Wrong client.");
						con->disconnect_from_host();
						os->delay_usec(16_ms);
						continue;
					case AuthResult::VersionMismatch:
						log("Refusing connection. Version mismatch.");
						con->disconnect_from_host();
						os->delay_usec(16_ms);
						continue;
					case AuthResult::SendClient:
						if (!connectedSend) {
							t_recieve_input = Thread::create(&_thread_recieve_input, new StartThreadArgs(dev, con));
							log("New connection from " + address + "(Sending client)");
							connectedSend = true;
							connections.append(con);
						} else {
							log("Refusing connection. Sending client already connected.");
							con->disconnect_from_host();
							os->delay_usec(16_ms);
						}
						break;
					case AuthResult::RecvClient:
						if (!connectedRecv) {
							t_send_data = Thread::create(&_thread_send_data, new StartThreadArgs(dev, con));
							log("New connection from " + address + "(Receiving client)");
							connectedRecv = true;
							connections.append(con);
						} else {
							log("Refusing connection. Receiving client already connected.");
							con->disconnect_from_host();
							os->delay_usec(16_ms);
						}
						break;
					default:
						break;
				}

			} else {
				log("Refusing connection. Already connected.");
				srv->take_connection()->disconnect_from_host();
				os->delay_usec(16_ms);
			}
		} else {
			log("Waiting...", LogLevel::LL_Debug);
			os->delay_usec(33_ms);
		}
	}

	dev->break_connection = true;
	dev->stop_device = true;

	if (t_send_data) {
		Thread::wait_to_finish(t_send_data);
		t_send_data = nullptr;
	}
	if (t_recieve_input) {
		Thread::wait_to_finish(t_recieve_input);
		t_recieve_input = nullptr;
	}

	dev->tcp_server->stop();
}

void GRDeviceDevelopment::_thread_send_data(void *p_userdata) {
	StartThreadArgs *args = (StartThreadArgs *)p_userdata;
	Ref<StreamPeerTCP> con = args->con;
	GRDeviceDevelopment *dev = args->dev;
	delete args;

	GodotRemote *gr = GodotRemote::get_singleton();
	OS *os = OS::get_singleton();
	Error err = Error::OK;
	String address = str(con->get_connected_host()) + ":" + str(con->get_connected_port());
	Thread::set_name("GR_server_send " + address);

	uint32_t prev_time = os->get_ticks_msec();

	while (!dev->break_connection && con->get_status() == StreamPeerTCP::STATUS_CONNECTED) {
		uint32_t time = os->get_ticks_msec();

		if (time - prev_time > (1000 / dev->target_send_fps)) {
			prev_time = time;

			Ref<ViewportTexture> vt = dev->resize_viewport->get_texture();
			if (vt.is_null()) {
				os->delay_usec(1_ms);
				continue;
			}

			Ref<Image> img = vt->get_data();

			if (img.is_null()) {
				os->delay_usec(1_ms);
				continue;
			}

			PoolByteArray viewport_data = gr->compress_jpg(img, 75, 1.0f, GodotRemote::SUBSAMPLING_H2V2);
			int ds = viewport_data.size();
			//log(String::num(ds) + " bytes", LogLevel::LL_Debug);

			if (!ds) {
				log(String("Cant encode image! Code: ") + str(err), LogLevel::LL_Error);
				os->delay_usec(1_ms);
				continue;
			}

			PoolByteArray data;
			data.append_array(get_packet_header());
			data.append(PacketType::ImageData);
			data.append_array(uint322bytes(ds));
			data.append_array(viewport_data);

			auto r = data.read();
			err = con->put_data(r.ptr(), data.size());
			r.release();

			if (err) {
				log(String("Cant send image data! Code: ") + str(err), LogLevel::LL_Error);
				r.release();
				continue;
			}
			r.release();
		} else {
			os->delay_usec(16_ms);
		}
	}

	log("Closing sending thread with address: " + address);

	dev->break_connection = true;
	con->disconnect_from_host();
}

void GRDeviceDevelopment::_thread_recieve_input(void *p_userdata) {
	StartThreadArgs *args = (StartThreadArgs *)p_userdata;
	Ref<StreamPeerTCP> con = args->con;
	GRDeviceDevelopment *dev = args->dev;
	delete args;

	OS *os = OS::get_singleton();
	String address = str(con->get_connected_host()) + ":" + str(con->get_connected_port());
	Error err = Error::OK;
	Thread::set_name("GR_server_recv " + address);

	while (!dev->break_connection && con->get_status() == StreamPeerTCP::STATUS_CONNECTED) {
		uint8_t res[5];
		err = con->get_data(res, 5);

		if (err)
			continue;

		if (validate_packet(res)) {
			PacketType type = (PacketType)res[4];

			switch (type) {
				case GRDevice::InitData:
					break;
				case GRDevice::ImageData:
					break;
				case GRDevice::InputData: {
					uint8_t s_res[8];
					err = con->get_data(s_res, 8);

					if (err) {
						continue;
					}

					const int size = bytes2var(s_res, 8);
					PoolByteArray data;
					data.resize(size);

					auto w = data.write();
					err = con->get_data(w.ptr(), size);
					w.release();

					if (err) {
						continue;
					}

					_process_input_data(data);

					break;
				}
				default:
					break;
			}
		} else {
			log("Packet is not a valid!", LogLevel::LL_Error);
		}
	}

	log("Closing receiving thread with address: " + address);

	dev->break_connection = true;
	con->disconnect_from_host();
}

//////////////////////////////////////////////
////////////// GRDDViewport //////////////////
//////////////////////////////////////////////

void GRDDViewport::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_size"), &GRDDViewport::_update_size);
	ClassDB::bind_method(D_METHOD("set_rendering_scale"), &GRDDViewport::set_rendering_scale);
	ClassDB::bind_method(D_METHOD("get_rendering_scale"), &GRDDViewport::get_rendering_scale);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "rendering_scale", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_rendering_scale", "get_rendering_scale");
}

void GRDDViewport::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE: {
			main_vp = SceneTree::get_singleton()->get_root();
			main_vp->connect("size_changed", this, "_update_size");
			_update_size();

			auto renderer = new GRDDViewportRenderer();
			renderer->tex = main_vp->get_texture();
			add_child(renderer);

			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			main_vp->disconnect("size_changed", this, "_update_size");
			main_vp = nullptr;
			break;
		}
		default:
			break;
	}
}

void GRDDViewport::_update_size() {
	if (main_vp && main_vp->get_texture().is_valid()) {
		Vector2 size = main_vp->get_size() * rendering_scale;

		if (size.x < 8)
			size.x = 8;
		else if (size.x > Image::MAX_WIDTH)
			size.x = Image::MAX_WIDTH;

		if (size.y < 8)
			size.y = 8;
		else if (size.y > Image::MAX_HEIGHT)
			size.y = Image::MAX_HEIGHT;

		set_size(size);
	}
}

void GRDDViewport::set_rendering_scale(float val) {
	rendering_scale = val;
	_update_size();
}

float GRDDViewport::get_rendering_scale() {
	return rendering_scale;
}

GRDDViewport::GRDDViewport() {
	set_name("GRDDViewport");

	set_hdr(false);
	set_disable_3d(true);
	set_keep_3d_linear(true);
	set_usage(Viewport::USAGE_2D);
	set_update_mode(Viewport::UPDATE_ALWAYS);
	set_disable_input(true);
	set_shadow_atlas_quadrant_subdiv(0, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(1, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(2, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(3, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
}

//////////////////////////////////////////////
/////////// GRDDViewportRenderer /////////////
//////////////////////////////////////////////

void GRDDViewportRenderer::_bind_methods() {
}

void GRDDViewportRenderer::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE: {
			vp = get_viewport();
			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			vp = nullptr;
			tex.unref();
			break;
		}
		case NOTIFICATION_PROCESS: {
			update();
			break;
		}
		case NOTIFICATION_DRAW: {
			draw_texture_rect(tex, Rect2(Vector2(), vp->get_size()), false);
			break;
		}
		default:
			break;
	}
}

GRDDViewportRenderer::GRDDViewportRenderer() {
	set_name("GRDDViewportRenderer");
	set_process(true);

	set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
}

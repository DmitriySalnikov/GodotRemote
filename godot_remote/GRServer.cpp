/* GRServer.cpp */

#ifndef NO_GODOTREMOTE_SERVER

#include "GRServer.h"
#include "GRPacket.h"
#include "GodotRemote.h"
#include "core/input_map.h"
#include "core/os/input_event.h"
#include "core/os/thread_safe.h"
#include "main/input_default.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"

using namespace GRUtils;

void GRServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_settings_node"), &GRServer::get_settings_node);
	ClassDB::bind_method(D_METHOD("get_gr_viewport"), &GRServer::get_gr_viewport);

	ClassDB::bind_method(D_METHOD("set_target_send_fps"), &GRServer::set_target_send_fps);
	ClassDB::bind_method(D_METHOD("set_auto_adjust_scale"), &GRServer::set_auto_adjust_scale);
	ClassDB::bind_method(D_METHOD("set_jpg_quality"), &GRServer::set_jpg_quality);
	ClassDB::bind_method(D_METHOD("set_render_scale"), &GRServer::set_render_scale);

	ClassDB::bind_method(D_METHOD("get_target_send_fps"), &GRServer::get_target_send_fps);
	ClassDB::bind_method(D_METHOD("get_auto_adjust_scale"), &GRServer::get_auto_adjust_scale);
	ClassDB::bind_method(D_METHOD("get_jpg_quality"), &GRServer::get_jpg_quality);
	ClassDB::bind_method(D_METHOD("get_render_scale"), &GRServer::get_render_scale);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_send_fps"), "set_target_send_fps", "get_target_send_fps");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_adjust_scale"), "set_auto_adjust_scale", "get_auto_adjust_scale");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "jpg_quality"), "set_jpg_quality", "get_jpg_quality");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "render_scale"), "set_render_scale", "get_render_scale");
}

void GRServer::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_CRASH:
		case NOTIFICATION_EXIT_TREE:
		case NOTIFICATION_PREDELETE: {
			if (get_status() == (int)WorkingStatus::Working) {
				_internal_call_only_deffered_stop();
			}
			break;
		}
	}
}

void GRServer::set_auto_adjust_scale(bool _val) {
	auto_adjust_scale = _val;
}

int GRServer::get_auto_adjust_scale() {
	return auto_adjust_scale;
}

void GRServer::set_jpg_quality(int _quality) {
	ERR_FAIL_COND(_quality < 0 || _quality > 100);
	jpg_quality = _quality;
}

int GRServer::get_jpg_quality() {
	return jpg_quality;
}

void GRServer::set_target_send_fps(int fps) {
	ERR_FAIL_COND(fps <= 0);
	target_send_fps = fps;
}

int GRServer::get_target_send_fps() {
	return target_send_fps;
}

void GRServer::set_render_scale(float _scale) {
	if (resize_viewport)
		resize_viewport->set_rendering_scale(_scale);
}

float GRServer::get_render_scale() {
	if (resize_viewport)
		return resize_viewport->get_rendering_scale();
	return -1;
}

GRServer::GRServer() :
		GRDevice() {
	set_name("GodotRemoteServer");
	tcp_server.instance();
	connection_mutex = Mutex::create();
	ips = new ImgProcessingStorage(this);
	_load_settings();
	init_server_utils();
}

GRServer::~GRServer() {
	if (get_status() == (int)WorkingStatus::Working) {
		_internal_call_only_deffered_stop();
	}
	delete ips;
	connection_mutex->unlock();
	memdelete(connection_mutex);
	deinit_server_utils();
}

void GRServer::_internal_call_only_deffered_start() {
	switch ((WorkingStatus)get_status()) {
		case GRDevice::WorkingStatus::Working:
			ERR_FAIL_MSG("Can't start already working GodotRemote Server");
		case GRDevice::WorkingStatus::Starting:
			ERR_FAIL_MSG("Can't start already starting GodotRemote Server");
		case GRDevice::WorkingStatus::Stopping:
			ERR_FAIL_MSG("Can't start stopping GodotRemote Server");
	}

	set_status(WorkingStatus::Starting);
	_log("Starting GodotRemote server");

	stop_device = false;
	server_thread_listen = Thread::create(&_thread_listen, this);
	t_image_encode = Thread::create(&_thread_image_processing, ips);

	if (!resize_viewport) {
		resize_viewport = memnew(GRSViewport);
		add_child(resize_viewport);
	}
	set_status(WorkingStatus::Working);
}

void GRServer::_internal_call_only_deffered_stop() {
	switch ((WorkingStatus)get_status()) {
		case GRDevice::WorkingStatus::Stopped:
			ERR_FAIL_MSG("Can't stop already stopped GodotRemote Server");
		case GRDevice::WorkingStatus::Stopping:
			ERR_FAIL_MSG("Can't stop already stopping GodotRemote Server");
		case GRDevice::WorkingStatus::Starting:
			ERR_FAIL_MSG("Can't stop starting GodotRemote Server");
	}

	set_status(WorkingStatus::Stopping);
	_log("Stopping GodotRemote server");

	stop_device = true;
	break_connection = true;

	if (server_thread_listen) {
		Thread::wait_to_finish(server_thread_listen);
	}
	if (t_image_encode) {
		Thread::wait_to_finish(t_image_encode);
		t_image_encode = nullptr;
	}

	//delete server_thread_listen;
	server_thread_listen = nullptr;

	if (resize_viewport && !resize_viewport->is_queued_for_deletion()) {
		remove_child(resize_viewport);
		memdelete(resize_viewport);
		//resize_viewport->queue_delete();
		resize_viewport = nullptr;
	}
	set_status(WorkingStatus::Stopped);
}

GRSViewport *GRServer::get_gr_viewport() {
	return resize_viewport;
}

Node *GRServer::get_settings_node() {
	return settings_menu_node;
}

void GRServer::_adjust_viewport_scale() {
	if (!resize_viewport)
		return;

	const float smooth = 0.8f;
	float scale = resize_viewport->auto_scale;
	if (prev_avg_fps == 0) {
		prev_avg_fps = avg_fps;
	} else {
		prev_avg_fps = (prev_avg_fps * smooth) + (avg_fps * (1.f - smooth));
	}

	if (!auto_adjust_scale) {
		scale = -1.f;
		goto end;
	}
	_log(prev_avg_fps);

	if (prev_avg_fps < target_send_fps - 4) {
		scale -= 0.001f;
	} else if (prev_avg_fps > target_send_fps - 1) {
		scale += 0.0012f;
	}

	if (scale < 0.1f)
		scale = 0.1f;
	if (scale > 1)
		scale = 1;

end:
	resize_viewport->auto_scale = scale;
	resize_viewport->_update_size();
}

void GRServer::_load_settings() {
	jpg_quality = GET_PS(GodotRemote::ps_jpg_quality_name);
	auto_adjust_scale = GET_PS(GodotRemote::ps_auto_adjust_scale_name);

	if (resize_viewport && !resize_viewport->is_queued_for_deletion()) {
		resize_viewport->set_rendering_scale(GET_PS(GodotRemote::ps_scale_of_sending_stream_name));
	}
}

void GRServer::_update_settings_from_client(const Dictionary settings) {
	Array keys = settings.keys();

	for (int i = 0; i < settings.size(); i++) {
		Variant key = keys[i];
		Variant value = settings[key];

		if (key.get_type() == Variant::INT) {
			GodotRemote::TypesOfServerSettings k = (GodotRemote::TypesOfServerSettings)(int)key;
			switch (k) {
				case GodotRemote::TypesOfServerSettings::USE_INTERNAL_SERVER_SETTINGS:
					if ((bool)value == true) {
						_load_settings();
						return;
					}
					break;
				case GodotRemote::TypesOfServerSettings::JPG_QUALITY:
					set_jpg_quality(value);
					break;
				case GodotRemote::TypesOfServerSettings::SEND_FPS:
					set_target_send_fps(value);
					break;
				case GodotRemote::TypesOfServerSettings::RENDER_SCALE:
					set_render_scale(value);
					break;
				default:
					_log("Unknown server setting with code: " + str((int)k));
					break;
			}
		}
	}
}

void GRServer::_reset_counters() {
	GRDevice::_reset_counters();
	prev_avg_fps = 0;
}

//////////////////////////////////////////////
////////////////// STATIC ////////////////////
//////////////////////////////////////////////

void GRServer::_thread_listen(void *p_userdata) {
	Thread::set_name("GR_listen_thread");
	GRServer *dev = (GRServer *)p_userdata;
	Ref<TCP_Server> srv = dev->tcp_server;
	Thread *t_connection = nullptr;
	OS *os = OS::get_singleton();
	bool is_client_connected = false;
	Ref<StreamPeerTCP> connection;
	Error err = OK;

	srv->listen(dev->port);
	_log("Start listening port " + str(dev->port));

	while (!dev->stop_device) {
		if (!srv->is_listening()) {
			err = srv->listen(dev->port);
			_log("Start listening port " + str(dev->port));

			if (err != OK) {

				switch (err) {
					case ERR_UNAVAILABLE:
						_log("Socket listening unavailable");
						break;
					case ERR_ALREADY_IN_USE:
						_log("Socket already in use");
						break;
					case ERR_INVALID_PARAMETER:
						_log("Invalid listening address");
						break;
					case ERR_CANT_CREATE:
						_log("Can't bind listener");
						break;
					case FAILED:
						_log("Failed to start listening");
						break;
				}

				os->delay_usec(1000_ms);
				continue;
			}
		}

		if (connection.is_null() || !connection->is_connected_to_host()) {
			dev->break_connection = true;
		}

		if (dev->break_connection) {
			if (t_connection) {
				_log("Waiting connection thread...", LogLevel::LL_Debug);
				Thread::wait_to_finish(t_connection);
				t_connection = nullptr;
			}
			is_client_connected = false;
		}

		if (srv->is_connection_available()) {
			if (!is_client_connected) {
				Ref<StreamPeerTCP> con = srv->take_connection();
				con->set_no_delay(true);
				String address = str(con->get_connected_host()) + ":" + str(con->get_connected_port());

				AuthResult res = _auth_client(dev, con);
				switch (res) {
					case AuthResult::OK:
						dev->break_connection = false;
						is_client_connected = true;

						t_connection = Thread::create(&_thread_connection, new StartThreadArgs(dev, con));
						_log("New connection from " + address);
						connection = con;
						break;
					case AuthResult::Error:
						_log("Refusing connection. Wrong client. " + address);
						con->disconnect_from_host();
						os->delay_usec(16_ms);
						continue;
					case AuthResult::VersionMismatch:
						_log("Refusing connection. Version mismatch. " + address);
						con->disconnect_from_host();
						os->delay_usec(16_ms);
						continue;
					default:
						_log("Unknown error code. Disconnecting." + address);
						con->disconnect_from_host();
						break;
				}

			} else {
				//_log("Refusing connection. Already connected. " + address, LogLevel::LL_Debug);
				//con->disconnect_from_host();
				os->delay_usec(16_ms);
			}
		} else {
			_log("Waiting...", LogLevel::LL_Debug);
			os->delay_usec(33_ms);
		}
	}

	dev->break_connection = true;
	dev->stop_device = true;

	if (t_connection) {
		_log("Stopping device thread", LogLevel::LL_Debug);
		Thread::wait_to_finish(t_connection);
		t_connection = nullptr;
	}

	dev->tcp_server->stop();
}

void GRServer::_thread_connection(void *p_userdata) {
	StartThreadArgs *args = (StartThreadArgs *)p_userdata;
	Ref<StreamPeerTCP> connection = args->con;
	GRServer *dev = args->dev;
	ImgProcessingStorage *ips = dev->ips;
	delete args;

	Ref<PacketPeerStream> ppeer(memnew(PacketPeerStream));
	ppeer->set_stream_peer(connection);
	ppeer->set_output_buffer_max_size(compress_buffer.size());

	GodotRemote *gr = GodotRemote::get_singleton();
	OS *os = OS::get_singleton();
	Error err = Error::OK;

	String address = str(connection->get_connected_host()) + ":" + str(connection->get_connected_port());
	Thread::set_name("GR_connection " + address);

	dev->_reset_counters();

	uint32_t prev_time = os->get_ticks_msec();
	uint32_t prev_ping_sending_time = prev_time;
	uint32_t prev_send_image_time = prev_time;
	bool ping_sended = false;

	while (!dev->break_connection && connection->is_connected_to_host()) {
		bool nothing_happens = true;
		int send_data_time_ms = (1000 / dev->target_send_fps);

		///////////////////////////////////////////////////////////////////
		// SENDING

		// IMAGE
		uint32_t time = os->get_ticks_msec();
		if (time - prev_time > send_data_time_ms && !ips->is_new_data) {
			nothing_happens = false;
			prev_time = time;

			ips->tex = dev->resize_viewport->get_texture();
		}
		// if image compressed to jpg and data is ready
		if (ips->is_new_data) {
			nothing_happens = false;

			Ref<GRPacketImageData> pack(memnew(GRPacketImageData));

			if (ips->ret_data.size() == 0) {
				_log("Cant encode image!", LogLevel::LL_Error);
				ips->is_new_data = false;
				os->delay_usec(1_ms);
				goto end_send;
			}
			pack->set_image_data(ips->ret_data);

			err = ppeer->put_var(pack->get_data());
			ips->is_new_data = false;

			// avg fps
			time = os->get_ticks_msec();
			dev->_update_avg_fps(time - prev_send_image_time);
			dev->_adjust_viewport_scale();
			prev_send_image_time = time;

			if (err) {
				_log("Cant send image data! Code: " + str(err), LogLevel::LL_Error);
				goto end_send;
			}
		}

		// PING
		time = os->get_ticks_msec();
		if ((os->get_ticks_msec() - prev_ping_sending_time) > 100 && !ping_sended) {
			prev_ping_sending_time = time;
			nothing_happens = false;
			ping_sended = true;

			Ref<GRPacketPing> pack(memnew(GRPacketPing));
			err = ppeer->put_var(pack->get_data());

			if (err) {
				_log("Send ping failed with code: " + str(err), LogLevel::LL_Error);
				goto end_send;
			}
		}
	end_send:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after sending!", LogLevel::LL_Error);
			continue;
		}

		///////////////////////////////////////////////////////////////////
		// RECEIVING
		uint32_t recv_start_time = os->get_ticks_msec();
		while (ppeer->get_available_packet_count() > 0 && (os->get_ticks_msec() - recv_start_time) < send_data_time_ms) {
			nothing_happens = false;
			Variant res;
			err = ppeer->get_var(res);

			if (err) {
				_log("Can't receive packet!", LogLevel::LL_Error);
				goto end_recv;
			}

			//_log(str_arr((PoolByteArray)res, true));
			Ref<GRPacket> pack = GRPacket::create(res);
			if (pack.is_null()) {
				_log("Received packet was NULL", LogLevel::LL_Error);
				continue;
			}

			GRPacket::PacketType type = pack->get_type();
			//_log((int)type);

			switch (type) {
				case GRPacket::PacketType::InitData:
					ERR_PRINT("NOT IMPLEMENTED");
					break;
				case GRPacket::PacketType::ImageData:
					ERR_PRINT("NOT IMPLEMENTED");
					break;
				case GRPacket::PacketType::InputData: {
					Ref<GRPacketInputData> data = pack;
					if (data.is_null()) {
						ERR_PRINT("Incorrect GRPacketInputData");
						break;
					}
					if (!_parse_input_data(data->get_input_data())) {
						connection->disconnect_from_host();
						break;
					}

					break;
				}
				case GRPacket::PacketType::ServerSettings: {
					Ref<GRPacketServerSettings> data = pack;
					if (data.is_null()) {
						ERR_PRINT("Incorrect GRPacketServerSettings");
						break;
					}
					dev->_update_settings_from_client(data->get_settings());
					break;
				}
				case GRPacket::PacketType::ServerSettingsRequest:
					ERR_PRINT("NOT IMPLEMENTED");
					break;
				case GRPacket::PacketType::Ping: {
					Ref<GRPacketPong> pack(memnew(GRPacketPong));
					err = ppeer->put_var(pack->get_data());

					if (err) {
						_log("Send pong failed with code: " + str(err), LogLevel::LL_Error);
						goto end_recv;
					}
					break;
				}
				case GRPacket::PacketType::Pong: {
					dev->_update_avg_ping(os->get_ticks_msec() - prev_ping_sending_time);
					ping_sended = false;
					break;
				}
				default:
					_log("Not supported packet type! " + str((int)type), LogLevel::LL_Warning);
					break;
			}
		}
	end_recv:

		if (nothing_happens) // for less cpu using
			os->delay_usec(1_ms);
	}

	_log("Closing connection thread with address: " + address, LogLevel::LL_Debug);
	dev->_load_settings();
	dev->break_connection = true;
}

void GRServer::_thread_image_processing(void *p_userdata) {
	Thread::set_name("GR_image_encoding");

	ImgProcessingStorage *ips = (ImgProcessingStorage *)p_userdata;
	GRServer *dev = ips->dev;
	OS *os = OS::get_singleton();

	while (!dev->stop_device) {

		if (ips->tex.is_valid() && !ips->is_new_data) {
			TimeCountInit();

			Ref<Image> img = ips->tex->get_data();
			TimeCount("Get Image Data From Viewport");

			if (img.is_null()) {
				os->delay_usec(1_ms);
				continue;
			}

			ips->ret_data = compress_jpg(img, dev->jpg_quality, GRUtils::SUBSAMPLING_H2V2);
			ips->is_new_data = true;
			ips->tex.unref();
		}

		os->delay_usec(1_ms);
	}
}

GRServer::AuthResult GRServer::_auth_client(GRServer *dev, Ref<StreamPeerTCP> con) {
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
	//
	// total return size = 5

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

		if (size != 3) // body size must be equal to 3
			return AuthResult::Error;

		PoolByteArray body;
		body.resize(size);
		w = body.write();

		// get body data
		err = con->get_data(w.ptr(), size);
		if (err != OK)
			return AuthResult::Error;
		w.release();

		PoolByteArray ret;
		// prepare response packet
		ret.append_array(get_packet_header());

		r = body.read();
		if (!validate_version(r.ptr())) {
			ret.append((int)AuthErrorCode::VersionMismatch);
			auto rr = ret.read();
			con->put_data(rr.ptr(), ret.size());
			return AuthResult::VersionMismatch;
		}
		r.release();

		// send back final validation packet
		ret.append((int)AuthErrorCode::OK);
		r = ret.read();
		err = con->put_data(r.ptr(), ret.size());
		r.release();
		if (err == OK) {
			return AuthResult::OK;
		}

		return AuthResult::Error;
	}

	return AuthResult::Error;
}

bool GRServer::_parse_input_data(const PoolByteArray &p_data) {
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

		if (data == next) {
			_log("Incorrect Input Data!!! Something wrong with data received from client!\n" + str_arr(p_data, true) + "\n", LogLevel::LL_Error);
			return false;
		}

		data += 5;

		switch (type) {
			case InputType::InputDeviceSensors: {
				PoolVector3Array vecs = bytes2var(data, 12 * 4 + 8);

				if (vecs.size() == 4) {
					set_accelerometer(vecs[0]);
					set_gravity(vecs[1]);
					set_gyroscope(vecs[2]);
					set_magnetometer(vecs[3]);
				}

				break;
			}
			case InputType::InputEventAction: {
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
			case InputType::InputEventJoypadButton: {
				Ref<InputEventJoypadButton> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_button_index(decode_uint32(data));
				e->set_pressure(decode_float(data + 4));
				e->set_pressed(*(data + 8));
				ev = e;
				break;
			}
			case InputType::InputEventJoypadMotion: {
				Ref<InputEventJoypadMotion> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_axis(decode_uint32(data));
				e->set_axis_value(decode_float(data + 4));
				ev = e;
				break;
			}
			case InputType::InputEventKey: {
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
			case InputType::InputEventMagnifyGesture: {
				Ref<InputEventMagnifyGesture> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_factor(decode_float(data));
				ev = e;
				break;
			}
			case InputType::InputEventMIDI: {
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
			case InputType::InputEventMouseButton: {
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
			case InputType::InputEventMouseMotion: {
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
			case InputType::InputEventPanGesture: {
				Ref<InputEventPanGesture> e;
				e.instance();
				data = _read_abstract_input_data(e.ptr(), vp_size, data);

				e->set_delta(bytes2var<Vector2>(data, 12) * vp_size);
				ev = e;
				break;
			}
			case InputType::InputEventScreenDrag: {
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
			case InputType::InputEventScreenTouch: {
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
				_log(String("Not supported InputEvent type: ") + str((int)type));
				break;
			}
		}

		data = next;
		if (ev.is_valid()) {
			Input::get_singleton()->call_deferred("parse_input_event", ev);
		}
	}

	return true;
}

const uint8_t *GRServer::_read_abstract_input_data(InputEvent *ie, const Vector2 &vs, const uint8_t *data) {
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
////////////// GRSViewport //////////////////
//////////////////////////////////////////////

void GRSViewport::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_size"), &GRSViewport::_update_size);
	ClassDB::bind_method(D_METHOD("set_rendering_scale"), &GRSViewport::set_rendering_scale);
	ClassDB::bind_method(D_METHOD("get_rendering_scale"), &GRSViewport::get_rendering_scale);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "rendering_scale", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_rendering_scale", "get_rendering_scale");
}

void GRSViewport::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE: {
			main_vp = SceneTree::get_singleton()->get_root();
			main_vp->connect("size_changed", this, "_update_size");
			_update_size();

			renderer = memnew(GRSViewportRenderer);
			renderer->tex = main_vp->get_texture();
			add_child(renderer);

			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			remove_child(renderer);
			//renderer->queue_delete();
			memdelete(renderer);
			main_vp->disconnect("size_changed", this, "_update_size");
			main_vp = nullptr;
			break;
		}
		default:
			break;
	}
}

void GRSViewport::_update_size() {
	float scale = rendering_scale;
	if (auto_scale > 0)
		scale = auto_scale;

	if (main_vp && main_vp->get_texture().is_valid()) {
		Vector2 size = main_vp->get_size() * scale;
		if (get_size() == size)
			return;

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

void GRSViewport::set_rendering_scale(float val) {
	rendering_scale = val;
	_update_size();
}

float GRSViewport::get_rendering_scale() {
	return rendering_scale;
}

GRSViewport::GRSViewport() {
	set_name("GRSViewport");

	rendering_scale = GET_PS(GodotRemote::ps_scale_of_sending_stream_name);

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
/////////// GRSViewportRenderer /////////////
//////////////////////////////////////////////

void GRSViewportRenderer::_bind_methods() {
}

void GRSViewportRenderer::_notification(int p_notification) {
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

GRSViewportRenderer::GRSViewportRenderer() {
	set_name("GRSViewportRenderer");
	set_process(true);

	set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
}

#endif // !NO_GODOTREMOTE_SERVER

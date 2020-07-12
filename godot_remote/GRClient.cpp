/* GRClient.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRClient.h"
#include "GRPacket.h"
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

void GRClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_texture_from_iamge", "image"), &GRClient::_update_texture_from_iamge);

	ClassDB::bind_method(D_METHOD("set_control_to_show_in", "control_node", "position_in_node"), &GRClient::set_control_to_show_in, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_texture", "texture"), &GRClient::set_custom_no_signal_texture);
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_material", "material"), &GRClient::set_custom_no_signal_material);
	ClassDB::bind_method(D_METHOD("set_address", "ip", "port", "ipv4"), &GRClient::set_address, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_ip", "ip", "ipv4"), &GRClient::set_ip, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_server_setting", "setting", "value"), &GRClient::set_server_setting);
	ClassDB::bind_method(D_METHOD("disable_overriding_server_settings"), &GRClient::disable_overriding_server_settings);

	ClassDB::bind_method(D_METHOD("get_ip"), &GRClient::get_ip);
	ClassDB::bind_method(D_METHOD("is_stream_active"), &GRClient::is_stream_active);
	ClassDB::bind_method(D_METHOD("is_connected_to_host"), &GRClient::is_connected_to_host);

	ADD_SIGNAL(MethodInfo("stream_state_changed", PropertyInfo(Variant::BOOL, "is_active")));
	ADD_SIGNAL(MethodInfo("connection_state_changed", PropertyInfo(Variant::BOOL, "is_connected")));
	ADD_SIGNAL(MethodInfo("working_state_changed", PropertyInfo(Variant::BOOL, "is_working")));

	// SETGET
	ClassDB::bind_method(D_METHOD("set_capture_on_focus", "val"), &GRClient::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD("set_capture_when_hover", "val"), &GRClient::set_capture_when_hover);
	ClassDB::bind_method(D_METHOD("set_connection_type", "type"), &GRClient::set_connection_type);
	ClassDB::bind_method(D_METHOD("set_target_send_fps", "fps"), &GRClient::set_target_send_fps);
	ClassDB::bind_method(D_METHOD("set_stretch_mode", "mode"), &GRClient::set_stretch_mode);

	ClassDB::bind_method(D_METHOD("is_capture_on_focus"), &GRClient::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD("is_capture_when_hover"), &GRClient::is_capture_when_hover);
	ClassDB::bind_method(D_METHOD("get_connection_type"), &GRClient::get_connection_type);
	ClassDB::bind_method(D_METHOD("get_target_send_fps"), &GRClient::get_target_send_fps);
	ClassDB::bind_method(D_METHOD("get_stretch_mode"), &GRClient::get_stretch_mode);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), "set_capture_on_focus", "is_capture_on_focus");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), "set_capture_when_hover", "is_capture_when_hover");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_type", PROPERTY_HINT_ENUM, "WiFi,ADB"), "set_connection_type", "get_connection_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_send_fps", PROPERTY_HINT_RANGE, "1,1000"), "set_target_send_fps", "get_target_send_fps");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stretch_mode", PROPERTY_HINT_ENUM, "Fill,Keep Aspect"), "set_stretch_mode", "get_stretch_mode");

	BIND_ENUM_CONSTANT(CONNECTION_ADB);
	BIND_ENUM_CONSTANT(CONNECTION_WiFi);

	BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT);
	BIND_ENUM_CONSTANT(STRETCH_FILL);
}

void GRClient::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_EXIT_TREE: {
			is_deleting = true;
			_internal_call_only_deffered_stop();
			break;
		}
		case NOTIFICATION_PREDELETE: {
			is_deleting = true;
			_internal_call_only_deffered_stop();
			break;
		}
	}
}

GRClient::GRClient() :
		GRDevice() {

	ip_validator.instance();
	ip_validator->compile(ip_validator_pattern);

	set_name("GodotRemoteClient");
	ips = new ImgProcessingStorage(this);
	thread_image_decoder = Thread::create(&_thread_image_decoder, ips);

	peer.instance();
	send_queue_mutex = Mutex::create();
	connection_mutex = Mutex::create();

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_image.instance();
	GetPoolVectorFromBin(tmp_no_signal, GRResources::Bin_NoSignalPNG);
	no_signal_image->load_png_from_buffer(tmp_no_signal);

	Ref<Shader> shader;
	shader.instance();
	shader->set_code(GRResources::Txt_CRT_Shader);
	no_signal_mat.instance();
	no_signal_mat->set_shader(shader);
#endif
}

GRClient::~GRClient() {
	is_deleting = true;
	_internal_call_only_deffered_stop();
	memdelete(send_queue_mutex);
	memdelete(connection_mutex);

	ips->is_working = false;
	if (thread_image_decoder) {
		Thread::wait_to_finish(thread_image_decoder);
	}
	memdelete(thread_image_decoder);
	thread_image_decoder = nullptr;
	delete ips;

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		tex_shows_stream->queue_delete();
	}
	if (input_collector && !input_collector->is_queued_for_deletion()) {
		input_collector->queue_delete();
	}
	tex_shows_stream = nullptr;
	input_collector = nullptr;

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_mat.unref();
	no_signal_image.unref();
#endif
}

void GRClient::set_control_to_show_in(Control *ctrl, int position_in_node) {
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

		signal_connection_state = true; // force execute update function
		_update_stream_texture_state(false);

		control_to_show_in->add_child(tex_shows_stream);
		control_to_show_in->move_child(tex_shows_stream, position_in_node);
		control_to_show_in->add_child(input_collector);

		input_collector->set_capture_on_focus(capture_only_when_control_in_focus);
		input_collector->set_gr_device(this);
		input_collector->set_tex_rect(tex_shows_stream);
	}
}

void GRClient::set_custom_no_signal_texture(Ref<Texture> custom_tex) {
	custom_no_signal_texture = custom_tex;
}

void GRClient::set_custom_no_signal_material(Ref<Material> custom_mat) {
	custom_no_signal_material = custom_mat;
}

bool GRClient::is_capture_on_focus() {
	return capture_only_when_control_in_focus;
}

void GRClient::set_capture_on_focus(bool value) {
	capture_only_when_control_in_focus = value;

	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_on_focus(capture_only_when_control_in_focus);
}

bool GRClient::is_capture_when_hover() {
	return capture_pointer_only_when_hover_control;
}

void GRClient::set_capture_when_hover(bool value) {
	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_when_hover(capture_pointer_only_when_hover_control);
}

void GRClient::set_connection_type(int type) {
	con_type = (ConnectionType)type;
}

int GRClient::get_connection_type() {
	return con_type;
}

void GRClient::set_target_send_fps(int fps) {
	ERR_FAIL_COND(fps <= 0);
	send_data_fps = fps;
}

int GRClient::get_target_send_fps() {
	return send_data_fps;
}

void GRClient::set_stretch_mode(int stretch) {
	stretch_mode = (StretchMode)stretch;
	_update_stream_texture_state(signal_connection_state);
}

int GRClient::get_stretch_mode() {
	return stretch_mode;
}

bool GRClient::is_stream_active() {
	return signal_connection_state;
}

String GRClient::get_ip() {
	return (String)server_address;
}

bool GRClient::set_ip(String ip, bool ipv4) {
	return set_address(ip, port, ipv4);
}

// TODO try to resolve address
/*IP_Address ip;
if (p_host.is_valid_ip_address())
	ip = p_host;
else
	ip = IP::get_singleton()->resolve_hostname(p_host);*/

bool GRClient::set_address(String ip, uint16_t _port, bool ipv4) {
	bool old = is_working();
	if (old) {
		//_internal_call_only_deffered_stop();
		stop();
	}
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
	bool all_ok = true;
	if (!server_address.is_valid() || is_invalid) {
		ERR_PRINT(ip + " is an invalid address!");
		_log(ip + " is an invalid address!");
		server_address = IP_Address(prev);
		port = prevPort;
		all_ok = false;
	}

	if (old) {
		//_internal_call_only_deffered_start();
		start();
	}
	return all_ok;
}

void GRClient::set_input_buffer(int mb) {
	bool old = is_working();
	if (old) {
		//_internal_call_only_deffered_stop();
		stop();
	}
	input_buffer_size_in_mb = mb;
	if (old) {
		//_internal_call_only_deffered_start();
		start();
	}
}

bool GRClient::_internal_call_only_deffered_start() {
	if (working) {
		ERR_FAIL_V_MSG(false, "Can't start already working GodotRemote Client");
	}

	_log("Starting GodotRemote client", LogLevel::LL_Debug);

	working = true;

	thread_connection.instance();
	thread_connection->dev = this;
	thread_connection->thread_ref = Thread::create(&_thread_connection, thread_connection.ptr());

	call_deferred("emit_signal", "working_state_changed", true);

	return true;
}

void GRClient::_internal_call_only_deffered_stop() {
	if (!working)
		return;

	connection_mutex->try_lock();
	_log("Stopping GodotRemote client", LogLevel::LL_Debug);

	working = false;
	thread_connection->stop_thread = true;
	thread_connection->break_connection = true;

	send_queue_mutex->unlock();
	connection_mutex->unlock();

	_update_stream_texture_state(false);

	call_deferred("emit_signal", "working_state_changed", false);

	thread_connection.unref();
}

bool GRClient::is_connected_to_host() {
	return peer->is_connected_to_host() && is_connection_working;
}

void GRClient::_update_texture_from_iamge(Ref<Image> img) {
	// avg fps
	uint32_t time = OS::get_singleton()->get_ticks_msec();
	_update_avg_fps(time - prev_display_image_time);
	prev_display_image_time = time;

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		Ref<ImageTexture> tex = tex_shows_stream->get_texture();
		if (tex.is_valid()) {
			tex->create_from_image(img);
		} else {
			Ref<ImageTexture> tex;
			tex.instance();
			tex->create_from_image(img);
			tex_shows_stream->set_texture(tex);
		}
	}
}

void GRClient::_update_stream_texture_state(bool is_has_signal) {
	if (is_deleting)
		return;

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		if (is_has_signal) {
			tex_shows_stream->set_stretch_mode(stretch_mode == StretchMode::STRETCH_KEEP_ASPECT ? TextureRect::STRETCH_KEEP_ASPECT_CENTERED : TextureRect::STRETCH_SCALE);
			tex_shows_stream->set_material(nullptr);

			if (signal_connection_state != is_has_signal) {
				call_deferred("emit_signal", "stream_state_changed", true);
			}
			signal_connection_state = true;
		} else {
			tex_shows_stream->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

			if (signal_connection_state != is_has_signal) {
				call_deferred("emit_signal", "stream_state_changed", false);
			}
			signal_connection_state = false;

			if (custom_no_signal_texture.is_valid()) {
				tex_shows_stream->set_texture(custom_no_signal_texture);
			}
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
			else {
				Ref<ImageTexture> tex = tex_shows_stream->get_texture();
				if (tex.is_valid()) {
					tex->create_from_image(no_signal_image);
				} else {
					tex.instance();
					tex->create_from_image(no_signal_image);
					tex_shows_stream->set_texture(tex);
				}
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

void GRClient::_reset_counters() {
	GRDevice::_reset_counters();
	prev_display_image_time = OS::get_singleton()->get_ticks_msec() - 16;
}

void GRClient::set_server_setting(int param, Variant value) {
	send_queue_mutex->lock();
	Ref<GRPacketServerSettings> packet = _find_queued_packet_by_type<Ref<GRPacketServerSettings> >();
	if (packet.is_null()) {
		packet.instance();
		send_queue.push_back(packet);
	}

	packet->add_setting(param, value);
	send_queue_mutex->unlock();
}

void GRClient::disable_overriding_server_settings() {
	set_server_setting((int)GodotRemote::TypesOfServerSettings::USE_INTERNAL_SERVER_SETTINGS, true);
}

//////////////////////////////////////////////
////////////////// STATIC ////////////////////
//////////////////////////////////////////////

void GRClient::_thread_connection(void *p_userdata) {
	Ref<ConnectionThreadParams> con_thread = (ConnectionThreadParams *)p_userdata;
	GRClient *dev = con_thread->dev;

	Ref<StreamPeerTCP> con = dev->peer;
	OS *os = OS::get_singleton();
	Thread::set_name("GRemote_connection");

	while (!con_thread->stop_thread) {
		if (os->get_ticks_msec() - dev->prev_valid_connection_time > 1000) {
			dev->_update_stream_texture_state(false);
		}

		if (con->get_status() == StreamPeerTCP::STATUS_CONNECTED || con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			con->disconnect_from_host();
		}

		IP_Address ip = dev->con_type == ConnectionType::CONNECTION_ADB ? IP_Address("127.0.0.1") : dev->server_address;

		String address = (String)ip + ":" + str(dev->port);
		Error err = con->connect_to_host(ip, dev->port);

		if (err == OK) {
			_log("Connecting to " + address, LogLevel::LL_Debug);
		} else {
			switch (err) {
				case FAILED:
					_log("Failed to open socket or can't connect to host", LogLevel::LL_Error);
					break;
				case ERR_UNAVAILABLE:
					_log("Socket is unavailable", LogLevel::LL_Error);
					break;
				case ERR_INVALID_PARAMETER:
					_log("Host address is invalid", LogLevel::LL_Error);
					break;
				case ERR_ALREADY_EXISTS:
					_log("Socket already in use", LogLevel::LL_Error);
					break;
			}
			os->delay_usec(250_ms);
			continue;
		}

		while (con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			os->delay_usec(1_ms);
		}

		if (con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			_log("Timeout! Connect to " + address + " failed.", LogLevel::LL_Debug);
			os->delay_usec(50_ms);
			continue;
		}

		con->set_no_delay(true);

		if (_auth_on_server(con)) {
			_log("Successful connected to " + address);

			dev->_update_stream_texture_state(true);

			con_thread->break_connection = false;
			dev->is_connection_working = true;
			dev->call_deferred("emit_signal", "connection_state_changed", true);

			_connection_loop(con_thread, con);

			dev->is_connection_working = false;
			dev->call_deferred("emit_signal", "connection_state_changed", false);
		}

		if (con->is_connected_to_host())
			con->disconnect_from_host();
	}

	dev->_update_stream_texture_state(false);
	if (con.is_valid() && con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
		con.unref();
	}
	_log("Connection thread stopped", LogLevel::LL_Debug);
}

void GRClient::_connection_loop(Ref<ConnectionThreadParams> con_thread, Ref<StreamPeerTCP> connection) {
	GRClient *dev = con_thread->dev;
	ImgProcessingStorage *ips = dev->ips;
	OS *os = OS::get_singleton();
	Error err = Error::OK;

	Ref<PacketPeerStream> ppeer(memnew(PacketPeerStream));
	ppeer->set_stream_peer(connection);
	ppeer->set_input_buffer_max_size(dev->input_buffer_size_in_mb * 1024 * 1024);

	dev->_reset_counters();

	uint32_t prev_time = os->get_ticks_msec();
	uint32_t prev_ping_sending_time = prev_time;
	bool ping_sended = false;

	while (!con_thread->break_connection && connection->is_connected_to_host()) {
		dev->connection_mutex->lock();
		if (con_thread->break_connection || !connection->is_connected_to_host())
			break;

		bool nothing_happens = true;
		uint32_t time = os->get_ticks_msec();
		uint32_t start_while_time = 0;
		dev->prev_valid_connection_time = time;
		int send_data_time_ms = (1000 / dev->send_data_fps);

		///////////////////////////////////////////////////////////////////
		// SENDING

		// INPUT
		if ((time - prev_time) > send_data_time_ms) {
			prev_time = time;
			nothing_happens = false;

			PoolByteArray d = dev->input_collector->get_collected_input_data();

			if (d.size() == 0)
				goto end_send;

			Ref<GRPacketInputData> pack(memnew(GRPacketInputData));
			pack->set_input_data(d);

			err = ppeer->put_var(pack->get_data());

			if (err) {
				_log("Put input data failed with code: " + str(err));
				goto end_send;
			}
		}

		// PING
		if ((time - prev_ping_sending_time) > 100 && !ping_sended) {
			prev_ping_sending_time = time;
			nothing_happens = false;
			ping_sended = true;

			Ref<GRPacketPing> pack(memnew(GRPacketPing));
			err = ppeer->put_var(pack->get_data());

			if (err) {
				_log("Send ping failed with code: " + str(err));
				goto end_send;
			}
		}

		// SEND QUEUE
		start_while_time = os->get_ticks_msec();
		while (!dev->send_queue.empty() && (os->get_ticks_msec() - start_while_time) <= send_data_time_ms / 2) {
			dev->send_queue_mutex->lock();

			Ref<GRPacket> packet = dev->send_queue.front()->get();
			if (packet.is_valid()) {
				dev->send_queue.pop_front();
				err = ppeer->put_var(packet->get_data());

				if (err) {
					_log("Put data from queue failed with code: " + str(err));
					dev->send_queue_mutex->unlock();
					goto end_send;
				}
			}
			dev->send_queue_mutex->unlock();
		}
	end_send:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after sending!", LogLevel::LL_Error);
			continue;
		}

		///////////////////////////////////////////////////////////////////
		// RECEIVING
		start_while_time = os->get_ticks_msec();
		while (ppeer->get_available_packet_count() > 0 && (os->get_ticks_msec() - start_while_time) <= send_data_time_ms / 2) {
			nothing_happens = false;

			Variant buf;
			ppeer->get_var(buf);

			if (err)
				goto end_recv;

			Ref<GRPacket> pack = GRPacket::create(buf);
			GRPacket::PacketType type = pack->get_type();

			switch (type) {
				case GRPacket::PacketType::InitData:
					break;
				case GRPacket::PacketType::ImageData: {
					Ref<GRPacketImageData> data = pack;
					if (data.is_null()) {
						ERR_FAIL_MSG("GRPacketImageData is null");
						goto end_recv;
					}

					if (!ips->is_new_data) {
						ips->tex_data = data->get_image_data();
						ips->is_new_data = true;
					}
					break;
				}
				case GRPacket::PacketType::InputData: {
					break;
				}
				case GRPacket::PacketType::Ping: {
					Ref<GRPacketPong> pack(memnew(GRPacketPong));
					err = ppeer->put_var(pack->get_data());
					if (err) {
						_log("Send pong failed with code: " + str(err));
						goto end_send;
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
		dev->connection_mutex->unlock();

		if (nothing_happens)
			os->delay_usec(1_ms);
	}

	_log("Closing connection");
	con_thread->break_connection = true;
}

void GRClient::_thread_image_decoder(void *p_userdata) {
	Thread::set_name("GR_image_encoding");

	ImgProcessingStorage *ips = (ImgProcessingStorage *)p_userdata;
	GRClient *dev = ips->dev;
	OS *os = OS::get_singleton();

	while (ips->is_working) {
		if (ips->is_new_data) {
			Ref<Image> img;
			img.instance();
			TimeCountInit();
			if (img->load_jpg_from_buffer(ips->tex_data) == OK) {
				dev->call_deferred("_update_texture_from_iamge", img);
				TimeCount("Decode Image Time");
			} else {
				_log("Can't decode JPG image.", LogLevel::LL_Error);
			}
			ips->is_new_data = false;
		}
		os->delay_usec(1_ms);
	}
}

bool GRClient::_auth_on_server(Ref<StreamPeerTCP> con) {
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
					_log("Can't connect to server. Version mismatch.");
					return false;
			}
		}
	} else {
		return false;
	}

	return false;
}

//////////////////////////////////////////////
///////////// INPUT COLLECTOR ////////////////
//////////////////////////////////////////////

void GRInputCollector::_update_stream_rect() {
	if (!dev || !dev->is_working())
		return;

	if (texture_rect && !texture_rect->is_queued_for_deletion()) {
		switch (dev->get_stretch_mode()) {
			case GRClient::StretchMode::STRETCH_KEEP_ASPECT: {
				Ref<Texture> tex = texture_rect->get_texture();
				if (tex.is_null())
					goto fill;

				Vector2 pos = texture_rect->get_global_position();
				Vector2 outer_size = texture_rect->get_size();
				Vector2 inner_size = tex->get_size();
				float asp_rec = outer_size.x / outer_size.y;
				float asp_tex = inner_size.x / inner_size.y;

				if (asp_rec > asp_tex) {
					float width = outer_size.y * asp_tex;
					stream_rect = Rect2(Vector2(pos.x + (outer_size.x - width) / 2, pos.y), Vector2(width, outer_size.y));
					return;
				} else {
					float height = outer_size.x / asp_tex;
					stream_rect = Rect2(Vector2(pos.x, pos.y + (outer_size.y - height) / 2), Vector2(outer_size.x, height));
					return;
				}
				break;
			}
			case GRClient::StretchMode::STRETCH_FILL:
			default:
			fill:
				stream_rect = Rect2(texture_rect->get_global_position(), texture_rect->get_size());
				return;
		}
	}
	if (parent && !parent->is_queued_for_deletion()) {
		stream_rect = Rect2(parent->get_global_position(), parent->get_size());
	}
	return;
}

void GRInputCollector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_input", "input_event"), &GRInputCollector::_input);

	ClassDB::bind_method(D_METHOD("is_capture_on_focus"), &GRInputCollector::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD("set_capture_on_focus", "value"), &GRInputCollector::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD("is_capture_when_hover"), &GRInputCollector::is_capture_when_hover);
	ClassDB::bind_method(D_METHOD("set_capture_when_hover", "value"), &GRInputCollector::set_capture_when_hover);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), "set_capture_on_focus", "is_capture_on_focus");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), "set_capture_when_hover", "is_capture_when_hover");
}

#define fix(e) (e - stream_rect.position) / stream_rect.size

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

void GRInputCollector::_input(Ref<InputEvent> ie) {
	if (!parent || (capture_only_when_control_in_focus && !parent->has_focus()) || (dev && !dev->is_working()) || !dev->is_stream_active()) {
		return;
	}

	if (collected_input_data.size() > 1024 * 8) {
		collected_input_data.resize(0);
	}

	// TODO maybe later i change it to update less frequent
	_update_stream_rect();

	PoolByteArray data;

	Ref<InputEventMouse> iem = ie;
	Ref<InputEventGesture> ieg = ie;
	Ref<InputEventWithModifiers> iewm = ie;

	if (ie.is_null()) {
		_log("InputEvent is null", LogLevel::LL_Error);
		goto end;
	}

	{
		Ref<InputEventKey> iek = ie;
		if (iek.is_valid()) {
			data.append(GRDevice::InputType::_InputEventKey);
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
			if (!stream_rect.has_point(iemb->get_position()) && capture_pointer_only_when_hover_control) {
				int idx = iemb->get_button_index();
				if (idx == BUTTON_WHEEL_UP || idx == BUTTON_WHEEL_DOWN ||
						idx == BUTTON_WHEEL_LEFT || idx == BUTTON_WHEEL_RIGHT) {
					return;
				} else {
					if (iemb->is_pressed())
						return;
				}
			}
			data.append(GRDevice::InputType::_InputEventMouseButton);
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
			if (!stream_rect.has_point(iemm->get_position()) && capture_pointer_only_when_hover_control)
				return;
			data.append(GRDevice::InputType::_InputEventMouseMotion);
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
			if (!stream_rect.has_point(iest->get_position()) && capture_pointer_only_when_hover_control) {
				if (iest->is_pressed())
					return;
			}
			data.append(GRDevice::InputType::_InputEventScreenTouch);
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
			if (!stream_rect.has_point(iesd->get_position()) && capture_pointer_only_when_hover_control)
				return;
			data.append(GRDevice::InputType::_InputEventScreenDrag);
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
			if (!stream_rect.has_point(iemg->get_position()) && capture_pointer_only_when_hover_control)
				return;
			data.append(GRDevice::InputType::_InputEventMagnifyGesture);
			data_append_defaults();

			data.append_array(float2bytes(iemg->get_factor()));

			goto end;
		}
	}

	{
		Ref<InputEventPanGesture> iepg = ie;
		if (iepg.is_valid()) {
			if (!stream_rect.has_point(iepg->get_position()) && capture_pointer_only_when_hover_control)
				return;
			data.append(GRDevice::InputType::_InputEventPanGesture);
			data_append_defaults();

			data.append_array(var2bytes(fix(iepg->get_delta())));

			goto end;
		}
	}

	{
		Ref<InputEventJoypadButton> iejb = ie;
		if (iejb.is_valid()) {
			data.append(GRDevice::InputType::_InputEventJoypadButton);
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
			data.append(GRDevice::InputType::_InputEventJoypadMotion);
			data_append_defaults();

			data.append_array(uint322bytes(iejm->get_axis()));
			data.append_array(float2bytes(iejm->get_axis_value()));

			goto end;
		}
	}

	{
		Ref<InputEventAction> iea = ie;
		if (iea.is_valid()) {
			data.append(GRDevice::InputType::_InputEventAction);
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
			data.append(GRDevice::InputType::_InputEventMIDI);
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
	_log("InputEvent type " + str(ie) + " not supported!", LogLevel::LL_Error);

end:

	if (data.size()) {
		collected_input_data.append_array(uint322bytes(data.size() + 4)); // block size
		collected_input_data.append_array(data);
	}
}

#undef fix
#undef data_append_defaults

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

void GRInputCollector::set_gr_device(GRClient *_dev) {
	dev = _dev;
}

void GRInputCollector::set_tex_rect(TextureRect *tr) {
	texture_rect = tr;
}

PoolByteArray GRInputCollector::get_collected_input_data() {
	PoolByteArray res;
	PoolVector3Array sensors;
	sensors.resize(4);
	auto w = sensors.write();
	w[0] = Input::get_singleton()->get_accelerometer();
	w[1] = Input::get_singleton()->get_gravity();
	w[2] = Input::get_singleton()->get_gyroscope();
	w[3] = Input::get_singleton()->get_magnetometer();
	w.release();

	// sensors
	res.append_array(uint322bytes(12 * sensors.size() + 8 + 4 + 1)); // +8 - sensors header, +4 - this value, +1 - type
	res.append(GRDevice::InputType::_InputDeviceSensors);
	res.append_array(var2bytes(sensors));

	// other input events
	res.append_array(collected_input_data);
	collected_input_data.resize(0);

	return res;
}

GRInputCollector::GRInputCollector() {
	parent = nullptr;
	set_process_input(true);
}

GRInputCollector::~GRInputCollector() {
}

#endif // !NO_GODOTREMOTE_CLIENT

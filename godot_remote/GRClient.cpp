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
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"

using namespace GRUtils;

void GRClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_texture_from_iamge", "image"), &GRClient::_update_texture_from_iamge);
	ClassDB::bind_method(D_METHOD("_update_stream_texture_state", "state"), &GRClient::_update_stream_texture_state);

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

	// SETGET
	ClassDB::bind_method(D_METHOD("set_capture_on_focus", "val"), &GRClient::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD("set_capture_when_hover", "val"), &GRClient::set_capture_when_hover);
	ClassDB::bind_method(D_METHOD("set_connection_type", "type"), &GRClient::set_connection_type);
	ClassDB::bind_method(D_METHOD("set_target_send_fps", "fps"), &GRClient::set_target_send_fps);
	ClassDB::bind_method(D_METHOD("set_stretch_mode", "mode"), &GRClient::set_stretch_mode);
	ClassDB::bind_method(D_METHOD("set_texture_filtering", "is_filtered"), &GRClient::set_texture_filtering);
	ClassDB::bind_method(D_METHOD("set_password", "password"), &GRClient::set_password);

	ClassDB::bind_method(D_METHOD("is_capture_on_focus"), &GRClient::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD("is_capture_when_hover"), &GRClient::is_capture_when_hover);
	ClassDB::bind_method(D_METHOD("get_connection_type"), &GRClient::get_connection_type);
	ClassDB::bind_method(D_METHOD("get_target_send_fps"), &GRClient::get_target_send_fps);
	ClassDB::bind_method(D_METHOD("get_stretch_mode"), &GRClient::get_stretch_mode);
	ClassDB::bind_method(D_METHOD("get_texture_filtering"), &GRClient::get_texture_filtering);
	ClassDB::bind_method(D_METHOD("get_password"), &GRClient::get_password);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), "set_capture_on_focus", "is_capture_on_focus");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), "set_capture_when_hover", "is_capture_when_hover");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_type", PROPERTY_HINT_ENUM, "WiFi,ADB"), "set_connection_type", "get_connection_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_send_fps", PROPERTY_HINT_RANGE, "1,1000"), "set_target_send_fps", "get_target_send_fps");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stretch_mode", PROPERTY_HINT_ENUM, "Fill,Keep Aspect"), "set_stretch_mode", "get_stretch_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "texture_filtering"), "set_texture_filtering", "get_texture_filtering");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");

	BIND_ENUM_CONSTANT(CONNECTION_ADB);
	BIND_ENUM_CONSTANT(CONNECTION_WiFi);

	BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT);
	BIND_ENUM_CONSTANT(STRETCH_FILL);
}

void GRClient::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_EXIT_TREE:
		case NOTIFICATION_PREDELETE: {
			is_deleting = true;
			if (get_status() == (int)WorkingStatus::Working) {
				_internal_call_only_deffered_stop();
			}
			break;
		}
	}
}

GRClient::GRClient() :
		GRDevice() {
	ip_validator.instance();
	ip_validator->compile(ip_validator_pattern);

	set_name("GodotRemoteClient");

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
	if (get_status() == (int)WorkingStatus::Working) {
		_internal_call_only_deffered_stop();
	}
	memdelete(send_queue_mutex);
	memdelete(connection_mutex);

	set_control_to_show_in(nullptr);

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_mat.unref();
	no_signal_image.unref();
#endif
}

void GRClient::_internal_call_only_deffered_start() {
	switch ((WorkingStatus)get_status()) {
		case GRDevice::WorkingStatus::Working:
			ERR_FAIL_MSG("Can't start already working GodotRemote Client");
		case GRDevice::WorkingStatus::Starting:
			ERR_FAIL_MSG("Can't start already starting GodotRemote Client");
		case GRDevice::WorkingStatus::Stopping:
			ERR_FAIL_MSG("Can't start stopping GodotRemote Client");
	}

	_log("Starting GodotRemote client", LogLevel::LL_Debug);
	set_status(WorkingStatus::Starting);

	if (thread_connection.is_valid()) {
		thread_connection->close_thread();
		thread_connection.unref();
	}
	thread_connection.instance();
	thread_connection->dev = this;
	thread_connection->peer.instance();
	thread_connection->thread_ref = Thread::create(&_thread_connection, thread_connection.ptr());

	call_deferred("_update_stream_texture_state", false);
	set_status(WorkingStatus::Working);
}

void GRClient::_internal_call_only_deffered_stop() {
	switch ((WorkingStatus)get_status()) {
		case GRDevice::WorkingStatus::Stopped:
			ERR_FAIL_MSG("Can't stop already stopped GodotRemote Client");
		case GRDevice::WorkingStatus::Stopping:
			ERR_FAIL_MSG("Can't stop already stopping GodotRemote Client");
		case GRDevice::WorkingStatus::Starting:
			ERR_FAIL_MSG("Can't stop starting GodotRemote Client");
	}

	_log("Stopping GodotRemote client", LogLevel::LL_Debug);
	set_status(WorkingStatus::Stopping);

	connection_mutex->lock();
	if (thread_connection.is_valid()) {
		thread_connection->break_connection = true;
		thread_connection->stop_thread = true;
		connection_mutex->unlock();
		thread_connection->close_thread();
		thread_connection.unref();
	}
	send_queue_mutex->unlock();
	connection_mutex->unlock();

	call_deferred("_update_stream_texture_state", false);
	set_status(WorkingStatus::Stopped);
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
		tex_shows_stream = memnew(GRTextureRect);
		input_collector = memnew(GRInputCollector);

		tex_shows_stream->set_name("GodotRemoteStreamSprite");
		input_collector->set_name("GodotRemoteInputCollector");

		tex_shows_stream->set_expand(true);
		tex_shows_stream->set_anchor(MARGIN_RIGHT, 1.f);
		tex_shows_stream->set_anchor(MARGIN_BOTTOM, 1.f);
		tex_shows_stream->dev = this;
		tex_shows_stream->this_in_client = &tex_shows_stream;

		control_to_show_in->add_child(tex_shows_stream);
		control_to_show_in->move_child(tex_shows_stream, position_in_node);
		control_to_show_in->add_child(input_collector);

		input_collector->set_capture_on_focus(capture_only_when_control_in_focus);
		input_collector->set_tex_rect(tex_shows_stream);
		input_collector->dev = this;
		input_collector->this_in_client = &input_collector;

		signal_connection_state = true; // force execute update function
		call_deferred("_update_stream_texture_state", false);
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
	call_deferred("_update_stream_texture_state", signal_connection_state);
}

int GRClient::get_stretch_mode() {
	return stretch_mode;
}

void GRClient::set_texture_filtering(bool is_filtering) {
	is_filtering_enabled = is_filtering;
}

bool GRClient::get_texture_filtering() {
	return is_filtering_enabled;
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

	restart();
	return all_ok;
}

void GRClient::set_input_buffer(int mb) {

	input_buffer_size_in_mb = mb;
	restart();
}

void GRClient::set_password(String _pass) {
	password = _pass;
}

String GRClient::get_password() {
	return password;
}

bool GRClient::is_connected_to_host() {
	if (thread_connection.is_valid() && thread_connection->peer.is_valid()) {
		return thread_connection->peer->is_connected_to_host() && is_connection_working;
	}
	return false;
}

void GRClient::_update_texture_from_iamge(Ref<Image> img) {
	// avg fps

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

		uint32_t new_flags = Texture::FLAG_MIPMAPS | (is_filtering_enabled ? Texture::FLAG_FILTER : 0);
		if (tex->get_flags() != new_flags) {
			tex->set_flags(new_flags);
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
	sync_time_client = 0;
	sync_time_server = 0;
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
	Ref<StreamPeerTCP> con = con_thread->peer;

	OS *os = OS::get_singleton();
	Thread::set_name("GRemote_connection");

	while (!con_thread->stop_thread) {
		if (os->get_ticks_usec() - dev->prev_valid_connection_time > 1000_ms) {
			dev->call_deferred("_update_stream_texture_state", false);
		}

		if (con->get_status() == StreamPeerTCP::STATUS_CONNECTED || con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			con->disconnect_from_host();
		}

		IP_Address ip = dev->con_type == ConnectionType::CONNECTION_ADB ? IP_Address("127.0.0.1") : dev->server_address;

		String address = (String)ip + ":" + str(dev->port);
		Error err = con->connect_to_host(ip, dev->port);

		_log("Connecting to " + address);
		if (err) {
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
			os->delay_usec(200_ms);
			continue;
		}

		con->set_no_delay(true);

		bool long_wait = false;

		Ref<PacketPeerStream> ppeer(memnew(PacketPeerStream));
		ppeer->set_stream_peer(con);
		ppeer->set_input_buffer_max_size(dev->input_buffer_size_in_mb * 1024 * 1024);

		GRDevice::AuthResult res = _auth_on_server(dev, ppeer);
		switch (res) {
			case GRDevice::AuthResult::OK: {
				_log("Successful connected to " + address);

				dev->call_deferred("_update_stream_texture_state", true);

				con_thread->break_connection = false;
				con_thread->peer = con;
				con_thread->ppeer = ppeer;
				dev->is_connection_working = true;
				dev->call_deferred("emit_signal", "connection_state_changed", true);

				_connection_loop(con_thread);

				con_thread->peer.unref();
				con_thread->ppeer.unref();

				dev->is_connection_working = false;
				dev->call_deferred("emit_signal", "connection_state_changed", false);
				break;
			}
			case GRDevice::AuthResult::Error:
			case GRDevice::AuthResult::Timeout:
			case GRDevice::AuthResult::RefuseConnection:
			case GRDevice::AuthResult::VersionMismatch:
			case GRDevice::AuthResult::IncorrectPassword:
				long_wait = true;
				break;
			case GRDevice::AuthResult::PasswordRequired:
				break;
			default:
				_log("Unknown error code: " + str((int)res) + ". Disconnecting. " + address);
				break;
		}

		if (con->is_connected_to_host()) {
			con->disconnect_from_host();
		}

		if (long_wait) {
			os->delay_usec(888_ms);
		}
	}

	dev->call_deferred("_update_stream_texture_state", false);
	_log("Connection thread stopped", LogLevel::LL_Debug);
	con_thread->finished = true;
}

void GRClient::_connection_loop(Ref<ConnectionThreadParams> con_thread) {
	GRClient *dev = con_thread->dev;
	Ref<StreamPeerTCP> connection = con_thread->peer;
	Ref<PacketPeerStream> ppeer = con_thread->ppeer;

	Thread *_img_thread = nullptr;
	bool _is_processing_img = false;

	OS *os = OS::get_singleton();
	Error err = Error::OK;

	dev->_reset_counters();

	List<Ref<GRPacketImageData> > stream_queue;

	uint64_t time64 = os->get_ticks_usec();
	uint64_t prev_send_input_time = time64;
	uint64_t prev_ping_sending_time = time64;
	uint64_t next_image_required_frametime = time64;
	uint64_t prev_display_image_time = time64 - 16_ms;

	bool ping_sended = false;

	while (!con_thread->break_connection && connection->is_connected_to_host()) {
		dev->connection_mutex->lock();
		if (con_thread->break_connection || !connection->is_connected_to_host())
			break;

		if (!_is_processing_img) {
			if (_img_thread) {
				Thread::wait_to_finish(_img_thread);
				memdelete(_img_thread);
				_img_thread = nullptr;
			}
		}

		bool nothing_happens = true;
		uint64_t start_while_time = 0;
		dev->prev_valid_connection_time = time64;
		int send_data_time_us = (1000000 / dev->send_data_fps);

		///////////////////////////////////////////////////////////////////
		// SENDING

		// INPUT
		time64 = os->get_ticks_usec();
		if ((time64 - prev_send_input_time) > send_data_time_us) {
			prev_send_input_time = time64;
			nothing_happens = false;

			PoolByteArray d = dev->input_collector->get_collected_input_data();

			if (d.size() == 0)
				goto end_send;

			Ref<GRPacketInputData> pack(memnew(GRPacketInputData));
			pack->set_input_data(d);

			err = ppeer->put_var(pack->get_data());

			if (err) {
				_log("Put input data failed with code: " + str(err), LogLevel::LL_Error);
				goto end_send;
			}
		}

		// PING
		time64 = os->get_ticks_usec();
		if ((time64 - prev_ping_sending_time) > 100_ms && !ping_sended) {
			nothing_happens = false;
			ping_sended = true;

			Ref<GRPacketPing> pack(memnew(GRPacketPing));
			err = ppeer->put_var(pack->get_data());
			prev_ping_sending_time = time64;

			if (err) {
				_log("Send ping failed with code: " + str(err), LogLevel::LL_Error);
				goto end_send;
			}
		}

		// SEND QUEUE
		start_while_time = os->get_ticks_usec();
		while (!dev->send_queue.empty() && (os->get_ticks_usec() - start_while_time) <= send_data_time_us / 2) {
			dev->send_queue_mutex->lock();

			Ref<GRPacket> packet = dev->send_queue.front()->get();
			if (packet.is_valid()) {
				dev->send_queue.pop_front();
				err = ppeer->put_var(packet->get_data());

				if (err) {
					_log("Put data from queue failed with code: " + str(err), LogLevel::LL_Error);
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

		// Send to processing one of buffered images
		time64 = os->get_ticks_usec();
		if (!_is_processing_img && !stream_queue.empty() && time64 >= next_image_required_frametime) {
			nothing_happens = false;
			Ref<GRPacketImageData> pack = stream_queue.front()->get();
			stream_queue.pop_front();
			if (pack.is_null()) {
				_log("Queued image data is null", LogLevel::LL_Error);
				goto end_img_process;
			}

			next_image_required_frametime = time64 + pack->get_frametime() * 0.8;

			dev->_update_avg_fps(time64 - prev_display_image_time);
			prev_display_image_time = time64;

			if (_img_thread) {
				Thread::wait_to_finish(_img_thread);
				memdelete(_img_thread);
				_img_thread = nullptr;
			}

			ImgProcessingStorage *ips = new ImgProcessingStorage(dev);
			ips->tex_data = pack->get_image_data();
			ips->_is_processing_img = &_is_processing_img;
			_img_thread = Thread::create(&_thread_image_decoder, ips);
		}
	end_img_process:

		if (stream_queue.size() > 10) {
			stream_queue.clear();
		}

		// Get some packets
		start_while_time = os->get_ticks_usec();
		while (ppeer->get_available_packet_count() > 0 && (os->get_ticks_usec() - start_while_time) <= send_data_time_us / 2) {
			nothing_happens = false;

			Variant buf;
			ppeer->get_var(buf);

			if (err)
				goto end_recv;

			Ref<GRPacket> pack = GRPacket::create(buf);
			if (pack.is_null()) {
				_log("GRPacket is null", LogLevel::LL_Error);
				continue;
			}

			GRPacket::PacketType type = pack->get_type();

			switch (type) {
				case GRPacket::PacketType::SyncTime: {
					Ref<GRPacketSyncTime> data = pack;
					if (data.is_null()) {
						_log("GRPacketSyncTime is null", LogLevel::LL_Error);
						continue;
					}

					dev->sync_time_client = os->get_ticks_usec();
					dev->sync_time_server = data->get_time();

					break;
				}
				case GRPacket::PacketType::ImageData: {
					Ref<GRPacketImageData> data = pack;
					if (data.is_null()) {
						_log("GRPacketImageData is null", LogLevel::LL_Error);
						continue;
					}

					stream_queue.push_back(data);
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
					dev->_update_avg_ping(os->get_ticks_usec() - prev_ping_sending_time);
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
	dev->connection_mutex->unlock();

	if (_img_thread) {
		Thread::wait_to_finish(_img_thread);
		memdelete(_img_thread);
		_img_thread = nullptr;
	}

	_log("Closing connection");
	con_thread->break_connection = true;
}

void GRClient::_thread_image_decoder(void *p_userdata) {
	ImgProcessingStorage *ips = (ImgProcessingStorage *)p_userdata;
	*ips->_is_processing_img = true;

	GRClient *dev = ips->dev;
	Ref<Image> img(memnew(Image));

	TimeCountInit();
	if (img->load_jpg_from_buffer(ips->tex_data) == OK) {
		dev->call_deferred("_update_texture_from_iamge", img);
		TimeCount("Decode Image Time");
	} else {
		_log("Can't decode JPG image.", LogLevel::LL_Error);
	}
	*ips->_is_processing_img = false;
	delete ips;
}

GRDevice::AuthResult GRClient::_auth_on_server(GRClient *dev, Ref<PacketPeerStream> &ppeer) {
#define wait_packet(_n)                                           \
	uint32_t time = OS::get_singleton()->get_ticks_msec();        \
	while (ppeer->get_available_packet_count() == 0) {            \
		if (OS::get_singleton()->get_ticks_msec() - time > 150) { \
			_log("Timeout: " + str(_n), LogLevel::LL_Debug);      \
			goto timeout;                                         \
		}                                                         \
		OS::get_singleton()->delay_usec(1_ms);                    \
	}
#define packet_error_check(_t)              \
	if (err) {                              \
		_log(_t, LogLevel::LL_Error);       \
		return GRDevice::AuthResult::Error; \
	}

	Ref<StreamPeerTCP> con = ppeer->get_stream_peer();
	String address = CON_ADDRESS(con);

	Error err = OK;
	Variant ret;
	// GET first packet
	wait_packet("first_packet");
	err = ppeer->get_var(ret);
	packet_error_check("Can't get first authorization packet from server. Code: " + str(err));

	if ((int)ret == (int)GRDevice::AuthResult::RefuseConnection) {
		_log("Connection refused", LogLevel::LL_Error);
		return GRDevice::AuthResult::RefuseConnection;
	}
	if ((int)ret == (int)GRDevice::AuthResult::TryToConnect) {
		Dictionary data;
		data["version"] = get_version();
		data["password"] = dev->password;

		// PUT auth data
		err = ppeer->put_var(data);
		packet_error_check("Can't put authorization data to server. Code: " + str(err));

		// GET result
		wait_packet("result");
		err = ppeer->get_var(ret);
		packet_error_check("Can't get final authorization packet from server. Code: " + str(err));

		if ((int)ret == (int)GRDevice::AuthResult::OK) {
			return GRDevice::AuthResult::OK;
		} else {
			GRDevice::AuthResult r = (GRDevice::AuthResult)(int)ret;
			switch (r) {
				case GRDevice::AuthResult::Error:
					_log("Can't connect to server", LogLevel::LL_Error);
					return r;
				case GRDevice::AuthResult::VersionMismatch:
					_log("Version mismatch", LogLevel::LL_Error);
					return r;
				case GRDevice::AuthResult::IncorrectPassword:
					_log("Incorrect password", LogLevel::LL_Error);
					return r;
			}
		}
	}

	return GRDevice::AuthResult::Error;

timeout:
	con->disconnect_from_host();
	_log("Connection timeout. Disconnecting");
	return GRDevice::AuthResult::Timeout;

#undef wait_packet
#undef packet_error_check
}

//////////////////////////////////////////////
///////////// INPUT COLLECTOR ////////////////
//////////////////////////////////////////////

void GRInputCollector::_update_stream_rect() {
	if (!dev || dev->get_status() != (int)GRDevice::WorkingStatus::Working)
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
	if (!parent || (capture_only_when_control_in_focus && !parent->has_focus()) || (dev && dev->get_status() != (int)GRDevice::WorkingStatus::Working) || !dev->is_stream_active()) {
		return;
	}

	_THREAD_SAFE_LOCK_
	if (collected_input_data.size() > 1024 * 1) {
		collected_input_data.resize(0);
	}
	_THREAD_SAFE_UNLOCK_

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
			data.append((int)GRDevice::InputType::InputEventKey);
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
			data.append((int)GRDevice::InputType::InputEventMouseButton);
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
			data.append((int)GRDevice::InputType::InputEventMouseMotion);
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
			data.append((int)GRDevice::InputType::InputEventScreenTouch);
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
			data.append((int)GRDevice::InputType::InputEventScreenDrag);
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
			data.append((int)GRDevice::InputType::InputEventMagnifyGesture);
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
			data.append((int)GRDevice::InputType::InputEventPanGesture);
			data_append_defaults();

			data.append_array(var2bytes(fix(iepg->get_delta())));

			goto end;
		}
	}

	{
		Ref<InputEventJoypadButton> iejb = ie;
		if (iejb.is_valid()) {
			data.append((int)GRDevice::InputType::InputEventJoypadButton);
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
			data.append((int)GRDevice::InputType::InputEventJoypadMotion);
			data_append_defaults();

			data.append_array(uint322bytes(iejm->get_axis()));
			data.append_array(float2bytes(iejm->get_axis_value()));

			goto end;
		}
	}

	{
		Ref<InputEventAction> iea = ie;
		if (iea.is_valid()) {
			data.append((int)GRDevice::InputType::InputEventAction);
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
			data.append((int)GRDevice::InputType::InputEventMIDI);
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

	if (!data.empty()) {
		_THREAD_SAFE_LOCK_
		collected_input_data.append_array(uint322bytes(data.size() + 4)); // block size
		collected_input_data.append_array(data);
		_THREAD_SAFE_UNLOCK_
	}
}

#undef fix
#undef data_append_defaults

void GRInputCollector::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE: {
			parent = cast_to<Control>(get_parent());
			break;
		}
		case NOTIFICATION_PROCESS: {
			_THREAD_SAFE_LOCK_
			auto w = sensors.write();
			w[0] = Input::get_singleton()->get_accelerometer();
			w[1] = Input::get_singleton()->get_gravity();
			w[2] = Input::get_singleton()->get_gyroscope();
			w[3] = Input::get_singleton()->get_magnetometer();
			w.release();
			_THREAD_SAFE_UNLOCK_
			break;
		}
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

void GRInputCollector::set_tex_rect(TextureRect *tr) {
	texture_rect = tr;
}

PoolByteArray GRInputCollector::get_collected_input_data() {
	PoolByteArray res;
	_THREAD_SAFE_LOCK_

	// sensors
	res.append_array(uint322bytes(12 * sensors.size() + 8 + 4 + 1)); // +8 - sensors header, +4 - this value, +1 - type
	res.append((int)GRDevice::InputType::InputDeviceSensors);
	res.append_array(var2bytes(sensors));

	// other input events
	res.append_array(collected_input_data);
	collected_input_data.resize(0);

	_THREAD_SAFE_UNLOCK_
	return res;
}

GRInputCollector::GRInputCollector() {
	_THREAD_SAFE_LOCK_
	parent = nullptr;
	set_process(true);
	set_process_input(true);
	sensors.resize(4);
	_THREAD_SAFE_UNLOCK_
}

GRInputCollector::~GRInputCollector() {
	_THREAD_SAFE_LOCK_
	sensors.resize(0);
	collected_input_data.resize(0);
	if (this_in_client)
		*this_in_client = nullptr;
	_THREAD_SAFE_UNLOCK_
}

#endif // !NO_GODOTREMOTE_CLIENT

GRTextureRect::~GRTextureRect() {
	if (this_in_client)
		*this_in_client = nullptr;
}

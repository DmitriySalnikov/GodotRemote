/* GRServer.cpp */

#ifndef NO_GODOTREMOTE_SERVER

#include "GRServer.h"
#include "GRNotifications.h"
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
	ClassDB::bind_method(D_METHOD("_load_settings"), &GRServer::_load_settings);
	ClassDB::bind_method(D_METHOD("_remove_resize_viewport", "vp"), &GRServer::_remove_resize_viewport);

	ClassDB::bind_method(D_METHOD("get_settings_node"), &GRServer::get_settings_node);
	ClassDB::bind_method(D_METHOD("get_gr_viewport"), &GRServer::get_gr_viewport);

	ClassDB::bind_method(D_METHOD("set_target_send_fps"), &GRServer::set_target_send_fps);
	ClassDB::bind_method(D_METHOD("set_auto_adjust_scale"), &GRServer::set_auto_adjust_scale);
	ClassDB::bind_method(D_METHOD("set_jpg_quality"), &GRServer::set_jpg_quality);
	ClassDB::bind_method(D_METHOD("set_render_scale"), &GRServer::set_render_scale);
	ClassDB::bind_method(D_METHOD("set_password", "password"), &GRServer::set_password);

	ClassDB::bind_method(D_METHOD("get_target_send_fps"), &GRServer::get_target_send_fps);
	ClassDB::bind_method(D_METHOD("get_auto_adjust_scale"), &GRServer::get_auto_adjust_scale);
	ClassDB::bind_method(D_METHOD("get_jpg_quality"), &GRServer::get_jpg_quality);
	ClassDB::bind_method(D_METHOD("get_render_scale"), &GRServer::get_render_scale);
	ClassDB::bind_method(D_METHOD("get_password"), &GRServer::get_password);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_stream_fps"), "set_target_send_fps", "get_target_send_fps");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_adjust_scale"), "set_auto_adjust_scale", "get_auto_adjust_scale");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "jpg_quality"), "set_jpg_quality", "get_jpg_quality");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "render_scale"), "set_render_scale", "get_render_scale");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
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

void GRServer::set_compression_type(int _type) {
	compression_type = (ImageCompressionType)_type;
}

int GRServer::get_compression_type() {
	return (int)compression_type;
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
	target_stream_fps = fps;
}

int GRServer::get_target_send_fps() {
	return target_stream_fps;
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

void GRServer::set_password(String _pass) {
	password = _pass;
}

String GRServer::get_password() {
	return password;
}

GRServer::GRServer() :
		GRDevice() {
	set_name("GodotRemoteServer");
	tcp_server.instance();
	connection_mutex = Mutex::create();
	call_deferred("_load_settings");
	init_server_utils();
}

GRServer::~GRServer() {
	if (get_status() == (int)WorkingStatus::Working) {
		_internal_call_only_deffered_stop();
	}
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

	if (server_thread_listen.is_valid()) {
		server_thread_listen->close_thread();
		server_thread_listen.unref();
	}
	server_thread_listen.instance();
	server_thread_listen->dev = this;
	server_thread_listen->thread_ref = Thread::create(&_thread_listen, server_thread_listen.ptr());

	if (!resize_viewport) {
		resize_viewport = memnew(GRSViewport);
		add_child(resize_viewport);
	}
	set_status(WorkingStatus::Working);

	GRNotifications::add_notification("Godot Remote Server Status", "Server started", NotificationIcon::Success);
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

	if (server_thread_listen.is_valid()) {
		server_thread_listen->close_thread();
		server_thread_listen.unref();
	}

	call_deferred("_remove_resize_viewport", resize_viewport);
	resize_viewport = nullptr;
	set_status(WorkingStatus::Stopped);

	GRNotifications::add_notification("Godot Remote Server Status", "Server stopped", NotificationIcon::Fail);
}

void GRServer::_remove_resize_viewport(Node *node) {
	GRSViewport *vp = cast_to<GRSViewport>(node);
	if (vp && !vp->is_queued_for_deletion()) {
		remove_child(vp);
		memdelete(vp);
	}
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

	if (prev_avg_fps < target_stream_fps - 4) {
		scale -= 0.001f;
	} else if (prev_avg_fps > target_stream_fps - 1) {
		scale += 0.0012f;
	}

	if (scale < 0.1f)
		scale = 0.1f;
	if (scale > 1)
		scale = 1;

end:
	resize_viewport->auto_scale = scale;
	resize_viewport->call_deferred("_update_size");
}

void GRServer::_load_settings() {
	// only updated by server itself
	password = GET_PS(GodotRemote::ps_server_password_name);

	// can be updated by client
	compression_type = (ImageCompressionType)(int)GET_PS(GodotRemote::ps_server_compression_type_name);
	target_stream_fps = GET_PS(GodotRemote::ps_server_stream_fps_name);
	jpg_quality = GET_PS(GodotRemote::ps_server_jpg_quality_name);
	auto_adjust_scale = GET_PS(GodotRemote::ps_server_auto_adjust_scale_name);

	if (resize_viewport && !resize_viewport->is_queued_for_deletion()) {
		resize_viewport->set_rendering_scale(GET_PS(GodotRemote::ps_server_scale_of_sending_stream_name));
	}

	// notification
	const String title = "Server settings updated";
	GRNotificationPanelUpdatable *np = cast_to<GRNotificationPanelUpdatable>(GRNotifications::get_notification(title));
	if (np) {
		np->clear_lines();
	}

	GRNotifications::add_notification_or_update_line(title, "title", "Loaded default server settings");
	GRNotifications::add_notification_or_update_line(title, "compression", "Compression type: " + str((int)compression_type));
	GRNotifications::add_notification_or_update_line(title, "quality", "JPG Quality: " + str(jpg_quality));
	GRNotifications::add_notification_or_update_line(title, "fps", "Stream FPS: " + str(target_stream_fps));
	GRNotifications::add_notification_or_update_line(title, "scale", "Scale of stream: " + str(GET_PS(GodotRemote::ps_server_scale_of_sending_stream_name)));
	GRNotifications::add_notification_or_update_line(title, "auto_scale", "Auto adjust scale: " + str(auto_adjust_scale)); // TODO not completed
}

void GRServer::_update_settings_from_client(const Dictionary settings) {
	Array keys = settings.keys();
	const String title = "Server settings updated";

	GRNotificationPanelUpdatable *np = cast_to<GRNotificationPanelUpdatable>(GRNotifications::get_notification(title));
	if (np) {
		np->remove_updatable_line("title");
	}

	for (int i = 0; i < settings.size(); i++) {
		Variant key = keys[i];
		Variant value = settings[key];
		_log("Trying to set server setting from client with key: " + str((int)key) + " and value: " + str(value), LogLevel::LL_Debug);

		if (key.get_type() == Variant::INT) {
			GodotRemote::TypesOfServerSettings k = (GodotRemote::TypesOfServerSettings)(int)key;
			switch (k) {
				case GodotRemote::TypesOfServerSettings::USE_INTERNAL_SERVER_SETTINGS:
					if ((bool)value) {
						call_deferred("_load_settings");
						return;
					}
					break;
				case GodotRemote::TypesOfServerSettings::COMPRESSION_TYPE: {
					if (compression_type != (ImageCompressionType)(int)value) {
						GRNotifications::add_notification_or_update_line(title, "compression", "Compression type: " + str((int)value));
						set_compression_type(value);
					}
					break;
				}
				case GodotRemote::TypesOfServerSettings::JPG_QUALITY: {
					if (jpg_quality != (int)value) {
						GRNotifications::add_notification_or_update_line(title, "quality", "JPG Quality: " + str((int)value));
						set_jpg_quality(value);
					}
					break;
				}
				case GodotRemote::TypesOfServerSettings::SEND_FPS: {
					if (target_stream_fps != (int)value) {
						GRNotifications::add_notification_or_update_line(title, "fps", "Stream FPS: " + str((int)value));
						set_target_send_fps(value);
					}
					break;
				}
				case GodotRemote::TypesOfServerSettings::RENDER_SCALE: {
					if (resize_viewport) {
						if (resize_viewport->get_rendering_scale() != (float)value) {
							GRNotifications::add_notification_or_update_line(title, "scale", "Scale of stream: " + str(value));
							resize_viewport->set_rendering_scale(value);
						}
					}
					break;
				}
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
	Ref<ListenerThreadParams> this_thread_info = (ListenerThreadParams *)p_userdata;
	GRServer *dev = this_thread_info->dev;
	Ref<TCP_Server> srv = dev->tcp_server;
	OS *os = OS::get_singleton();
	Ref<ConnectionThreadParams> connection_thread_info;
	Error err = OK;
	bool listening_error_notification_shown = false;

	while (!this_thread_info->stop_thread) {
		if (!srv->is_listening()) {
			err = srv->listen(dev->port);

			if (err != OK) {
				switch (err) {
					case ERR_UNAVAILABLE: {
						String txt = "Socket listening unavailable";
						_log(txt, LogLevel::LL_Error);
						if (!listening_error_notification_shown)
							GRNotifications::add_notification("Can't start listening", txt, NotificationIcon::Error, true, 1.25f);
						break;
					}
					case ERR_ALREADY_IN_USE: {
						String txt = "Socket already in use";
						_log(txt, LogLevel::LL_Error);
						if (!listening_error_notification_shown)
							GRNotifications::add_notification("Can't start listening", txt, NotificationIcon::Error, true, 1.25f);
						break;
					}
					case ERR_INVALID_PARAMETER: {
						String txt = "Invalid listening address";
						_log(txt, LogLevel::LL_Error);
						if (!listening_error_notification_shown)
							GRNotifications::add_notification("Can't start listening", txt, NotificationIcon::Error, true, 1.25f);
						break;
					}
					case ERR_CANT_CREATE: {
						String txt = "Can't bind listener";
						_log(txt, LogLevel::LL_Error);
						if (!listening_error_notification_shown)
							GRNotifications::add_notification("Can't start listening", txt, NotificationIcon::Error, true, 1.25f);
						break;
					}
					case FAILED: {
						String txt = "Failed to start listening";
						_log(txt, LogLevel::LL_Error);
						if (!listening_error_notification_shown)
							GRNotifications::add_notification("Can't start listening", txt, NotificationIcon::Error, true, 1.25f);
						break;
					}
				}

				listening_error_notification_shown = true;
				os->delay_usec(1000_ms);
				continue;
			} else {
				_log("Start listening port " + str(dev->port), LogLevel::LL_Normal);
				GRNotifications::add_notification("Start listening", "Start listening on port: " + str(dev->port), NotificationIcon::Success, true);
			}
		}
		listening_error_notification_shown = false;

		if (connection_thread_info.is_valid()) {
			if (connection_thread_info->ppeer.is_null()) {
				connection_thread_info->break_connection = true;
			}

			if (connection_thread_info->finished || connection_thread_info->break_connection) {
				_log("Waiting connection thread...", LogLevel::LL_Debug);
				connection_thread_info->close_thread();
				connection_thread_info.unref();
			}
		}

		if (srv->is_connection_available()) {
			Ref<StreamPeerTCP> con = srv->take_connection();
			con->set_no_delay(true);
			String address = CON_ADDRESS(con);

			Ref<PacketPeerStream> ppeer(memnew(PacketPeerStream));
			ppeer->set_stream_peer(con);
			ppeer->set_output_buffer_max_size(compress_buffer.size());

			if (connection_thread_info.is_null()) {
				Dictionary ret_data;
				GRDevice::AuthResult res = _auth_client(dev, ppeer, ret_data);
				String dev_id = "";

				if (ret_data.has("id")) {
					dev_id = ret_data["id"];
				}

				switch (res) {
					case GRDevice::AuthResult::OK:
						connection_thread_info.instance();
						connection_thread_info->device_id = dev_id;

						connection_thread_info->dev = dev;
						connection_thread_info->ppeer = ppeer;
						connection_thread_info->thread_ref = Thread::create(&_thread_connection, connection_thread_info.ptr());
						_log("New connection from " + address);

						GRNotifications::add_notification("Connected", "Client connected: " + address + "\nDevice ID: " + connection_thread_info->device_id, NotificationIcon::Success);
						break;

					case GRDevice::AuthResult::VersionMismatch:
					case GRDevice::AuthResult::IncorrectPassword:
					case GRDevice::AuthResult::Error:
					case GRDevice::AuthResult::RefuseConnection:
						continue;
					default:
						_log("Unknown error code. Disconnecting. " + address);
						continue;
				}

			} else {
				Dictionary ret_data;
				GRDevice::AuthResult res = _auth_client(dev, ppeer, ret_data, true);
			}
		} else {
			_log("Waiting...", LogLevel::LL_Debug);
			os->delay_usec(33_ms);
		}
	}

	if (connection_thread_info.is_valid()) {
		_log("Closing connection thread...", LogLevel::LL_Debug);
		connection_thread_info->break_connection = true;
		connection_thread_info->close_thread();
		connection_thread_info.unref();
	}

	dev->tcp_server->stop();
	this_thread_info->finished = true;
}

void GRServer::_thread_connection(void *p_userdata) {
	Ref<ConnectionThreadParams> thread_info = (ConnectionThreadParams *)p_userdata;
	Ref<StreamPeerTCP> connection = thread_info->ppeer->get_stream_peer();
	Ref<PacketPeerStream> ppeer = thread_info->ppeer;
	GRServer *dev = thread_info->dev;
	dev->_reset_counters();

	GodotRemote *gr = GodotRemote::get_singleton();
	OS *os = OS::get_singleton();
	Error err = Error::OK;

	String address = CON_ADDRESS(connection);
	Thread::set_name("GR_connection " + address);

	Ref<ImgProcessingStorage> ips;

	uint64_t time64 = os->get_ticks_usec();
	uint64_t prev_send_image_time = time64;
	uint64_t prev_ping_sending_time = time64;
	uint64_t prev_process_image_time = time64;
	//uint64_t prev_send_sync_time = time64;

	bool ping_sended = false;
	bool time_synced = false;

	while (!thread_info->break_connection && connection.is_valid() && !connection->is_queued_for_deletion() && connection->is_connected_to_host()) {
		bool nothing_happens = true;
		uint64_t send_data_time_us = (1000000 / dev->target_stream_fps);

		///////////////////////////////////////////////////////////////////
		// SENDING

		// TIME SYNC
		//time = os->get_ticks_usec();
		//if (time - prev_send_sync_time > 1000_ms) {
		if (!time_synced) {
			time_synced = true;
			nothing_happens = false;
			//prev_send_sync_time = time;
			Ref<GRPacketSyncTime> pack(memnew(GRPacketSyncTime));
			err = ppeer->put_var(pack->get_data());
			if (err) {
				_log("Can't send sync time data! Code: " + str(err), LogLevel::LL_Error);
				goto end_send;
			}
		}

		// IMAGE
		time64 = os->get_ticks_usec();
		if (time64 - prev_process_image_time >= send_data_time_us && ips.is_null() &&
				dev->resize_viewport && !dev->resize_viewport->is_queued_for_deletion()) {
			nothing_happens = false;
			prev_process_image_time = time64;

			ips.instance();
			ips->compression_type = dev->compression_type;
			ips->start(dev->resize_viewport->get_texture(), dev->jpg_quality);
		}
		// if image compressed to jpg and data is ready
		if (ips.is_valid() && ips->is_new_data) {
			nothing_happens = false;

			Ref<GRPacketImageData> pack(memnew(GRPacketImageData));

			if (ips->ret_data.empty()) {
				_log("Image data is empty!", LogLevel::LL_Error);
				ips->close();
				ips.unref();
				goto end_send;
			}

			pack->set_compression_type((int)ips->compression_type);
			pack->set_size(Size2(ips->width, ips->height));
			pack->set_format(ips->format);
			pack->set_image_data(ips->ret_data);
			pack->set_start_time(os->get_ticks_usec());
			pack->set_frametime(send_data_time_us);

			err = ppeer->put_var(pack->get_data());
			ips->close();
			ips.unref();

			// avg fps
			dev->_update_avg_fps(time64 - prev_send_image_time);
			dev->_adjust_viewport_scale();
			prev_send_image_time = time64;

			if (err) {
				_log("Can't send image data! Code: " + str(err), LogLevel::LL_Error);
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
	end_send:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after sending!", LogLevel::LL_Error);
			GRNotifications::add_notification("Error", "Lost connection after sending data!", NotificationIcon::Error);
			continue;
		}

		///////////////////////////////////////////////////////////////////
		// RECEIVING
		uint64_t recv_start_time = os->get_ticks_usec();
		while (connection->is_connected_to_host() && ppeer->get_available_packet_count() > 0 && (os->get_ticks_usec() - recv_start_time) < send_data_time_us / 2) {
			nothing_happens = false;
			Variant res;
			err = ppeer->get_var(res);

			if (err) {
				_log("Can't receive packet!", LogLevel::LL_Error);
				continue;
			}

			//_log(str_arr((PoolByteArray)res, true));
			Ref<GRPacket> pack = GRPacket::create(res);
			if (pack.is_null()) {
				_log("Received packet was NULL", LogLevel::LL_Error);
				continue;
			}

			PacketType type = pack->get_type();
			//_log((int)type);

			switch (type) {
				case PacketType::SyncTime: {
					ERR_PRINT("NOT IMPLEMENTED");
					break;
				}
				case PacketType::ImageData: {
					ERR_PRINT("NOT IMPLEMENTED");
					break;
				}
				case PacketType::InputData: {
					Ref<GRPacketInputData> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketInputData", LogLevel::LL_Error);
						break;
					}

					for (int i = 0; i < data->get_inputs_count(); i++) {
						Ref<GRInputData> id = data->get_input_data(i);
						InputType ev_type = id->get_type();

						if (ev_type >= InputType::InputEvent) {
							Ref<GRInputDataEvent> ied = id;
							if (ied.is_null()) {
								_log("GRInputDataEvent is null", LogLevel::LL_Error);
								continue;
							}

							Ref<InputEvent> ev = ied->construct_event();
							if (ev.is_valid()) {
								Input::get_singleton()->call_deferred("parse_input_event", ev);
							}
						} else {
							switch (ev_type) {
								case InputType::None: {
									_log("Not valid input type! 0");
									break;
								}
								case InputType::InputDeviceSensors: {
									Ref<GRInputDeviceSensorsData> sd = id;
									if (sd.is_null()) {
										_log("GRInputDeviceSensorsData is null", LogLevel::LL_Error);
										continue;
									}

									auto s = sd->get_sensors();
									set_accelerometer(s[0]);
									set_gravity(s[1]);
									set_gyroscope(s[2]);
									set_magnetometer(s[3]);
									break;
								}
								default: {
									_log("Not supported input type! " + str((int)ev_type), LogLevel::LL_Error);
									continue;
									break;
								}
							}
						}
					}
					break;
				}
				case PacketType::ServerSettings: {
					Ref<GRPacketServerSettings> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketServerSettings", LogLevel::LL_Error);
						break;
					}
					dev->_update_settings_from_client(data->get_settings());
					break;
				}
				case PacketType::ServerSettingsRequest: {
					ERR_PRINT("NOT IMPLEMENTED");
					break;
				}
				case PacketType::Ping: {
					Ref<GRPacketPong> pack(memnew(GRPacketPong));
					err = ppeer->put_var(pack->get_data());

					if (err) {
						_log("Send pong failed with code: " + str(err), LogLevel::LL_Error);
						goto end_recv;
					}
					break;
				}
				case PacketType::Pong: {
					dev->_update_avg_ping(os->get_ticks_usec() - prev_ping_sending_time);
					ping_sended = false;
					break;
				}
				default: {
					_log("Not supported packet type! " + str((int)type), LogLevel::LL_Warning);
					break;
				}
			}
		}
	end_recv:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after receiving!", LogLevel::LL_Error);
			GRNotifications::add_notification("Error", "Lost connection after receiving data!", NotificationIcon::Error);
			continue;
		}

		if (nothing_happens) // for less cpu using
			os->delay_usec(1_ms);
	}

	_log("Closing connection thread with address: " + address, LogLevel::LL_Debug);

	if (connection->is_connected_to_host()) {
		GRNotifications::add_notification("Disconnected", "Closing connection with " + address, NotificationIcon::Fail, false);
	} else {
		GRNotifications::add_notification("Disconnected", "Client disconnected: " + address, NotificationIcon::Fail, false);
	}

	if (ips.is_valid()) {
		ips->close();
		ips.unref();
	}

	if (ppeer.is_valid()) {
		ppeer.unref();
	}
	thread_info->ppeer.unref();
	thread_info->break_connection = true;
	dev->call_deferred("_load_settings");

	thread_info->finished = true;
}

GRServer::AuthResult GRServer::_auth_client(GRServer *dev, Ref<PacketPeerStream> &ppeer, Dictionary &ret_data, bool refuse_connection) {
// _v - variable definition, _n - dict key, _c - fail condition, _e - error message, _r - return value on fail condition
#define dict_get(_t, _v, _n, _c, _e, _r) \
	_t _v;                               \
	if (dict.has(_n))                    \
		_v = (_t)dict[_n];               \
	else                                 \
		goto error_dict;                 \
	if (_c) {                            \
		ppeer->put_var((int)_r);         \
		_log(_e, LogLevel::LL_Debug);    \
		return _r;                       \
	}
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
		_log(_t, LogLevel::LL_Debug);       \
		return GRDevice::AuthResult::Error; \
	}

	Ref<StreamPeerTCP> con = ppeer->get_stream_peer();
	String address = CON_ADDRESS(con);

	Error err = OK;
	Variant res;
	if (!refuse_connection) {
		// PUT client can try to connect
		err = ppeer->put_var((int)GRDevice::AuthResult::TryToConnect);
		packet_error_check("Can't send authorization init packet to " + address + ". Code: " + str(err));

		// GET auth data
		wait_packet("auth_data");
		err = ppeer->get_var(res);
		packet_error_check("Can't get authorization data from client to " + address + ". Code: " + str(err));

		Dictionary dict = res;
		if (dict.empty()) {
			goto error_dict;
		} else {
			dict_get(String, id, "id",
					id.empty(), "Device ID field is empty or does not exists. " + address,
					GRDevice::AuthResult::VersionMismatch);

			ret_data["id"] = id;

			dict_get(PoolByteArray, ver, "version",
					ver.empty(), "Version field is empty or does not exists. " + address,
					GRDevice::AuthResult::VersionMismatch);

			if (!validate_version(ver)) {
				_log("Version mismatch", LogLevel::LL_Error);
				return GRDevice::AuthResult::VersionMismatch;
			}

			if (!dev->password.empty()) {
				dict_get(String, password, "password",
						password != dev->password, "Incorrect password. " + address,
						GRDevice::AuthResult::IncorrectPassword);
			}
		}

		// PUT auth ok
		err = ppeer->put_var((int)GRDevice::AuthResult::OK);
		packet_error_check("Can't send final authorization packet from client to " + address + ". Code: " + str(err));

		return GRDevice::AuthResult::OK;

	error_dict:
		_log("Got invalid authorization data from client. " + address);
		return GRDevice::AuthResult::Error;

	} else {
		// PUT refuse connection
		Error err = con->put_data((uint8_t *)GRDevice::AuthResult::RefuseConnection, 1);
		return GRDevice::AuthResult::RefuseConnection;
	}
timeout:
	con->disconnect_from_host();
	_log("Connection timeout. Refusing " + address);
	return GRDevice::AuthResult::Timeout;

#undef dict_get
#undef wait_packet
#undef packet_error_check
}

//////////////////////////////////////////////
////////// ImgProcessingStorage //////////////
//////////////////////////////////////////////
void GRServer::ImgProcessingStorage::_get_texture_data_from_main_thread() {
	TimeCountInit();

	_THREAD_SAFE_LOCK_;
	img = tex->get_data(); // extremely slow
	TimeCount("Get image data from VisualServer");

	if (!img->empty()) {
		_thread_process = Thread::create(&GRServer::ImgProcessingStorage::_processing_thread, this);
	} else {
		finished = true;
		is_new_data = true;
		_log("Can't copy viewport image data", LogLevel::LL_Error);
	}
	_THREAD_SAFE_UNLOCK_
}

void GRServer::ImgProcessingStorage::_processing_thread(void *p_user) {
	Ref<ImgProcessingStorage> ips = (ImgProcessingStorage *)p_user;
	if (ips.is_null())
		return;

	ips->width = ips->img->get_width();
	ips->height = ips->img->get_height();

	TimeCountInit();
	ips->format = ips->img->get_format();
	if (!(ips->format == Image::FORMAT_RGBA8 || ips->format == Image::FORMAT_RGB8)) {
		ips->img->convert(Image::FORMAT_RGB8);
		ips->format = ips->img->get_format();

		if (ips->format != Image::FORMAT_RGB8) {
			_log("Can't convert stream image to RGB8.", LogLevel::LL_Error);
			GRNotifications::add_notification("Stream Error", "Can't convert stream image to RGB8.", NotificationIcon::Error);
			goto end;
		}
		TimeCount("Image Convert");
	}
	ips->bytes_in_color = ips->img->get_format() == Image::FORMAT_RGB8 ? 3 : 4;

	if (ips->img->get_data().empty())
		goto end;

	ips->_THREAD_SAFE_LOCK_;
	switch (ips->compression_type) {
		case GRUtils::ImageCompressionType::Uncompressed: {
			ips->ret_data = ips->img->get_data();
			break;
		}
		case GRUtils::ImageCompressionType::JPG: {
			if (!ips->img->empty()) {
				Error err = compress_jpg(ips->ret_data, ips->img->get_data(), ips->width, ips->height, ips->bytes_in_color, ips->jpg_quality, GRUtils::SUBSAMPLING_H2V2);
				if (err) {
					_log("Can't compress stream image JPG. Code: " + str(err), LogLevel::LL_Error);
					GRNotifications::add_notification("Stream Error", "Can't compress stream image to JPG. Code: " + str(err), NotificationIcon::Error);
				}
			}
			break;
		}
		case GRUtils::ImageCompressionType::PNG: {
			ips->ret_data = ips->img->save_png_to_buffer();
			if (ips->ret_data.empty()) {
				_log("Can't compress stream image to PNG.", LogLevel::LL_Error);
				GRNotifications::add_notification("Stream Error", "Can't compress stream image to PNG.", NotificationIcon::Error);
			}
			break;
		}
		default:
			_log("Not implemented compression type: " + str((int)ips->compression_type), LogLevel::LL_Error);
			break;
	}
	ips->_THREAD_SAFE_UNLOCK_;
end:

	ips->is_new_data = true;
	ips->finished = true;
}

void GRServer::ImgProcessingStorage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_get_texture_data_from_main_thread"), &GRServer::ImgProcessingStorage::_get_texture_data_from_main_thread);
}

void GRServer::ImgProcessingStorage::start(Ref<ViewportTexture> _tex, int _jpg_quality) {
	jpg_quality = _jpg_quality;
	tex = _tex;

	call_deferred("_get_texture_data_from_main_thread");
}

void GRServer::ImgProcessingStorage::close() {
	if (_thread_process) {
		Thread::wait_to_finish(_thread_process);
		memdelete(_thread_process);
		_thread_process = nullptr;
	}
}

GRServer::ImgProcessingStorage::~ImgProcessingStorage() {
	_THREAD_SAFE_LOCK_
	close();
	img.unref();
	tex.unref();
	ret_data.resize(0);
	_THREAD_SAFE_UNLOCK_
}

//////////////////////////////////////////////
////////////// GRSViewport ///////////////////
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
	call_deferred("_update_size");
}

float GRSViewport::get_rendering_scale() {
	return rendering_scale;
}

GRSViewport::GRSViewport() {
	set_name("GRSViewport");

	rendering_scale = GET_PS(GodotRemote::ps_server_scale_of_sending_stream_name);

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

/* GRClient.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRClient.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRResources.h"

#ifndef GDNATIVE_LIBRARY

#include "core/input_map.h"
#include "core/io/file_access_pack.h"
#include "core/io/ip.h"
#include "core/io/resource_loader.h"
#include "core/io/tcp_server.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/os/input_event.h"
#include "core/os/thread_safe.h"
#include "main/input_default.h"
#include "scene/gui/control.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/material.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/texture.h"

#else

#include <InputMap.hpp>
#include <IP.hpp>
#include <ResourceLoader.hpp>
#include <TCP_Server.hpp>
#include <Directory.hpp>
#include <File.hpp>
#include <Thread.hpp>
#include <Input.hpp>
#include <Control.hpp>
#include <Node.hpp>
#include <SceneTree.hpp>
#include <Viewport.hpp>
#include <Material.hpp>
#include <PackedScene.hpp>
#include <Texture.hpp>
#include <RandomNumberGenerator.hpp>
#include <GlobalConstants.hpp>
#include <ProjectSettings.hpp>
using namespace godot;


#define BUTTON_WHEEL_UP	   GlobalConstants::BUTTON_WHEEL_UP
#define BUTTON_WHEEL_DOWN  GlobalConstants::BUTTON_WHEEL_DOWN
#define BUTTON_WHEEL_LEFT  GlobalConstants::BUTTON_WHEEL_LEFT
#define BUTTON_WHEEL_RIGHT GlobalConstants::BUTTON_WHEEL_RIGHT
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY
void GRClient::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_update_texture_from_iamge", "image"), &GRClient::_update_texture_from_iamge);
	ClassDB::bind_method(D_METHOD("_update_stream_texture_state", "state"), &GRClient::_update_stream_texture_state);
	ClassDB::bind_method(D_METHOD("_force_update_stream_viewport_signals"), &GRClient::_force_update_stream_viewport_signals);
	ClassDB::bind_method(D_METHOD("_viewport_size_changed"), &GRClient::_viewport_size_changed);
	ClassDB::bind_method(D_METHOD("_load_custom_input_scene", "_data"), &GRClient::_load_custom_input_scene);
	ClassDB::bind_method(D_METHOD("_remove_custom_input_scene"), &GRClient::_remove_custom_input_scene);

	ClassDB::bind_method(D_METHOD("send_packet", "packet"), &GRClient::send_packet);

	ClassDB::bind_method(D_METHOD("set_control_to_show_in", "control_node", "position_in_node"), &GRClient::set_control_to_show_in, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_texture", "texture"), &GRClient::set_custom_no_signal_texture);
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_vertical_texture", "texture"), &GRClient::set_custom_no_signal_vertical_texture);
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_material", "material"), &GRClient::set_custom_no_signal_material);
	ClassDB::bind_method(D_METHOD("set_address_port", "ip", "port"), &GRClient::set_address_port);
	ClassDB::bind_method(D_METHOD("set_address", "ip"), &GRClient::set_address);
	ClassDB::bind_method(D_METHOD("set_server_setting", "setting", "value"), &GRClient::set_server_setting);
	ClassDB::bind_method(D_METHOD("disable_overriding_server_settings"), &GRClient::disable_overriding_server_settings);

	ClassDB::bind_method(D_METHOD("get_address"), &GRClient::get_address);
	ClassDB::bind_method(D_METHOD("is_stream_active"), &GRClient::is_stream_active);
	ClassDB::bind_method(D_METHOD("is_connected_to_host"), &GRClient::is_connected_to_host);

	ADD_SIGNAL(MethodInfo("stream_state_changed", PropertyInfo(Variant::INT, "state", PROPERTY_HINT_ENUM)));
	ADD_SIGNAL(MethodInfo("connection_state_changed", PropertyInfo(Variant::BOOL, "is_connected")));
	ADD_SIGNAL(MethodInfo("mouse_mode_changed", PropertyInfo(Variant::INT, "mouse_mode")));
	ADD_SIGNAL(MethodInfo("server_settings_received", PropertyInfo(Variant::DICTIONARY, "settings")));

	// SETGET
	ClassDB::bind_method(D_METHOD("set_capture_on_focus", "val"), &GRClient::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD("set_capture_when_hover", "val"), &GRClient::set_capture_when_hover);
	ClassDB::bind_method(D_METHOD("set_capture_pointer", "val"), &GRClient::set_capture_pointer);
	ClassDB::bind_method(D_METHOD("set_capture_input", "val"), &GRClient::set_capture_input);
	ClassDB::bind_method(D_METHOD("set_connection_type", "type"), &GRClient::set_connection_type);
	ClassDB::bind_method(D_METHOD("set_target_send_fps", "fps"), &GRClient::set_target_send_fps);
	ClassDB::bind_method(D_METHOD("set_stretch_mode", "mode"), &GRClient::set_stretch_mode);
	ClassDB::bind_method(D_METHOD("set_texture_filtering", "is_filtered"), &GRClient::set_texture_filtering);
	ClassDB::bind_method(D_METHOD("set_password", "password"), &GRClient::set_password);
	ClassDB::bind_method(D_METHOD("set_device_id", "id"), &GRClient::set_device_id);
	ClassDB::bind_method(D_METHOD("set_viewport_orientation_syncing", "is_syncing"), &GRClient::set_viewport_orientation_syncing);
	ClassDB::bind_method(D_METHOD("set_viewport_aspect_ratio_syncing", "is_syncing"), &GRClient::set_viewport_aspect_ratio_syncing);
	ClassDB::bind_method(D_METHOD("set_server_settings_syncing", "is_syncing"), &GRClient::set_server_settings_syncing);

	ClassDB::bind_method(D_METHOD("is_capture_on_focus"), &GRClient::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD("is_capture_when_hover"), &GRClient::is_capture_when_hover);
	ClassDB::bind_method(D_METHOD("is_capture_pointer"), &GRClient::is_capture_pointer);
	ClassDB::bind_method(D_METHOD("is_capture_input"), &GRClient::is_capture_input);
	ClassDB::bind_method(D_METHOD("get_connection_type"), &GRClient::get_connection_type);
	ClassDB::bind_method(D_METHOD("get_target_send_fps"), &GRClient::get_target_send_fps);
	ClassDB::bind_method(D_METHOD("get_stretch_mode"), &GRClient::get_stretch_mode);
	ClassDB::bind_method(D_METHOD("get_texture_filtering"), &GRClient::get_texture_filtering);
	ClassDB::bind_method(D_METHOD("get_password"), &GRClient::get_password);
	ClassDB::bind_method(D_METHOD("get_device_id"), &GRClient::get_device_id);
	ClassDB::bind_method(D_METHOD("is_viewport_orientation_syncing"), &GRClient::is_viewport_orientation_syncing);
	ClassDB::bind_method(D_METHOD("is_viewport_aspect_ratio_syncing"), &GRClient::is_viewport_aspect_ratio_syncing);
	ClassDB::bind_method(D_METHOD("is_server_settings_syncing"), &GRClient::is_server_settings_syncing);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), "set_capture_on_focus", "is_capture_on_focus");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), "set_capture_when_hover", "is_capture_when_hover");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_pointer"), "set_capture_pointer", "is_capture_pointer");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_input"), "set_capture_input", "is_capture_input");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_type", PROPERTY_HINT_ENUM, "WiFi,ADB"), "set_connection_type", "get_connection_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_send_fps", PROPERTY_HINT_RANGE, "1,1000"), "set_target_send_fps", "get_target_send_fps");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stretch_mode", PROPERTY_HINT_ENUM, "Fill,Keep Aspect"), "set_stretch_mode", "get_stretch_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "texture_filtering"), "set_texture_filtering", "get_texture_filtering");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "device_id"), "set_device_id", "get_device_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "viewport_orientation_syncing"), "set_viewport_orientation_syncing", "is_viewport_orientation_syncing");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "viewport_aspect_ratio_syncing"), "set_viewport_aspect_ratio_syncing", "is_viewport_aspect_ratio_syncing");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "server_settings_syncing"), "set_server_settings_syncing", "is_server_settings_syncing");

	BIND_ENUM_CONSTANT(CONNECTION_ADB);
	BIND_ENUM_CONSTANT(CONNECTION_WiFi);

	BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT);
	BIND_ENUM_CONSTANT(STRETCH_FILL);

	BIND_ENUM_CONSTANT(STREAM_NO_SIGNAL);
	BIND_ENUM_CONSTANT(STREAM_ACTIVE);
	BIND_ENUM_CONSTANT(STREAM_NO_IMAGE);
}

#else


void GRClient::_register_methods() {
}

#endif

void GRClient::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_EXIT_TREE:
		case NOTIFICATION_PREDELETE: {
			is_deleting = true;
			if (get_status() == (int)WorkingStatus::Working) {
				_internal_call_only_deffered_stop();
				set_control_to_show_in(nullptr);
			}
			break;
		}
	}
}

GRClient::GRClient() :
		GRDevice() {
	set_name("GodotRemoteClient");

#ifndef GDNATIVE_LIBRARY
	Math::randomize();
	device_id = str(Math::randd() * Math::rand()).md5_text().substr(0, 6);
#else
	RandomNumberGenerator* rng = RandomNumberGenerator::_new();
	rng->randomize();
	device_id = str(rng->randf() * rng->randf()).md5_text().substr(0, 6);
	rng->free();
#endif


	send_queue_mutex = Mutex_create();
	connection_mutex = Mutex_create();

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_image.instance();
	GetPoolVectorFromBin(tmp_no_signal, GRResources::Bin_NoSignalPNG);
	no_signal_image->load_png_from_buffer(tmp_no_signal);

	no_signal_vertical_image.instance();
	GetPoolVectorFromBin(tmp_no_signal_vert, GRResources::Bin_NoSignalVerticalPNG);
	no_signal_vertical_image->load_png_from_buffer(tmp_no_signal_vert);

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

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_mat.unref();
	no_signal_image.unref();
	no_signal_vertical_image.unref();
#endif
}

void GRClient::_internal_call_only_deffered_start() {
	switch ((WorkingStatus)get_status()) {
		case WorkingStatus::Working:
			ERR_FAIL_MSG("Can't start already working GodotRemote Client");
		case WorkingStatus::Starting:
			ERR_FAIL_MSG("Can't start already starting GodotRemote Client");
		case WorkingStatus::Stopping:
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
	thread_connection->thread_ref = Thread_create(GRClient, _thread_connection, thread_connection.ptr(), this);

	call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
	set_status(WorkingStatus::Working);
}

void GRClient::_internal_call_only_deffered_stop() {
	switch ((WorkingStatus)get_status()) {
		case WorkingStatus::Stopped:
			ERR_FAIL_MSG("Can't stop already stopped GodotRemote Client");
		case WorkingStatus::Stopping:
			ERR_FAIL_MSG("Can't stop already stopping GodotRemote Client");
		case WorkingStatus::Starting:
			ERR_FAIL_MSG("Can't stop starting GodotRemote Client");
	}

	_log("Stopping GodotRemote client", LogLevel::LL_Debug);
	set_status(WorkingStatus::Stopping);
	_remove_custom_input_scene();

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

	call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
	set_status(WorkingStatus::Stopped);
}

void GRClient::set_control_to_show_in(Control *ctrl, int position_in_node) {
	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		tex_shows_stream->dev = nullptr;
		tex_shows_stream->queue_del();
		tex_shows_stream = nullptr;
	}
	if (input_collector && !input_collector->is_queued_for_deletion()) {
		input_collector->dev = nullptr;
		input_collector->queue_del();
		input_collector = nullptr;
	}
	if (control_to_show_in && !control_to_show_in->is_queued_for_deletion() &&
			control_to_show_in->is_connected("resized", this, "_viewport_size_changed")) {
		control_to_show_in->disconnect("resized", this, "_viewport_size_changed");
	}

	_remove_custom_input_scene();

	control_to_show_in = ctrl;

	if (control_to_show_in && !control_to_show_in->is_queued_for_deletion()) {
		control_to_show_in->connect("resized", this, "_viewport_size_changed");

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

		input_collector->set_tex_rect(tex_shows_stream);
		input_collector->dev = this;
		input_collector->this_in_client = &input_collector;

		signal_connection_state = StreamState::STREAM_ACTIVE; // force execute update function
		call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
		call_deferred("_force_update_stream_viewport_signals"); // force update if client connected faster than scene loads
	}
}

void GRClient::set_custom_no_signal_texture(Ref<Texture> custom_tex) {
	custom_no_signal_texture = custom_tex;
	call_deferred("_update_stream_texture_state", signal_connection_state);
}

void GRClient::set_custom_no_signal_vertical_texture(Ref<Texture> custom_tex) {
	custom_no_signal_vertical_texture = custom_tex;
	call_deferred("_update_stream_texture_state", signal_connection_state);
}

void GRClient::set_custom_no_signal_material(Ref<Material> custom_mat) {
	custom_no_signal_material = custom_mat;
	call_deferred("_update_stream_texture_state", signal_connection_state);
}

bool GRClient::is_capture_on_focus() {
	if (input_collector && !input_collector->is_queued_for_deletion())
		return input_collector->is_capture_on_focus();
	return false;
}

void GRClient::set_capture_on_focus(bool value) {
	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_on_focus(value);
}

bool GRClient::is_capture_when_hover() {
	if (input_collector && !input_collector->is_queued_for_deletion())
		return input_collector->is_capture_when_hover();
	return false;
}

void GRClient::set_capture_when_hover(bool value) {
	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_when_hover(value);
}

bool GRClient::is_capture_pointer() {
	if (input_collector && !input_collector->is_queued_for_deletion())
		return input_collector->is_capture_pointer();
	return false;
}

void GRClient::set_capture_pointer(bool value) {
	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_pointer(value);
}

bool GRClient::is_capture_input() {
	if (input_collector && !input_collector->is_queued_for_deletion())
		return input_collector->is_capture_input();
	return false;
}

void GRClient::set_capture_input(bool value) {
	if (input_collector && !input_collector->is_queued_for_deletion())
		input_collector->set_capture_input(value);
}

void GRClient::set_connection_type(ConnectionType type) {
	con_type = type;
}

ConnectionType GRClient::get_connection_type() {
	return con_type;
}

void GRClient::set_target_send_fps(int fps) {
	ERR_FAIL_COND(fps <= 0);
	send_data_fps = fps;
}

int GRClient::get_target_send_fps() {
	return send_data_fps;
}

void GRClient::set_stretch_mode(StretchMode stretch) {
	stretch_mode = stretch;
	call_deferred("_update_stream_texture_state", signal_connection_state);
}

StretchMode GRClient::get_stretch_mode() {
	return stretch_mode;
}

void GRClient::set_texture_filtering(bool is_filtering) {
	is_filtering_enabled = is_filtering;
}

bool GRClient::get_texture_filtering() {
	return is_filtering_enabled;
}

StreamState GRClient::get_stream_state() {
	return signal_connection_state;
}

void GRClient::send_packet(Ref<GRPacket> packet) {
	ERR_FAIL_COND(packet.is_null());

	send_queue_mutex->lock();
	send_queue.push_back(packet);
	send_queue_mutex->unlock();
}

bool GRClient::is_stream_active() {
	return signal_connection_state;
}

String GRClient::get_address() {
	return (String)server_address;
}

bool GRClient::set_address(String ip) {
	return set_address_port(ip, port);
}

bool GRClient::set_address_port(String ip, uint16_t _port) {
	bool all_ok = false;

#ifndef GDNATIVE_LIBRARY
	IP_Address adr;
#else
	String adr;
#endif

	if (ip.is_valid_ip_address()) {
		adr = ip;
		if (adr.is_valid_ip()) {
			server_address = ip;
			port = _port;
			restart();
			all_ok = true;
		}
		else {
			_log("Address is invalid: " + ip, LogLevel::LL_Error);
			GRNotifications::add_notification("Resolve Address Error", "Address is invalid: " + ip, NotificationIcon::_Error);
		}
	}
	else {
		adr = IP::get_singleton()->resolve_hostname(adr);
		if (adr.is_valid_ip()) {
			_log("Resolved address for " + ip + "\n" + adr, LogLevel::LL_Debug);
			server_address = ip;
			port = _port;
			restart();
			all_ok = true;
		}
		else {
			_log("Can't resolve address for " + ip, LogLevel::LL_Error);
			GRNotifications::add_notification("Resolve Address Error", "Can't resolve address: " + ip, NotificationIcon::_Error);
		}
	}

	return all_ok;
}

void GRClient::set_input_buffer(int mb) {

	input_buffer_size_in_mb = mb;
	restart();
}

void GRClient::set_viewport_orientation_syncing(bool is_syncing) {
	_viewport_orientation_syncing = is_syncing;
	if (is_syncing) {
		if (input_collector && !input_collector->is_queued_for_deletion()) {
			_force_update_stream_viewport_signals();
		}
	}
}

bool GRClient::is_viewport_orientation_syncing() {
	return _viewport_orientation_syncing;
}

void GRClient::set_viewport_aspect_ratio_syncing(bool is_syncing) {
	_viewport_aspect_ratio_syncing = is_syncing;
	if (is_syncing) {
		call_deferred("_viewport_size_changed"); // force update screen aspect
	}
}

bool GRClient::is_viewport_aspect_ratio_syncing() {
	return _viewport_aspect_ratio_syncing;
}

void GRClient::set_server_settings_syncing(bool is_syncing) {
	_server_settings_syncing = is_syncing;
}

bool GRClient::is_server_settings_syncing() {
	return _server_settings_syncing;
}

void GRClient::set_password(String _pass) {
	password = _pass;
}

String GRClient::get_password() {
	return password;
}

void GRClient::set_device_id(String _id) {
	ERR_FAIL_COND(_id.empty());
	device_id = _id;
}

String GRClient::get_device_id() {
	return device_id;
}

bool GRClient::is_connected_to_host() {
	if (thread_connection.is_valid() && thread_connection->peer.is_valid()) {
		return thread_connection->peer->is_connected_to_host() && is_connection_working;
	}
	return false;
}

void GRClient::_force_update_stream_viewport_signals() {
	is_vertical = ScreenOrientation::NONE;
	if (!control_to_show_in || control_to_show_in->is_queued_for_deletion()) {
		return;
	}

	call_deferred("_viewport_size_changed"); // force update screen aspect ratio
}

void GRClient::_load_custom_input_scene(Ref<GRPacketCustomInputScene> _data) {
	_remove_custom_input_scene();

	if (_data->get_scene_path().empty() || _data->get_scene_data().size() == 0) {
		_log("Scene not specified or data is empty. Removing custom input scene", LogLevel::LL_Debug);
		return;
	}

	if (!control_to_show_in) {
		_log("Not specified control to show", LogLevel::LL_Error);
		return;
	}

	Error err = Error::OK;
#ifndef GDNATIVE_LIBRARY
	FileAccess* file = FileAccess::open(custom_input_scene_tmp_pck_file, FileAccess::ModeFlags::WRITE, &err);
#else
	File* file = File::_new();
	err = file->open(custom_input_scene_tmp_pck_file, File::ModeFlags::WRITE);
#endif
	if ((int)err) {
		_log("Can't open temp file to store custom input scene: " + custom_input_scene_tmp_pck_file + ", code: " + str((int)err), LogLevel::LL_Error);
	} else {

		PoolByteArray scene_data;
		if (_data->is_compressed()) {
			err = decompress_bytes(_data->get_scene_data(), _data->get_original_size(), scene_data, _data->get_compression_type());
		} else {
			scene_data = _data->get_scene_data();
		}

		if ((int)err) {
			_log("Can't decompress or set scene_data: Code: " + str((int)err), LogLevel::LL_Error);
		} else {

#ifndef GDNATIVE_LIBRARY
			auto r = scene_data.read();
			file->store_buffer(r.ptr(), scene_data.size());
#else
			file->store_buffer(scene_data);
#endif
			file->close();

#ifndef GDNATIVE_LIBRARY
			if (PackedData::get_singleton()->is_disabled()) {
				err = Error::FAILED;
			} else {
				err = PackedData::get_singleton()->add_pack(custom_input_scene_tmp_pck_file, true, 0);
			}
#else
			err = ProjectSettings::get_singleton()->load_resource_pack(custom_input_scene_tmp_pck_file, true, 0) ? Error::OK : Error::FAILED;
#endif

			if ((int)err) {
				_log("Can't load PCK file: " + custom_input_scene_tmp_pck_file, LogLevel::LL_Error);
			} else {

#ifndef GDNATIVE_LIBRARY
				Ref<PackedScene> pck = ResourceLoader::load(_data->get_scene_path(), "", false, &err);
#else
				Ref<PackedScene> pck = ResourceLoader::get_singleton()->load(_data->get_scene_path(), "", false);
				err = pck->can_instance() ? Error::OK : Error::FAILED;
#endif
				if ((int)err) {
					_log("Can't load scene file: " + _data->get_scene_path() + ", code: " + str((int)err), LogLevel::LL_Error);
				} else {

					custom_input_scene = pck->instance();
					if (!custom_input_scene) {
						_log("Can't instance scene from PCK file: " + custom_input_scene_tmp_pck_file + ", scene: " + _data->get_scene_path(), LogLevel::LL_Error);
					} else {

						control_to_show_in->add_child(custom_input_scene);
					}
				}
			}
		}
	}

#ifndef GDNATIVE_LIBRARY
	if (file) {
		memdelete(file);
	}
#else
	if (file) {
		file->free();
	}
#endif
}

void GRClient::_remove_custom_input_scene() {
	if (custom_input_scene && !custom_input_scene->is_queued_for_deletion()) {

		custom_input_scene->queue_del();
		custom_input_scene = nullptr;

		Error err = Error::OK;
#ifndef GDNATIVE_LIBRARY
		DirAccess *dir = DirAccess::open(custom_input_scene_tmp_pck_file.get_base_dir(), &err);
#else
		Directory* dir = Directory::_new();
		dir->open(custom_input_scene_tmp_pck_file.get_base_dir());
#endif
		if ((int)err) {
			_log("Can't open folder: " + custom_input_scene_tmp_pck_file.get_base_dir(), LogLevel::LL_Error);
		} else {
			if (dir && dir->file_exists(custom_input_scene_tmp_pck_file)) {
				err = dir->remove(custom_input_scene_tmp_pck_file);
				if ((int)err) {
					_log("Can't delete file: " + custom_input_scene_tmp_pck_file + ". Code: " + str((int)err), LogLevel::LL_Error);
				}
			}
		}

#ifndef GDNATIVE_LIBRARY
		if (dir) {
			memdelete(dir);
		}
#else
		dir->free();
#endif
	}
}

void GRClient::_viewport_size_changed() {
	if (!control_to_show_in || control_to_show_in->is_queued_for_deletion()) {
		return;
	}

	send_queue_mutex->lock();

	if (_viewport_orientation_syncing) {
		Vector2 size = control_to_show_in->get_size();
		ScreenOrientation tmp_vert = size.x < size.y ? ScreenOrientation::VERTICAL : ScreenOrientation::HORIZONTAL;
		if (tmp_vert != is_vertical) {
			is_vertical = tmp_vert;
			Ref<GRPacketClientStreamOrientation> packet = _find_queued_packet_by_type<Ref<GRPacketClientStreamOrientation> >();

			if (packet.is_null()) {
				packet.instance();
				send_packet(packet);
			}

			packet->set_vertical(is_vertical == ScreenOrientation::VERTICAL);
		}
	}

	if (_viewport_aspect_ratio_syncing) {
		Ref<GRPacketClientStreamAspect> packet = _find_queued_packet_by_type<Ref<GRPacketClientStreamAspect> >();

		if (packet.is_null()) {
			packet.instance();
			send_packet(packet);
		}

		Vector2 size = control_to_show_in->get_size();
		packet->set_aspect(size.x / size.y);
	}

	send_queue_mutex->unlock();
}

void GRClient::_update_texture_from_iamge(Ref<Image> img) {
	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		if (img.is_valid()) {
			Ref<ImageTexture> tex = tex_shows_stream->get_texture();
			if (tex.is_valid()) {
				tex->create_from_image(img);
			} else {
				tex.instance();
				tex->create_from_image(img);
				tex_shows_stream->set_texture(tex);
			}

			uint32_t new_flags = Texture::FLAG_MIPMAPS | (is_filtering_enabled ? Texture::FLAG_FILTER : 0);
			if (tex->get_flags() != new_flags) {
				tex->set_flags(new_flags);
			}
		} else {
			tex_shows_stream->set_texture(nullptr);
		}
	}
}

void GRClient::_update_stream_texture_state(StreamState is_has_signal) {
	if (is_deleting)
		return;

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		switch (is_has_signal) {
			case StreamState::STREAM_NO_SIGNAL: {
				tex_shows_stream->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

				if (custom_no_signal_texture.is_valid() || custom_no_signal_vertical_texture.is_valid()) {
					tex_shows_stream->set_texture(no_signal_is_vertical ?
														  (custom_no_signal_vertical_texture.is_valid() ? custom_no_signal_vertical_texture : custom_no_signal_texture) :
														  (custom_no_signal_texture.is_valid() ? custom_no_signal_texture : custom_no_signal_vertical_texture));
				}
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
				else {
					_update_texture_from_iamge(no_signal_is_vertical ? no_signal_vertical_image : no_signal_image);
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
				break;
			}
			case StreamState::STREAM_ACTIVE: {
				tex_shows_stream->set_stretch_mode(stretch_mode == StretchMode::STRETCH_KEEP_ASPECT ? TextureRect::STRETCH_KEEP_ASPECT_CENTERED : TextureRect::STRETCH_SCALE);
				tex_shows_stream->set_material(nullptr);
				break;
			}
			case StreamState::STREAM_NO_IMAGE:
				tex_shows_stream->set_stretch_mode(TextureRect::STRETCH_SCALE);
				tex_shows_stream->set_material(nullptr);
				tex_shows_stream->set_texture(nullptr);
				break;
			default:
				_log("Wrong stream state!", LogLevel::LL_Error);
				break;
		}

		if (signal_connection_state != is_has_signal) {
			call_deferred("emit_signal", "stream_state_changed", is_has_signal);
			signal_connection_state = is_has_signal;
		}
	}
}

void GRClient::_reset_counters() {
	GRDevice::_reset_counters();
	sync_time_client = 0;
	sync_time_server = 0;
}

void GRClient::set_server_setting(TypesOfServerSettings param, Variant value) {
	send_queue_mutex->lock();
	Ref<GRPacketServerSettings> packet = _find_queued_packet_by_type<Ref<GRPacketServerSettings> >();

	if (packet.is_null()) {
		packet.instance();
		send_packet(packet);
	}

	packet->add_setting(param, value);
	send_queue_mutex->unlock();
}

void GRClient::disable_overriding_server_settings() {
	set_server_setting(TypesOfServerSettings::USE_INTERNAL_SERVER_SETTINGS, true);
}

//////////////////////////////////////////////
////////////////// STATIC ////////////////////
//////////////////////////////////////////////

void GRClient::_thread_connection(void *p_userdata) {
	Ref<ConnectionThreadParams> con_thread = (ConnectionThreadParams *)p_userdata;
	GRClient *dev = con_thread->dev;
	Ref<StreamPeerTCP> con = con_thread->peer;

	OS *os = OS::get_singleton();
	Thread_set_name("GRemote_connection");
	GRDevice::AuthResult prev_auth_error = GRDevice::AuthResult::OK;

	const String con_error_title = "Connection Error";

	while (!con_thread->stop_thread) {
		if (os->get_ticks_usec() - dev->prev_valid_connection_time > 1000_ms) {
			dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
			dev->call_deferred("_remove_custom_input_scene");
		}

		if (con->get_status() == StreamPeerTCP::STATUS_CONNECTED || con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			con->disconnect_from_host();
		}

#ifndef GDNATIVE_LIBRARY
		IP_Address adr;
#else
		String adr;
#endif

		if (dev->con_type == CONNECTION_ADB) {
#ifndef GDNATIVE_LIBRARY
			adr = IP_Address("127.0.0.1");
#else
			adr = "127.0.0.1";
#endif
		}
		else {
			if (dev->server_address.is_valid_ip_address()) {
				adr = dev->server_address;
				if (adr.is_valid_ip()) {
				} else {
					_log("Address is invalid: " + dev->server_address, LogLevel::LL_Error);
					if (prev_auth_error != GRDevice::AuthResult::Error)
						GRNotifications::add_notification("Resolve Address Error", "Address is invalid: " + dev->server_address, NotificationIcon::_Error);
					prev_auth_error = GRDevice::AuthResult::Error;
				}
			} else {
				adr = IP::get_singleton()->resolve_hostname(adr);
				if (adr.is_valid_ip()) {
					_log("Resolved address for " + dev->server_address + "\n" + adr, LogLevel::LL_Debug);
				} else {
					_log("Can't resolve address for " + dev->server_address, LogLevel::LL_Error);
					if (prev_auth_error != GRDevice::AuthResult::Error)
						GRNotifications::add_notification("Resolve Address Error", "Can't resolve address: " + dev->server_address, NotificationIcon::_Error);
					prev_auth_error = GRDevice::AuthResult::Error;
				}
			}
		}

		String address = (String)adr + ":" + str(dev->port);
		Error err = con->connect_to_host(adr, dev->port);

		_log("Connecting to " + address, LogLevel::LL_Debug);
		if ((int)err) {
			switch (err) {
				case Error::FAILED:
					_log("Failed to open socket or can't connect to host", LogLevel::LL_Error);
					break;
				case Error::ERR_UNAVAILABLE:
					_log("Socket is unavailable", LogLevel::LL_Error);
					break;
				case Error::ERR_INVALID_PARAMETER:
					_log("Host address is invalid", LogLevel::LL_Error);
					break;
				case Error::ERR_ALREADY_EXISTS:
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
			_log("Connection timed out with " + address, LogLevel::LL_Debug);
			if (prev_auth_error != GRDevice::AuthResult::Timeout) {
				GRNotifications::add_notification(con_error_title, "Connection timed out: " + address, NotificationIcon::Warning);
				prev_auth_error = GRDevice::AuthResult::Timeout;
			}
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

				dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_IMAGE);

				con_thread->break_connection = false;
				con_thread->peer = con;
				con_thread->ppeer = ppeer;

				dev->is_connection_working = true;
				dev->call_deferred("emit_signal", "connection_state_changed", true);
				dev->call_deferred("_force_update_stream_viewport_signals"); // force update screen aspect ratio and orientation
				GRNotifications::add_notification("Connected", "Connected to " + address, NotificationIcon::Success);
				
				_connection_loop(con_thread);

				con_thread->peer.unref();
				con_thread->ppeer.unref();

				dev->is_connection_working = false;
				dev->call_deferred("emit_signal", "connection_state_changed", false);
				dev->call_deferred("emit_signal", "mouse_mode_changed", Input::MouseMode::MOUSE_MODE_VISIBLE);
				break;
			}
			case GRDevice::AuthResult::Error:
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Can't connect to " + address, NotificationIcon::_Error);
				long_wait = true;
				break;
			case GRDevice::AuthResult::Timeout:
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Timeout\n" + address, NotificationIcon::_Error);
				long_wait = true;
				break;
			case GRDevice::AuthResult::RefuseConnection:
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Connection refused\n" + address, NotificationIcon::_Error);
				long_wait = true;
				break;
			case GRDevice::AuthResult::VersionMismatch:
				GRNotifications::add_notification(con_error_title, "Version mismatch\n" + address, NotificationIcon::_Error);
				long_wait = true;
				break;
			case GRDevice::AuthResult::IncorrectPassword:
				GRNotifications::add_notification(con_error_title, "Incorrect password\n" + address, NotificationIcon::_Error);
				long_wait = true;
				break;
			case GRDevice::AuthResult::PasswordRequired:
				GRNotifications::add_notification(con_error_title, "Required password but it's not implemented.... " + address, NotificationIcon::_Error);
				break;
			default:
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Unknown error code: " + str((int)res) + "\n" + address, NotificationIcon::_Error);
				_log("Unknown error code: " + str((int)res) + ". Disconnecting. " + address);
				break;
		}

		prev_auth_error = res;

		if (con->is_connected_to_host()) {
			con->disconnect_from_host();
		}

		if (long_wait) {
			os->delay_usec(888_ms);
		}
	}

	dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
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
	String address = CON_ADDRESS(connection);

	dev->_reset_counters();

	std::vector<Ref<GRPacketImageData> > stream_queue;

	uint64_t time64 = os->get_ticks_usec();
	uint64_t prev_cycle_time = 0;
	uint64_t prev_send_input_time = time64;
	uint64_t prev_ping_sending_time = time64;
	uint64_t next_image_required_frametime = time64;
	uint64_t prev_display_image_time = time64 - 16_ms;

	bool ping_sended = false;

	TimeCountInit();
	while (!con_thread->break_connection && connection->is_connected_to_host()) {
		dev->connection_mutex->lock();
		if (con_thread->break_connection || !connection->is_connected_to_host())
			break;
		TimeCount("Cycle start");
		uint64_t cycle_start_time = os->get_ticks_usec();

		if (!_is_processing_img) {
			if (_img_thread) {
				t_wait_to_finish(_img_thread);
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

			Ref<GRPacketInputData> pack = dev->input_collector->get_collected_input_data();

			if (pack.is_valid()) {
				err = ppeer->put_var(pack->get_data());
				if ((int)err) {
					_log("Put input data failed with code: " + str((int)err), LogLevel::LL_Error);
					goto end_send;
				}
			} else {
				_log("Can't get input data from input collector", LogLevel::LL_Error);
			}
			TimeCount("Input send");
		}

		// PING
		time64 = os->get_ticks_usec();
		if ((time64 - prev_ping_sending_time) > 100_ms && !ping_sended) {
			nothing_happens = false;
			ping_sended = true;

			Ref<GRPacketPing> pack(memnew(GRPacketPing));
			err = ppeer->put_var(pack->get_data());
			prev_ping_sending_time = time64;

			if ((int)err) {
				_log("Send ping failed with code: " + str((int)err), LogLevel::LL_Error);
				goto end_send;
			}
			TimeCount("Ping send");
		}

		// SEND QUEUE
		start_while_time = os->get_ticks_usec();
		while (!dev->send_queue.empty() && (os->get_ticks_usec() - start_while_time) <= send_data_time_us / 2) {
			dev->send_queue_mutex->lock();

#ifndef GDNATIVE_LIBRARY
			Ref<GRPacket> packet = dev->send_queue.front()->get();
#else
			Ref<GRPacket> packet = dev->send_queue.front();
#endif

				if (packet.is_valid()) {

#ifndef GDNATIVE_LIBRARY
				dev->send_queue.pop_front();
#else
				dev->send_queue.erase(dev->send_queue.begin());
#endif

				err = ppeer->put_var(packet->get_data());

				if ((int)err) {
					_log("Put data from queue failed with code: " + str((int)err), LogLevel::LL_Error);
					dev->send_queue_mutex->unlock();
					goto end_send;
				}
			}
			dev->send_queue_mutex->unlock();
		}
		TimeCount("Send queue");
	end_send:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after sending!", LogLevel::LL_Error);
			GRNotifications::add_notification("Error", "Lost connection after sending data!", NotificationIcon::_Error);
			continue;
		}

		///////////////////////////////////////////////////////////////////
		// RECEIVING

		// Send to processing one of buffered images
		time64 = os->get_ticks_usec();
		if (!_is_processing_img && !stream_queue.empty() && time64 >= next_image_required_frametime) {
			nothing_happens = false;

#ifndef GDNATIVE_LIBRARY
			Ref<GRPacketImageData> pack = stream_queue.front()->get();
			stream_queue.pop_front();
#else
			Ref<GRPacketImageData> pack = stream_queue.front();
			stream_queue.erase(stream_queue.begin());
#endif

			if (pack.is_null()) {
				_log("Queued image data is null", LogLevel::LL_Error);
				goto end_img_process;
			}

			uint64_t frametime = pack->get_frametime() > 1000_ms ? 1000_ms : pack->get_frametime();
			next_image_required_frametime = time64 + frametime - prev_cycle_time;

			dev->_update_avg_fps(time64 - prev_display_image_time);
			prev_display_image_time = time64;

			if (_img_thread) {
				t_wait_to_finish(_img_thread);
				memdelete(_img_thread);
				_img_thread = nullptr;
			}

			ImgProcessingStorage *ips = new ImgProcessingStorage(dev);
			if (pack->get_is_empty()) {
				dev->_update_avg_fps(0);
				dev->call_deferred("_update_texture_from_iamge", Ref<Image>());
				dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_IMAGE);
			} else {
				ips->tex_data = pack->get_image_data();
				ips->compression_type = (ImageCompressionType)pack->get_compression_type();
				ips->size = pack->get_size();
				ips->format = pack->get_format();
				ips->_is_processing_img = &_is_processing_img;
				_img_thread = Thread_create(GRClient, _thread_image_decoder, ips, dev);
			}

			TimeCount("Get image from queue");
		}
	end_img_process:

		// check if image displayed less then few seconds ago. if not then remove texture
		const double image_loss_time = 1.5;
		if (os->get_ticks_usec() > prev_display_image_time + uint64_t(1000_ms * image_loss_time)) {
			if (dev->signal_connection_state != StreamState::STREAM_NO_IMAGE) {
				dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_IMAGE);
				dev->_reset_counters();
			}
		}

		if (stream_queue.size() > 10) {
			stream_queue.clear();
		}

		// Get some packets
		start_while_time = os->get_ticks_usec();
		while (ppeer->get_available_packet_count() > 0 && (os->get_ticks_usec() - start_while_time) <= send_data_time_us / 2) {
			nothing_happens = false;

			Variant buf;
			ppeer->get_var(buf);

			if ((int)err)
				goto end_recv;

			Ref<GRPacket> pack = GRPacket::create(buf);
			if (pack.is_null()) {
				_log("GRPacket is null", LogLevel::LL_Error);
				continue;
			}

			PacketType type = pack->get_type();

			switch (type) {
				case PacketType::SyncTime: {
					Ref<GRPacketSyncTime> data = pack;
					if (data.is_null()) {
						_log("GRPacketSyncTime is null", LogLevel::LL_Error);
						continue;
					}

					dev->sync_time_client = os->get_ticks_usec();
					dev->sync_time_server = data->get_time();

					break;
				}
				case PacketType::ImageData: {
					Ref<GRPacketImageData> data = pack;
					if (data.is_null()) {
						_log("GRPacketImageData is null", LogLevel::LL_Error);
						continue;
					}

					stream_queue.push_back(data);
					break;
				}
				case PacketType::ServerSettings: {
					if (!dev->_server_settings_syncing) {
						continue;
					}

					Ref<GRPacketServerSettings> data = pack;
					if (data.is_null()) {
						_log("GRPacketServerSettings is null", LogLevel::LL_Error);
						continue;
					}

					dev->call_deferred("emit_signal", "server_settings_received", data->get_settings());
					break;
				}
				case PacketType::MouseModeSync: {
					Ref<GRPacketMouseModeSync> data = pack;
					if (data.is_null()) {
						_log("GRPacketMouseModeSync is null", LogLevel::LL_Error);
						continue;
					}

					dev->call_deferred("emit_signal", "mouse_mode_changed", data->get_mouse_mode());
					break;
				}
				case PacketType::CustomInputScene: {
					Ref<GRPacketCustomInputScene> data = pack;
					if (data.is_null()) {
						_log("GRPacketCustomInputScene is null", LogLevel::LL_Error);
						continue;
					}

					dev->call_deferred("_load_custom_input_scene", data);
					break;
				}
				case PacketType::Ping: {
					Ref<GRPacketPong> pack(memnew(GRPacketPong));
					err = ppeer->put_var(pack->get_data());
					if ((int)err) {
						_log("Send pong failed with code: " + str((int)err));
						goto end_send;
					}
					break;
				}
				case PacketType::Pong: {
					dev->_update_avg_ping(os->get_ticks_usec() - prev_ping_sending_time);
					ping_sended = false;
					break;
				}
				default:
					_log("Not supported packet type! " + str((int)type), LogLevel::LL_Warning);
					break;
			}
		}
		TimeCount("End receiving");
	end_recv:
		dev->connection_mutex->unlock();

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after receiving!", LogLevel::LL_Error);
			GRNotifications::add_notification("Error", "Lost connection after receiving data!", NotificationIcon::_Error);
			continue;
		}

		if (nothing_happens)
			os->delay_usec(1_ms);

		prev_cycle_time = os->get_ticks_usec() - cycle_start_time;
	}

	dev->send_queue_mutex->lock();
	dev->send_queue.clear();
	dev->send_queue_mutex->unlock();

	dev->connection_mutex->unlock();

	if (connection->is_connected_to_host()) {
		_log("Lost connection to " + address, LogLevel::LL_Error);
		GRNotifications::add_notification("Disconnected", "Closing connection to " + address, NotificationIcon::Fail);
	} else {
		_log("Closing connection to " + address, LogLevel::LL_Error);
		GRNotifications::add_notification("Disconnected", "Lost connection to " + address, NotificationIcon::Fail);
	}

	if (_img_thread) {
		t_wait_to_finish(_img_thread);
		memdelete(_img_thread);
		_img_thread = nullptr;
	}

	_log("Closing connection");
	con_thread->break_connection = true;
}

void GRClient::_thread_image_decoder(void *p_userdata) {
	ImgProcessingStorage *ips = (ImgProcessingStorage *)p_userdata;
	*ips->_is_processing_img = true;

	Error err = Error::OK;
	GRClient *dev = ips->dev;
	Ref<Image> img(memnew(Image));
	ImageCompressionType type = ips->compression_type;

	TimeCountInit();
	switch (type) {
		case ImageCompressionType::Uncompressed: {
#ifndef GDNATIVE_LIBRARY
			img->create(ips->size.x, ips->size.y, false, (Image::Format)ips->format, ips->tex_data);
#else
			img->create_from_data(ips->size.x, ips->size.y, false, (Image::Format)ips->format, ips->tex_data);
#endif
			if (img_is_empty(img)) { // is NOT OK
				err = Error::FAILED;
				_log("Incorrect uncompressed image data.", LogLevel::LL_Error);
				GRNotifications::add_notification("Stream Error", "Incorrect uncompressed image data.", NotificationIcon::_Error);
			}
			break;
		}
		case ImageCompressionType::JPG: {
			err = img->load_jpg_from_buffer(ips->tex_data);
			if ((int)err || img_is_empty(img)) { // is NOT OK
				_log("Can't decode JPG image.", LogLevel::LL_Error);
				GRNotifications::add_notification("Stream Error", "Can't decode JPG image. Code: " + str((int)err), NotificationIcon::_Error);
			}
			break;
		}
		case ImageCompressionType::PNG: {
			err = img->load_png_from_buffer(ips->tex_data);
			if ((int)err || img_is_empty(img)) { // is NOT OK
				_log("Can't decode PNG image.", LogLevel::LL_Error);
				GRNotifications::add_notification("Stream Error", "Can't decode PNG image. Code: " + str((int)err), NotificationIcon::_Error);
			}
			break;
		}
		default:
			_log("Not implemented image decoder type: " + str((int)type), LogLevel::LL_Error);
			break;
	}

	if (!(int)err) { // is OK
		TimeCount("Create Image Time");
		dev->call_deferred("_update_texture_from_iamge", img);

		if (dev->signal_connection_state != StreamState::STREAM_ACTIVE) {
			dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_ACTIVE);
		}
	}

	*ips->_is_processing_img = false;
	delete ips;
}

GRDevice::AuthResult GRClient::_auth_on_server(GRClient *dev, Ref<PacketPeerStream> &ppeer) {
#define wait_packet(_n)                                                                        \
	time = OS::get_singleton()->get_ticks_msec();                                              \
	while (ppeer->get_available_packet_count() == 0) {                                         \
		if (OS::get_singleton()->get_ticks_msec() - time > 150) {                              \
			_log("Connection timeout. Disconnecting. Waited: " + str(_n), LogLevel::LL_Debug); \
			goto timeout;                                                                      \
		}                                                                                      \
		if (!con->is_connected_to_host()) {                                                    \
			return GRDevice::AuthResult::Error;                                                \
		}                                                                                      \
		OS::get_singleton()->delay_usec(1_ms);                                                 \
	}
#define packet_error_check(_t)              \
	if ((int)err) {                              \
		_log(_t, LogLevel::LL_Debug);       \
		return GRDevice::AuthResult::Error; \
	}

	Ref<StreamPeerTCP> con = ppeer->get_stream_peer();
	String address = CON_ADDRESS(con);
	uint32_t time = 0;

	Error err = Error::OK;
	Variant ret;
	// GET first packet
	wait_packet("first_packet");
#ifndef GDNATIVE_LIBRARY
	err = ppeer->get_var(ret);
	packet_error_check("Can't get first authorization packet from server. Code: " + str((int)err));
#else
	ret = ppeer->get_var();
#endif

	if ((int)ret == (int)GRDevice::AuthResult::RefuseConnection) {
		_log("Connection refused", LogLevel::LL_Error);
		return GRDevice::AuthResult::RefuseConnection;
	}
	if ((int)ret == (int)GRDevice::AuthResult::TryToConnect) {
		Dictionary data;
		data["id"] = dev->device_id;
		data["version"] = get_gr_version();
		data["password"] = dev->password;

		// PUT auth data
		err = ppeer->put_var(data);
		packet_error_check("Can't put authorization data to server. Code: " + str((int)err));

		// GET result
		wait_packet("result");
#ifndef GDNATIVE_LIBRARY
		err = ppeer->get_var(ret);
		packet_error_check("Can't get final authorization packet from server. Code: " + str((int)err));
#else
		ret = ppeer->get_var();
#endif

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
	if (!dev || dev->get_status() != WorkingStatus::Working)
		return;

	if (texture_rect && !texture_rect->is_queued_for_deletion()) {
		switch (dev->get_stretch_mode()) {
			case StretchMode::STRETCH_KEEP_ASPECT: {
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
			case StretchMode::STRETCH_FILL:
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

void GRInputCollector::_collect_input(Ref<InputEvent> ie) {
	Ref<GRInputDataEvent> data = GRInputDataEvent::parse_event(ie, stream_rect);
	if (data.is_valid()) {
		_TS_LOCK_;
		collected_input_data.push_back(data);
		_TS_UNLOCK_;
	}
}

void GRInputCollector::_release_pointers() {
	{
		auto buttons = mouse_buttons.keys();
		for (int i = 0; i < buttons.size(); i++) {
			if (mouse_buttons[buttons[i]]) {
				Ref<InputEventMouseButton> iemb(memnew(InputEventMouseButton));
				iemb->set_button_index(buttons[i]);
				iemb->set_pressed(false);
				buttons[i] = false;
				_collect_input(iemb);
			}
		}
	}

	{
		auto touches = screen_touches.keys();
		for (int i = 0; i < touches.size(); i++) {
			if (screen_touches[touches[i]]) {
				Ref<InputEventScreenTouch> iest(memnew(InputEventScreenTouch));
				iest->set_index(touches[i]);
				iest->set_pressed(false);
				touches[i] = false;
				_collect_input(iest);
			}
		}
	}
}

#ifndef GDNATIVE_LIBRARY

void GRInputCollector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_input", "input_event"), &GRInputCollector::_input);

	ClassDB::bind_method(D_METHOD("is_capture_on_focus"), &GRInputCollector::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD("set_capture_on_focus", "value"), &GRInputCollector::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD("is_capture_when_hover"), &GRInputCollector::is_capture_when_hover);
	ClassDB::bind_method(D_METHOD("set_capture_when_hover", "value"), &GRInputCollector::set_capture_when_hover);
	ClassDB::bind_method(D_METHOD("is_capture_pointer"), &GRInputCollector::is_capture_pointer);
	ClassDB::bind_method(D_METHOD("set_capture_pointer", "value"), &GRInputCollector::set_capture_pointer);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), "set_capture_on_focus", "is_capture_on_focus");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), "set_capture_when_hover", "is_capture_when_hover");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_pointer"), "set_capture_pointer", "is_capture_pointer");
}

#else


void GRInputCollector::_register_methods() {
}

#endif

void GRInputCollector::_input(Ref<InputEvent> ie) {
	if (!parent || (capture_only_when_control_in_focus && !parent->has_focus()) ||
			(dev && dev->get_status() != WorkingStatus::Working) ||
			!dev->is_stream_active() || !is_inside_tree()) {
		return;
	}

	_TS_LOCK_;
	if (collected_input_data.size() >= 256) {
		collected_input_data.resize(0);
	}
	_TS_UNLOCK_;

	// TODO maybe later i change it to update less frequent
	_update_stream_rect();

	if (ie.is_null()) {
		_log("InputEvent is null", LogLevel::LL_Error);
		return;
	}

	{
		Ref<InputEventMouseButton> iemb = ie;
		if (iemb.is_valid()) {
			int idx = iemb->get_button_index();

			if ((!stream_rect.has_point(iemb->get_position()) && capture_pointer_only_when_hover_control) || dont_capture_pointer) {
				if (idx == BUTTON_WHEEL_UP || idx == BUTTON_WHEEL_DOWN ||
						idx == BUTTON_WHEEL_LEFT || idx == BUTTON_WHEEL_RIGHT) {
					return;
				} else {
					if (iemb->is_pressed() || !((bool)mouse_buttons[idx]))
						return;
				}
			}

			mouse_buttons[idx] = iemb->is_pressed();
			goto end;
		}
	}

	{
		Ref<InputEventMouseMotion> iemm = ie;
		if (iemm.is_valid()) {
			if ((!stream_rect.has_point(iemm->get_position()) && capture_pointer_only_when_hover_control) || dont_capture_pointer)
				return;
			goto end;
		}
	}

	{
		Ref<InputEventScreenTouch> iest = ie;
		if (iest.is_valid()) {
			int idx = iest->get_index();
			if ((!stream_rect.has_point(iest->get_position()) && capture_pointer_only_when_hover_control) || dont_capture_pointer) {
				if (iest->is_pressed() || !((bool)screen_touches[idx]))
					return;
			}

			screen_touches[idx] = iest->is_pressed();
			goto end;
		}
	}

	{
		Ref<InputEventScreenDrag> iesd = ie;
		if (iesd.is_valid()) {
			if ((!stream_rect.has_point(iesd->get_position()) && capture_pointer_only_when_hover_control) || dont_capture_pointer)
				return;
			goto end;
		}
	}

	{
		Ref<InputEventMagnifyGesture> iemg = ie;
		if (iemg.is_valid()) {
			if ((!stream_rect.has_point(iemg->get_position()) && capture_pointer_only_when_hover_control) || dont_capture_pointer)
				return;
			goto end;
		}
	}

	{
		Ref<InputEventPanGesture> iepg = ie;
		if (iepg.is_valid()) {
			if ((!stream_rect.has_point(iepg->get_position()) && capture_pointer_only_when_hover_control) || dont_capture_pointer)
				return;
			goto end;
		}
	}

end:

	_collect_input(ie);
}

void GRInputCollector::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE: {
			parent = cast_to<Control>(get_parent());
			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			parent = nullptr;
			break;
		}
		case NOTIFICATION_PROCESS: {
			_TS_LOCK_;
			auto w = sensors.write();
			w[0] = Input::get_singleton()->get_accelerometer();
			w[1] = Input::get_singleton()->get_gravity();
			w[2] = Input::get_singleton()->get_gyroscope();
			w[3] = Input::get_singleton()->get_magnetometer();
			_TS_UNLOCK_;
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

bool GRInputCollector::is_capture_pointer() {
	return !dont_capture_pointer;
}

void GRInputCollector::set_capture_pointer(bool value) {
	if (!value) {
		_release_pointers();
	}
	dont_capture_pointer = !value;
}

bool GRInputCollector::is_capture_input() {
	return is_processing_input();
}

void GRInputCollector::set_capture_input(bool value) {
	set_process_input(value);
}

void GRInputCollector::set_tex_rect(TextureRect *tr) {
	texture_rect = tr;
}

Ref<GRPacketInputData> GRInputCollector::get_collected_input_data() {
	Ref<GRPacketInputData> res(memnew(GRPacketInputData));
	Ref<GRInputDeviceSensorsData> s(memnew(GRInputDeviceSensorsData));
	s->set_sensors(sensors);

	_TS_LOCK_;

	collected_input_data.push_back(s);
	res->set_input_data(collected_input_data);
	collected_input_data.resize(0);

	_TS_UNLOCK_;
	return res;
}

GRInputCollector::GRInputCollector() {
	_TS_LOCK_;
	parent = nullptr;
	set_process(true);
	set_process_input(true);
	sensors.resize(4);
	_TS_UNLOCK_;
}

GRInputCollector::~GRInputCollector() {
	_TS_LOCK_;
	sensors.resize(0);
	collected_input_data.resize(0);
	if (this_in_client)
		*this_in_client = nullptr;
	_TS_UNLOCK_;
}

//////////////////////////////////////////////
/////////////// TEXTURE RECT /////////////////
//////////////////////////////////////////////

void GRTextureRect::_tex_size_changed() {
	if (dev) {
		Vector2 v = get_size();
		bool is_vertical = v.x < v.y;
		if (is_vertical != dev->no_signal_is_vertical) {
			dev->no_signal_is_vertical = is_vertical;
			dev->_update_stream_texture_state(dev->signal_connection_state); // update texture
		}
	}
}

#ifndef GDNATIVE_LIBRARY

void GRTextureRect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_tex_size_changed"), &GRTextureRect::_tex_size_changed);
}

#else


void GRTextureRect::_register_methods() {
}

#endif

GRTextureRect::GRTextureRect() {
	connect("resized", this, "_tex_size_changed");
}

GRTextureRect::~GRTextureRect() {
	if (this_in_client)
		*this_in_client = nullptr;
	disconnect("resized", this, "_tex_size_changed");
}

#endif // !NO_GODOTREMOTE_CLIENT

/* GRClient.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRClient.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRResources.h"
#include "GodotRemote.h"

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

#include <ClassDB.hpp>
#include <Control.hpp>
#include <Directory.hpp>
#include <File.hpp>
#include <GlobalConstants.hpp>
#include <IP.hpp>
#include <Input.hpp>
#include <InputMap.hpp>
#include <Material.hpp>
#include <Node.hpp>
#include <PackedScene.hpp>
#include <ProjectSettings.hpp>
#include <RandomNumberGenerator.hpp>
#include <ResourceLoader.hpp>
#include <SceneTree.hpp>
#include <TCP_Server.hpp>
#include <Texture.hpp>
#include <Thread.hpp>
#include <Viewport.hpp>
using namespace godot;

#define BUTTON_WHEEL_UP GlobalConstants::BUTTON_WHEEL_UP
#define BUTTON_WHEEL_DOWN GlobalConstants::BUTTON_WHEEL_DOWN
#define BUTTON_WHEEL_LEFT GlobalConstants::BUTTON_WHEEL_LEFT
#define BUTTON_WHEEL_RIGHT GlobalConstants::BUTTON_WHEEL_RIGHT
#endif

enum class DeletingVarName {
	CONTROL_TO_SHOW_STREAM,
	TEXTURE_TO_SHOW_STREAM,
	INPUT_COLLECTOR,
	CUSTOM_INPUT_SCENE,
};

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY
void GRClient::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_update_texture_from_image", "image"), &GRClient::_update_texture_from_image);
	ClassDB::bind_method(D_METHOD("_update_stream_texture_state", "state"), &GRClient::_update_stream_texture_state);
	ClassDB::bind_method(D_METHOD("_force_update_stream_viewport_signals"), &GRClient::_force_update_stream_viewport_signals);
	ClassDB::bind_method(D_METHOD("_viewport_size_changed"), &GRClient::_viewport_size_changed);
	ClassDB::bind_method(D_METHOD("_load_custom_input_scene", "_data"), &GRClient::_load_custom_input_scene);
	ClassDB::bind_method(D_METHOD("_remove_custom_input_scene"), &GRClient::_remove_custom_input_scene);
	ClassDB::bind_method(D_METHOD("_on_node_deleting", "var_name"), &GRClient::_on_node_deleting);

	ClassDB::bind_method(D_METHOD("set_control_to_show_in", "control_node", "position_in_node"), &GRClient::set_control_to_show_in, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_texture", "texture"), &GRClient::set_custom_no_signal_texture);
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_vertical_texture", "texture"), &GRClient::set_custom_no_signal_vertical_texture);
	ClassDB::bind_method(D_METHOD("set_custom_no_signal_material", "material"), &GRClient::set_custom_no_signal_material);
	ClassDB::bind_method(D_METHOD("set_address_port", "ip", "port"), &GRClient::set_address_port);
	ClassDB::bind_method(D_METHOD("set_address", "ip"), &GRClient::set_address);
	ClassDB::bind_method(D_METHOD("set_server_setting", "setting", "value"), &GRClient::set_server_setting);
	ClassDB::bind_method(D_METHOD("disable_overriding_server_settings"), &GRClient::disable_overriding_server_settings);

	ClassDB::bind_method(D_METHOD("get_custom_input_scene"), &GRClient::get_custom_input_scene);
	ClassDB::bind_method(D_METHOD("get_address"), &GRClient::get_address);
	ClassDB::bind_method(D_METHOD("is_stream_active"), &GRClient::is_stream_active);
	ClassDB::bind_method(D_METHOD("is_connected_to_host"), &GRClient::is_connected_to_host);

	ADD_SIGNAL(MethodInfo("custom_input_scene_added"));
	ADD_SIGNAL(MethodInfo("custom_input_scene_removed"));

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
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "password"), "set_password", "get_password");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "device_id"), "set_device_id", "get_device_id");
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
	///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////
	/*
	METHOD_REG(GRClient::_internal_call_only_deffered_start);
	METHOD_REG(GRClient::_internal_call_only_deffered_stop);

	METHOD_REG(GRClient::_internal_call_only_deffered_restart);

	METHOD_REG(GRClient::get_avg_ping);
	METHOD_REG(GRClient::get_avg_fps);

	METHOD_REG(GRClient::get_port);
	METHOD_REG(GRClient::set_port);

	METHOD_REG(GRClient::start);
	METHOD_REG(GRClient::stop);
	METHOD_REG(GRClient::get_status);

	register_signal<GRClient>("status_changed", "status", GODOT_VARIANT_TYPE_INT);
	register_property<GRClient, uint16_t>("port", &GRClient::set_port, &GRClient::get_port, 52341);
	*/
	///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////

	METHOD_REG(GRClient, _notification);
	METHOD_REG(GRClient, _thread_connection);
	METHOD_REG(GRClient, _thread_image_decoder);

	METHOD_REG(GRClient, _update_texture_from_image);
	METHOD_REG(GRClient, _update_stream_texture_state);
	METHOD_REG(GRClient, _force_update_stream_viewport_signals);
	METHOD_REG(GRClient, _viewport_size_changed);
	METHOD_REG(GRClient, _load_custom_input_scene);
	METHOD_REG(GRClient, _remove_custom_input_scene);
	METHOD_REG(GRClient, _on_node_deleting);

	METHOD_REG(GRClient, set_control_to_show_in);
	METHOD_REG(GRClient, set_custom_no_signal_texture);
	METHOD_REG(GRClient, set_custom_no_signal_vertical_texture);
	METHOD_REG(GRClient, set_custom_no_signal_material);
	METHOD_REG(GRClient, set_address_port);
	METHOD_REG(GRClient, set_address);
	METHOD_REG(GRClient, set_server_setting);
	METHOD_REG(GRClient, disable_overriding_server_settings);

	METHOD_REG(GRClient, get_custom_input_scene);
	METHOD_REG(GRClient, get_address);
	METHOD_REG(GRClient, is_stream_active);
	METHOD_REG(GRClient, is_connected_to_host);

	register_signal<GRClient>("custom_input_scene_added", Dictionary::make());
	register_signal<GRClient>("custom_input_scene_removed", Dictionary::make());

	register_signal<GRClient>("stream_state_changed", "state", GODOT_VARIANT_TYPE_INT);
	register_signal<GRClient>("connection_state_changed", "is_connected", GODOT_VARIANT_TYPE_BOOL);
	register_signal<GRClient>("mouse_mode_changed", "mouse_mode", GODOT_VARIANT_TYPE_INT);
	register_signal<GRClient>("server_settings_received", "settings", GODOT_VARIANT_TYPE_DICTIONARY);

	// SETGET
	METHOD_REG(GRClient, set_capture_on_focus);
	METHOD_REG(GRClient, set_capture_when_hover);
	METHOD_REG(GRClient, set_capture_pointer);
	METHOD_REG(GRClient, set_capture_input);
	METHOD_REG(GRClient, set_connection_type);
	METHOD_REG(GRClient, set_target_send_fps);
	METHOD_REG(GRClient, set_stretch_mode);
	METHOD_REG(GRClient, set_texture_filtering);
	METHOD_REG(GRClient, set_password);
	METHOD_REG(GRClient, set_device_id);
	METHOD_REG(GRClient, set_viewport_orientation_syncing);
	METHOD_REG(GRClient, set_viewport_aspect_ratio_syncing);
	METHOD_REG(GRClient, set_server_settings_syncing);

	METHOD_REG(GRClient, is_capture_on_focus);
	METHOD_REG(GRClient, is_capture_when_hover);
	METHOD_REG(GRClient, is_capture_pointer);
	METHOD_REG(GRClient, is_capture_input);
	METHOD_REG(GRClient, get_connection_type);
	METHOD_REG(GRClient, get_target_send_fps);
	METHOD_REG(GRClient, get_stretch_mode);
	METHOD_REG(GRClient, get_texture_filtering);
	METHOD_REG(GRClient, get_password);
	METHOD_REG(GRClient, get_device_id);
	METHOD_REG(GRClient, is_viewport_orientation_syncing);
	METHOD_REG(GRClient, is_viewport_aspect_ratio_syncing);
	METHOD_REG(GRClient, is_server_settings_syncing);

	register_property<GRClient, bool>("capture_on_focus", &GRClient::set_capture_on_focus, &GRClient::is_capture_on_focus, false);
	register_property<GRClient, bool>("capture_when_hover", &GRClient::set_capture_when_hover, &GRClient::is_capture_when_hover, false);
	register_property<GRClient, bool>("capture_pointer", &GRClient::set_capture_pointer, &GRClient::is_capture_pointer, true);
	register_property<GRClient, bool>("capture_input", &GRClient::set_capture_input, &GRClient::is_capture_input, true);
	register_property<GRClient, int>("connection_type", &GRClient::set_connection_type, &GRClient::get_connection_type, CONNECTION_WiFi, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "WiFi,ADB");
	register_property<GRClient, int>("target_send_fps", &GRClient::set_target_send_fps, &GRClient::get_target_send_fps, 60, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "1,1000");
	register_property<GRClient, int>("stretch_mode", &GRClient::set_stretch_mode, &GRClient::get_stretch_mode, STRETCH_KEEP_ASPECT, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Fill,Keep Aspect");
	register_property<GRClient, bool>("texture_filtering", &GRClient::set_texture_filtering, &GRClient::get_texture_filtering, true);
	register_property<GRClient, String>("password", &GRClient::set_password, &GRClient::get_password, "");
	register_property<GRClient, String>("device_id", &GRClient::set_device_id, &GRClient::get_device_id, "");
	register_property<GRClient, bool>("viewport_orientation_syncing", &GRClient::set_viewport_orientation_syncing, &GRClient::is_viewport_orientation_syncing, true);
	register_property<GRClient, bool>("viewport_aspect_ratio_syncing", &GRClient::set_viewport_aspect_ratio_syncing, &GRClient::is_viewport_aspect_ratio_syncing, true);
	register_property<GRClient, bool>("server_settings_syncing", &GRClient::set_server_settings_syncing, &GRClient::is_server_settings_syncing, true);
}

#endif

void GRClient::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE: {
			_deinit();
			GRDevice::_deinit();
			break;
			case NOTIFICATION_EXIT_TREE:
				is_deleting = true;
				if (get_status() == (int)WorkingStatus::STATUS_WORKING) {
					_internal_call_only_deffered_stop();
				}
				break;
		}
	}
}

void GRClient::_init() {
	set_name("GodotRemoteClient");
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRDevice::_init();
#endif

#ifndef GDNATIVE_LIBRARY
	Math::randomize();
	device_id = str(Math::randd() * Math::rand()).md5_text().substr(0, 6);
#else
	RandomNumberGenerator *rng = memnew(RandomNumberGenerator);
	rng->randomize();
	device_id = str(rng->randf() * rng->randf()).md5_text().substr(0, 6);
	memdelete(rng);
#endif

	Mutex_create(connection_mutex);

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_image.instance();
	GetPoolVectorFromBin(tmp_no_signal, GRResources::Bin_NoSignalPNG);
	no_signal_image->load_png_from_buffer(tmp_no_signal);

	no_signal_vertical_image.instance();
	GetPoolVectorFromBin(tmp_no_signal_vert, GRResources::Bin_NoSignalVerticalPNG);
	no_signal_vertical_image->load_png_from_buffer(tmp_no_signal_vert);

	Ref<Shader> shader = newref(Shader);
	shader->set_code(GRResources::Txt_CRT_Shader);
	no_signal_mat.instance();
	no_signal_mat->set_shader(shader);
#endif
}

void GRClient::_deinit() {
	LEAVE_IF_EDITOR();

	is_deleting = true;
	if (get_status() == (int)WorkingStatus::STATUS_WORKING) {
		_internal_call_only_deffered_stop();
	}
	set_control_to_show_in(nullptr, 0);
	Mutex_delete(connection_mutex);

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_mat.unref();
	no_signal_image.unref();
	no_signal_vertical_image.unref();
#endif
}

void GRClient::_internal_call_only_deffered_start() {
	switch ((WorkingStatus)get_status()) {
		case WorkingStatus::STATUS_WORKING:
			ERR_FAIL_MSG("Can't start already working GodotRemote Client");
		case WorkingStatus::STATUS_STARTING:
			ERR_FAIL_MSG("Can't start already starting GodotRemote Client");
		case WorkingStatus::STATUS_STOPPING:
			ERR_FAIL_MSG("Can't start stopping GodotRemote Client");
	}

	_log("Starting GodotRemote client. Version: " + str(GodotRemote::get_singleton()->get_version()), LogLevel::LL_NORMAL);
	set_status(WorkingStatus::STATUS_STARTING);

	if (thread_connection) {
		Mutex_lock(connection_mutex);
		thread_connection->break_connection = true;
		thread_connection->stop_thread = true;
		Mutex_unlock(connection_mutex);
		thread_connection->close_thread();
		memdelete(thread_connection);
		thread_connection = nullptr;
	}
	thread_connection = memnew(ConnectionThreadParamsClient);
	thread_connection->dev = this;
	thread_connection->peer.instance();
	Thread_start(thread_connection->thread_ref, GRClient, _thread_connection, thread_connection, this);

	call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
	set_status(WorkingStatus::STATUS_WORKING);
}

void GRClient::_internal_call_only_deffered_stop() {
	switch ((WorkingStatus)get_status()) {
		case WorkingStatus::STATUS_STOPPED:
			ERR_FAIL_MSG("Can't stop already stopped GodotRemote Client");
		case WorkingStatus::STATUS_STOPPING:
			ERR_FAIL_MSG("Can't stop already stopping GodotRemote Client");
		case WorkingStatus::STATUS_STARTING:
			ERR_FAIL_MSG("Can't stop starting GodotRemote Client");
	}

	_log("Stopping GodotRemote client", LogLevel::LL_DEBUG);
	set_status(WorkingStatus::STATUS_STOPPING);
	_remove_custom_input_scene();

	if (thread_connection) {
		Mutex_lock(connection_mutex);
		thread_connection->break_connection = true;
		thread_connection->stop_thread = true;
		Mutex_unlock(connection_mutex);
		thread_connection->close_thread();
		memdelete(thread_connection);
		thread_connection = nullptr;
	}

	_send_queue_resize(0);

	call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
	set_status(WorkingStatus::STATUS_STOPPED);
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
		control_to_show_in->disconnect("tree_exiting", this, "_on_node_deleting");
	}

	_remove_custom_input_scene();

	control_to_show_in = ctrl;

	if (control_to_show_in && !control_to_show_in->is_queued_for_deletion()) {
		control_to_show_in->connect("resized", this, "_viewport_size_changed");

		tex_shows_stream = memnew(GRTextureRect);
		input_collector = memnew(GRInputCollector);

		tex_shows_stream->connect("tree_exiting", this, "_on_node_deleting", vec_args({ (int)DeletingVarName::TEXTURE_TO_SHOW_STREAM }));
		input_collector->connect("tree_exiting", this, "_on_node_deleting", vec_args({ (int)DeletingVarName::INPUT_COLLECTOR }));
		control_to_show_in->connect("tree_exiting", this, "_on_node_deleting", vec_args({ (int)DeletingVarName::CONTROL_TO_SHOW_STREAM }));

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

void GRClient::_on_node_deleting(int var_name) {
	switch ((DeletingVarName)var_name) {
		case DeletingVarName::CONTROL_TO_SHOW_STREAM:
			control_to_show_in = nullptr;
			set_control_to_show_in(nullptr, 0);
			break;
		case DeletingVarName::TEXTURE_TO_SHOW_STREAM:
			tex_shows_stream = nullptr;
			break;
		case DeletingVarName::INPUT_COLLECTOR:
			input_collector = nullptr;
			break;
		case DeletingVarName::CUSTOM_INPUT_SCENE:
			custom_input_scene = nullptr;
			break;
		default:
			break;
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

void GRClient::set_connection_type(ENUM_ARG(ConnectionType) type) {
	con_type = (ConnectionType)type;
}

ENUM_ARG(GRClient::ConnectionType)
GRClient::get_connection_type() {
	return con_type;
}

void GRClient::set_target_send_fps(int fps) {
	ERR_FAIL_COND(fps <= 0);
	send_data_fps = fps;
}

int GRClient::get_target_send_fps() {
	return send_data_fps;
}

void GRClient::set_stretch_mode(ENUM_ARG(StretchMode) stretch) {
	stretch_mode = (StretchMode)stretch;
	call_deferred("_update_stream_texture_state", signal_connection_state);
}

ENUM_ARG(GRClient::StretchMode)
GRClient::get_stretch_mode() {
	return stretch_mode;
}

void GRClient::set_texture_filtering(bool is_filtering) {
	is_filtering_enabled = is_filtering;
}

bool GRClient::get_texture_filtering() {
	return is_filtering_enabled;
}

ENUM_ARG(GRClient::StreamState)
GRClient::get_stream_state() {
	return signal_connection_state;
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
		} else {
			_log("Address is invalid: " + ip, LogLevel::LL_ERROR);
			GRNotifications::add_notification("Resolve Address Error", "Address is invalid: " + ip, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
		}
	} else {
		adr = IP::get_singleton()->resolve_hostname(adr);
		if (adr.is_valid_ip()) {
			_log("Resolved address for " + ip + "\n" + adr, LogLevel::LL_DEBUG);
			server_address = ip;
			port = _port;
			restart();
			all_ok = true;
		} else {
			_log("Can't resolve address for " + ip, LogLevel::LL_ERROR);
			GRNotifications::add_notification("Resolve Address Error", "Can't resolve address: " + ip, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
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
	if (thread_connection && thread_connection->peer.is_valid()) {
		return thread_connection->peer->is_connected_to_host() && is_connection_working;
	}
	return false;
}

Node *GRClient::get_custom_input_scene() {
	return custom_input_scene;
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
		_log("Scene not specified or data is empty. Removing custom input scene", LogLevel::LL_DEBUG);
		return;
	}

	if (!control_to_show_in) {
		_log("Not specified control to show", LogLevel::LL_ERROR);
		return;
	}

	Error err = Error::OK;
#ifndef GDNATIVE_LIBRARY
	FileAccess *file = FileAccess::open(custom_input_scene_tmp_pck_file, FileAccess::ModeFlags::WRITE, &err);
#else
	File *file = memnew(File);
	err = file->open(custom_input_scene_tmp_pck_file, File::ModeFlags::WRITE);
#endif
	if ((int)err) {
		_log("Can't open temp file to store custom input scene: " + custom_input_scene_tmp_pck_file + ", code: " + str((int)err), LogLevel::LL_ERROR);
	} else {

		PoolByteArray scene_data;
		if (_data->is_compressed()) {
			err = decompress_bytes(_data->get_scene_data(), _data->get_original_size(), scene_data, _data->get_compression_type());
		} else {
			scene_data = _data->get_scene_data();
		}

		if ((int)err) {
			_log("Can't decompress or set scene_data: Code: " + str((int)err), LogLevel::LL_ERROR);
		} else {

#ifndef GDNATIVE_LIBRARY
			auto r = scene_data.read();
			file->store_buffer(r.ptr(), scene_data.size());
			release_pva_read(r);
#else
			file->store_buffer(scene_data);
#endif
			file->close();

#ifndef GDNATIVE_LIBRARY
			if (PackedData::get_singleton()->is_disabled()) {
				err = Error::FAILED;
			} else {
#if VERSION_MINOR >= 2 && VERSION_PATCH >= 4
				err = PackedData::get_singleton()->add_pack(custom_input_scene_tmp_pck_file, true, 0);
#else
				err = PackedData::get_singleton()->add_pack(custom_input_scene_tmp_pck_file, true);
#endif
			}
#else
			err = ProjectSettings::get_singleton()->load_resource_pack(custom_input_scene_tmp_pck_file, true, 0) ? Error::OK : Error::FAILED;
#endif

			if ((int)err) {
				_log("Can't load PCK file: " + custom_input_scene_tmp_pck_file, LogLevel::LL_ERROR);
			} else {

#ifndef GDNATIVE_LIBRARY
				Ref<PackedScene> pck = ResourceLoader::load(_data->get_scene_path(), "", false, &err);
#else
				Ref<PackedScene> pck = ResourceLoader::get_singleton()->load(_data->get_scene_path(), "", false);
				err = pck->can_instance() ? Error::OK : Error::FAILED;
#endif
				if ((int)err) {
					_log("Can't load scene file: " + _data->get_scene_path() + ", code: " + str((int)err), LogLevel::LL_ERROR);
				} else {

					custom_input_scene = pck->instance();
					if (!custom_input_scene) {
						_log("Can't instance scene from PCK file: " + custom_input_scene_tmp_pck_file + ", scene: " + _data->get_scene_path(), LogLevel::LL_ERROR);
					} else {

						control_to_show_in->add_child(custom_input_scene);
						custom_input_scene->connect("tree_exiting", this, "_on_node_deleting", vec_args({ (int)DeletingVarName::CUSTOM_INPUT_SCENE }));

						_reset_counters();
						emit_signal("custom_input_scene_added");
					}
				}
			}
		}
	}

	if (file) {
		memdelete(file);
	}
}

void GRClient::_remove_custom_input_scene() {
	if (custom_input_scene && !custom_input_scene->is_queued_for_deletion()) {

		custom_input_scene->queue_del();
		custom_input_scene = nullptr;
		emit_signal("custom_input_scene_removed");

		Error err = Error::OK;
#ifndef GDNATIVE_LIBRARY
		DirAccess *dir = DirAccess::open(custom_input_scene_tmp_pck_file.get_base_dir(), &err);
#else
		Directory *dir = memnew(Directory);
		dir->open(custom_input_scene_tmp_pck_file.get_base_dir());
#endif
		if ((int)err) {
			_log("Can't open folder: " + custom_input_scene_tmp_pck_file.get_base_dir(), LogLevel::LL_ERROR);
		} else {
			if (dir && dir->file_exists(custom_input_scene_tmp_pck_file)) {
				err = dir->remove(custom_input_scene_tmp_pck_file);
				if ((int)err) {
					_log("Can't delete file: " + custom_input_scene_tmp_pck_file + ". Code: " + str((int)err), LogLevel::LL_ERROR);
				}
			}
		}

		if (dir) {
			memdelete(dir);
		}
	}
}

void GRClient::_viewport_size_changed() {
	if (!control_to_show_in || control_to_show_in->is_queued_for_deletion()) {
		return;
	}

	if (_viewport_orientation_syncing) {
		Vector2 size = control_to_show_in->get_size();
		ScreenOrientation tmp_vert = size.x < size.y ? ScreenOrientation::VERTICAL : ScreenOrientation::HORIZONTAL;
		if (tmp_vert != is_vertical) {
			is_vertical = tmp_vert;
			Mutex_lock(send_queue_mutex);
			Ref<GRPacketClientStreamOrientation> packet = _find_queued_packet_by_type<Ref<GRPacketClientStreamOrientation> >();
			if (packet.is_valid()) {
				packet->set_vertical(is_vertical == ScreenOrientation::VERTICAL);
				Mutex_unlock(send_queue_mutex);
				goto ratio_sync;
			}
			Mutex_unlock(send_queue_mutex);

			if (packet.is_null()) {
				packet.instance();
				packet->set_vertical(is_vertical == ScreenOrientation::VERTICAL);
				send_packet(packet);
			}
		}
	}

ratio_sync:

	if (_viewport_aspect_ratio_syncing) {
		Vector2 size = control_to_show_in->get_size();

		Mutex_lock(send_queue_mutex);
		Ref<GRPacketClientStreamAspect> packet = _find_queued_packet_by_type<Ref<GRPacketClientStreamAspect> >();
		if (packet.is_valid()) {
			packet->set_aspect(size.x / size.y);
			Mutex_unlock(send_queue_mutex);
			return;
		}
		Mutex_unlock(send_queue_mutex);

		if (packet.is_null()) {
			packet.instance();
			packet->set_aspect(size.x / size.y);
			send_packet(packet);
		}
	}
}

void GRClient::_update_texture_from_image(Ref<Image> img) {
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

void GRClient::_update_stream_texture_state(ENUM_ARG(StreamState) _stream_state) {
	if (is_deleting)
		return;

	if (tex_shows_stream && !tex_shows_stream->is_queued_for_deletion()) {
		switch (_stream_state) {
			case StreamState::STREAM_NO_SIGNAL: {
				tex_shows_stream->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

				if (custom_no_signal_texture.is_valid() || custom_no_signal_vertical_texture.is_valid()) {
					tex_shows_stream->set_texture(no_signal_is_vertical ?
															(custom_no_signal_vertical_texture.is_valid() ? custom_no_signal_vertical_texture : custom_no_signal_texture) :
															(custom_no_signal_texture.is_valid() ? custom_no_signal_texture : custom_no_signal_vertical_texture));
				}
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
				else {
					_update_texture_from_image(no_signal_is_vertical ? no_signal_vertical_image : no_signal_image);
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
				_log("Wrong stream state!", LogLevel::LL_ERROR);
				break;
		}

		if (signal_connection_state != _stream_state) {
			call_deferred("emit_signal", "stream_state_changed", _stream_state);
			signal_connection_state = (StreamState)_stream_state;
		}
	}
}

void GRClient::_reset_counters() {
	GRDevice::_reset_counters();
	sync_time_client = 0;
	sync_time_server = 0;
}

void GRClient::set_server_setting(ENUM_ARG(TypesOfServerSettings) param, Variant value) {
	Mutex_lock(send_queue_mutex);
	Ref<GRPacketServerSettings> packet = _find_queued_packet_by_type<Ref<GRPacketServerSettings> >();
	if (packet.is_valid()) {
		packet->add_setting(param, value);
		Mutex_unlock(send_queue_mutex);
		return;
	}
	Mutex_unlock(send_queue_mutex);

	if (packet.is_null()) {
		packet.instance();
		packet->add_setting(param, value);
		send_packet(packet);
	}
}

void GRClient::disable_overriding_server_settings() {
	set_server_setting(TypesOfServerSettings::SERVER_SETTINGS_USE_INTERNAL, true);
}

//////////////////////////////////////////////
////////////////// STATIC ////////////////////
//////////////////////////////////////////////

void GRClient::_thread_connection(THREAD_DATA p_userdata) {
	ConnectionThreadParamsClient *con_thread = (ConnectionThreadParamsClient *)p_userdata;
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

		dev->_send_queue_resize(0);

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
		} else {
			if (dev->server_address.is_valid_ip_address()) {
				adr = dev->server_address;
				if (adr.is_valid_ip()) {
				} else {
					_log("Address is invalid: " + dev->server_address, LogLevel::LL_ERROR);
					if (prev_auth_error != GRDevice::AuthResult::Error)
						GRNotifications::add_notification("Resolve Address Error", "Address is invalid: " + dev->server_address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
					prev_auth_error = GRDevice::AuthResult::Error;
				}
			} else {
				adr = IP::get_singleton()->resolve_hostname(adr);
				if (adr.is_valid_ip()) {
					_log("Resolved address for " + dev->server_address + "\n" + adr, LogLevel::LL_DEBUG);
				} else {
					_log("Can't resolve address for " + dev->server_address, LogLevel::LL_ERROR);
					if (prev_auth_error != GRDevice::AuthResult::Error)
						GRNotifications::add_notification("Resolve Address Error", "Can't resolve address: " + dev->server_address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
					prev_auth_error = GRDevice::AuthResult::Error;
				}
			}
		}

		String address = (String)adr + ":" + str(dev->port);
		Error err = con->connect_to_host(adr, dev->port);

		_log("Connecting to " + address, LogLevel::LL_DEBUG);
		if ((int)err) {
			switch (err) {
				case Error::FAILED:
					_log("Failed to open socket or can't connect to host", LogLevel::LL_ERROR);
					break;
				case Error::ERR_UNAVAILABLE:
					_log("Socket is unavailable", LogLevel::LL_ERROR);
					break;
				case Error::ERR_INVALID_PARAMETER:
					_log("Host address is invalid", LogLevel::LL_ERROR);
					break;
				case Error::ERR_ALREADY_EXISTS:
					_log("Socket already in use", LogLevel::LL_ERROR);
					break;
			}
			sleep_usec(250_ms);
			continue;
		}

		while (con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			sleep_usec(1_ms);
		}

		if (con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			_log("Connection timed out with " + address, LogLevel::LL_DEBUG);
			if (prev_auth_error != GRDevice::AuthResult::Timeout) {
				GRNotifications::add_notification(con_error_title, "Connection timed out: " + address, GRNotifications::NotificationIcon::ICON_WARNING, true, 1.f);
				prev_auth_error = GRDevice::AuthResult::Timeout;
			}
			sleep_usec(200_ms);
			continue;
		}

		con->set_no_delay(true);

		bool long_wait = false;

		Ref<PacketPeerStream> ppeer = newref(PacketPeerStream);
		ppeer->set_stream_peer(con);
		ppeer->set_input_buffer_max_size(dev->input_buffer_size_in_mb * 1024 * 1024);

		GRDevice::AuthResult res = _auth_on_server(dev, ppeer);
		switch (res) {
			case GRDevice::AuthResult::OK: {
				_log("Successful connected to " + address, LogLevel::LL_NORMAL);

				dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_IMAGE);

				con_thread->break_connection = false;
				con_thread->peer = con;
				con_thread->ppeer = ppeer;

				dev->is_connection_working = true;
				dev->call_deferred("emit_signal", "connection_state_changed", true);
				dev->call_deferred("_force_update_stream_viewport_signals"); // force update screen aspect ratio and orientation
				GRNotifications::add_notification("Connected", "Connected to " + address, GRNotifications::NotificationIcon::ICON_SUCCESS, true, 1.f);

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
					GRNotifications::add_notification(con_error_title, "Can't connect to " + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				long_wait = true;
				break;
			case GRDevice::AuthResult::Timeout:
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Timeout\n" + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				long_wait = true;
				break;
			case GRDevice::AuthResult::RefuseConnection:
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Connection refused\n" + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				long_wait = true;
				break;
			case GRDevice::AuthResult::VersionMismatch:
				GRNotifications::add_notification(con_error_title, "Version mismatch\n" + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				long_wait = true;
				break;
			case GRDevice::AuthResult::IncorrectPassword:
				GRNotifications::add_notification(con_error_title, "Incorrect password\n" + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				long_wait = true;
				break;
			case GRDevice::AuthResult::PasswordRequired:
				GRNotifications::add_notification(con_error_title, "Required password but it's not implemented.... " + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				break;
			default:
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Unknown error code: " + str((int)res) + "\n" + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				_log("Unknown error code: " + str((int)res) + ". Disconnecting. " + address, LogLevel::LL_NORMAL);
				break;
		}

		((Ref<StreamPeerTCP>)ppeer->get_stream_peer())->disconnect_from_host();
		ppeer->set_output_buffer_max_size(0);
		ppeer->set_input_buffer_max_size(0);

		prev_auth_error = res;

		if (con->is_connected_to_host()) {
			con->disconnect_from_host();
		}

		if (long_wait) {
			sleep_usec(888_ms);
		}
	}

	dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_SIGNAL);
	_log("Connection thread stopped", LogLevel::LL_DEBUG);
	con_thread->finished = true;
}

void GRClient::_connection_loop(ConnectionThreadParamsClient *con_thread) {
	GRClient *dev = con_thread->dev;
	Ref<StreamPeerTCP> connection = con_thread->peer;
	Ref<PacketPeerStream> ppeer = con_thread->ppeer;
	// Data sync with _img_thread
	ImgProcessingStorageClient *ipsc = memnew(ImgProcessingStorageClient);
	ipsc->dev = dev;

	Thread_define(_img_thread);
	Thread_start(_img_thread, GRClient, _thread_image_decoder, ipsc, dev);

	OS *os = OS::get_singleton();
	Error err = Error::OK;
	String address = CONNECTION_ADDRESS(connection);

	dev->_reset_counters();

	//Array stream_queue; // Ref<GRPacketImageData>
	std::vector<Ref<GRPacketImageData> > stream_queue; // Ref<GRPacketImageData>

	uint64_t time64 = os->get_ticks_usec();
	uint64_t prev_cycle_time = 0;
	uint64_t prev_send_input_time = time64;
	uint64_t prev_ping_sending_time = time64;
	uint64_t next_image_required_frametime = time64;
	uint64_t prev_display_image_time = time64 - 16_ms;

	bool ping_sended = false;

	TimeCountInit();
	while (!con_thread->break_connection && !con_thread->stop_thread && connection->is_connected_to_host()) {
		Mutex_lock(dev->connection_mutex);
		TimeCount("Cycle start");
		uint64_t cycle_start_time = os->get_ticks_usec();

		bool nothing_happens = true;
		uint64_t start_while_time = 0;
		dev->prev_valid_connection_time = time64;
		int send_data_time_us = (1000000 / dev->send_data_fps);

		///////////////////////////////////////////////////////////////////
		// SENDING
		bool is_queued_send = false; // this placed here for android compiler

		// INPUT
		TimeCountReset();
		time64 = os->get_ticks_usec();
		if ((time64 - prev_send_input_time) > send_data_time_us) {
			prev_send_input_time = time64;
			nothing_happens = false;

			if (dev->input_collector) {
				Ref<GRPacketInputData> pack = dev->input_collector->get_collected_input_data();

				if (pack.is_valid()) {
					err = ppeer->put_var(pack->get_data());
					if ((int)err) {
						_log("Put input data failed with code: " + str((int)err), LogLevel::LL_ERROR);
						goto end_send;
					}
				} else {
					_log("Can't get input data from input collector", LogLevel::LL_ERROR);
				}
				TimeCount("Input send");
			}
		}

		// PING
		TimeCountReset();
		time64 = os->get_ticks_usec();
		if ((time64 - prev_ping_sending_time) > 100_ms && !ping_sended) {
			nothing_happens = false;
			ping_sended = true;

			Ref<GRPacketPing> pack(memnew(GRPacketPing));
			err = ppeer->put_var(pack->get_data());
			prev_ping_sending_time = time64;

			if ((int)err) {
				_log("Send ping failed with code: " + str((int)err), LogLevel::LL_ERROR);
				goto end_send;
			}
			TimeCount("Ping send");
		}

		// SEND QUEUE
		start_while_time = os->get_ticks_usec();
		while (!dev->send_queue.empty() && (os->get_ticks_usec() - start_while_time) <= send_data_time_us / 2) {
			is_queued_send = true;
			Ref<GRPacket> packet = dev->_send_queue_pop_front();

			if (packet.is_valid()) {
				err = ppeer->put_var(packet->get_data());

				if ((int)err) {
					_log("Put data from queue failed with code: " + str((int)err), LogLevel::LL_ERROR);
					goto end_send;
				}
			}
		}
		if (is_queued_send) {
			TimeCount("Send queued data");
		}
	end_send:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after sending!", LogLevel::LL_ERROR);
			GRNotifications::add_notification("Error", "Lost connection after sending data!", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
			Mutex_unlock(dev->connection_mutex);
			continue;
		}

		///////////////////////////////////////////////////////////////////
		// RECEIVING

		// Send to processing one of buffered images
		time64 = os->get_ticks_usec();
		TimeCountReset();
		if (!ipsc->_is_processing_img && !stream_queue.empty() && time64 >= next_image_required_frametime) {
			nothing_happens = false;

			Ref<GRPacketImageData> pack = stream_queue.front();
			stream_queue.erase(stream_queue.begin());

			if (pack.is_null()) {
				_log("Queued image data is null", LogLevel::LL_ERROR);
				goto end_img_process;
			}

			uint64_t frametime = pack->get_frametime() > 1000_ms ? 1000_ms : pack->get_frametime();
			next_image_required_frametime = time64 + frametime - prev_cycle_time;

			dev->_update_avg_fps(time64 - prev_display_image_time);
			prev_display_image_time = time64;

			if (pack->get_is_empty()) {
				dev->_update_avg_fps(0);
				dev->call_deferred("_update_texture_from_image", Ref<Image>());
				dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_IMAGE);
			} else {
				ipsc->tex_data = pack->get_image_data();
				ipsc->compression_type = (ImageCompressionType)pack->get_compression_type();
				ipsc->size = pack->get_size();
				ipsc->format = pack->get_format();
				ipsc->_is_processing_img = true;
			}

			pack.unref();
			TimeCount("Get image from queue");
		}
	end_img_process:

		// check if image displayed less then few seconds ago. if not then remove texture
		const double image_loss_time = 1.5;
		if (os->get_ticks_usec() > int64_t(prev_display_image_time + uint64_t(1000_ms * image_loss_time))) {
			if (dev->signal_connection_state != StreamState::STREAM_NO_IMAGE) {
				dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_NO_IMAGE);
				dev->_reset_counters();
			}
		}

		if (stream_queue.size() > 10) {
			//for (int i = 0; i<stream_queue.size(); i++)
			//{
			//	((Ref<GRPacketImageData>)stream_queue[i]).unref();
			//}
			stream_queue.clear();
		}

		// Get some packets
		TimeCountReset();
		start_while_time = os->get_ticks_usec();
		while (ppeer->get_available_packet_count() > 0 && (os->get_ticks_usec() - start_while_time) <= send_data_time_us / 2) {
			nothing_happens = false;

#ifndef GDNATIVE_LIBRARY
			Variant buf;
			err = ppeer->get_var(buf);
#else
			Variant buf = ppeer->get_var();
#endif

			if ((int)err)
				goto end_recv;

			Ref<GRPacket> pack = GRPacket::create(buf);
			if (pack.is_null()) {
				_log("Incorrect GRPacket", LogLevel::LL_ERROR);
				continue;
			}

			GRPacket::PacketType type = pack->get_type();

			switch (type) {
				case GRPacket::PacketType::SyncTime: {
					Ref<GRPacketSyncTime> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketSyncTime", LogLevel::LL_ERROR);
						continue;
					}

					dev->sync_time_client = os->get_ticks_usec();
					dev->sync_time_server = data->get_time();

					break;
				}
				case GRPacket::PacketType::ImageData: {
					Ref<GRPacketImageData> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketImageData", LogLevel::LL_ERROR);
						continue;
					}

					stream_queue.push_back(data);
					break;
				}
				case GRPacket::PacketType::ServerSettings: {
					if (!dev->_server_settings_syncing) {
						continue;
					}

					Ref<GRPacketServerSettings> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketServerSettings", LogLevel::LL_ERROR);
						continue;
					}

					dev->call_deferred("emit_signal", "server_settings_received", map_to_dict(data->get_settings()));
					break;
				}
				case GRPacket::PacketType::MouseModeSync: {
					Ref<GRPacketMouseModeSync> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketMouseModeSync", LogLevel::LL_ERROR);
						continue;
					}

					dev->call_deferred("emit_signal", "mouse_mode_changed", data->get_mouse_mode());
					break;
				}
				case GRPacket::PacketType::CustomInputScene: {
					Ref<GRPacketCustomInputScene> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketCustomInputScene", LogLevel::LL_ERROR);
						continue;
					}

					dev->call_deferred("_load_custom_input_scene", data);
					break;
				}
				case GRPacket::PacketType::CustomUserData: {
					Ref<GRPacketCustomUserData> data = pack;
					if (data.is_null()) {
						_log("Incorrect GRPacketCustomUserData", LogLevel::LL_ERROR);
						break;
					}
					dev->call_deferred("emit_signal", "user_data_received", data->get_packet_id(), data->get_user_data());
					break;
				}
				case GRPacket::PacketType::Ping: {
					Ref<GRPacketPong> pack(memnew(GRPacketPong));
					err = ppeer->put_var(pack->get_data());
					if ((int)err) {
						_log("Send pong failed with code: " + str((int)err), LogLevel::LL_NORMAL);
						break;
					}
					break;
				}
				case GRPacket::PacketType::Pong: {
					dev->_update_avg_ping(os->get_ticks_usec() - prev_ping_sending_time);
					ping_sended = false;
					break;
				}
				default:
					_log("Not supported packet type! " + str((int)type), LogLevel::LL_WARNING);
					break;
			}
		}
		TimeCount("End receiving");
	end_recv:
		Mutex_unlock(dev->connection_mutex);

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after receiving!", LogLevel::LL_ERROR);
			GRNotifications::add_notification("Error", "Lost connection after receiving data!", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
			continue;
		}

		if (nothing_happens)
			sleep_usec(1_ms);

		prev_cycle_time = os->get_ticks_usec() - cycle_start_time;
	}

	dev->_send_queue_resize(0);
	stream_queue.clear();

	if (connection->is_connected_to_host()) {
		_log("Lost connection to " + address, LogLevel::LL_ERROR);
		GRNotifications::add_notification("Disconnected", "Closing connection to " + address, GRNotifications::NotificationIcon::ICON_FAIL, true, 1.f);
	} else {
		_log("Closing connection to " + address, LogLevel::LL_ERROR);
		GRNotifications::add_notification("Disconnected", "Lost connection to " + address, GRNotifications::NotificationIcon::ICON_FAIL, true, 1.f);
	}

	ipsc->_thread_closing = true;
	Thread_close(_img_thread);
	memdelete(ipsc);

	_log("Closing connection", LogLevel::LL_NORMAL);
	con_thread->break_connection = true;
}

void GRClient::_thread_image_decoder(THREAD_DATA p_userdata) {
	ImgProcessingStorageClient *ipsc = (ImgProcessingStorageClient *)p_userdata;
	GRClient *dev = ipsc->dev;
	Error err = Error::OK;

	while (!ipsc->_thread_closing) {
		if (!ipsc->_is_processing_img) {
			sleep_usec(1_ms);
			continue;
		}

		Ref<Image> img(memnew(Image));
		ImageCompressionType type = ipsc->compression_type;

		TimeCountInit();
		switch (type) {
			case ImageCompressionType::COMPRESSION_UNCOMPRESSED: {
#ifndef GDNATIVE_LIBRARY
				img->create(ipsc->size.x, ipsc->size.y, false, (Image::Format)ipsc->format, ipsc->tex_data);
#else
				img->create_from_data((int)ipsc->size.x, (int)ipsc->size.y, false, (Image::Format)ipsc->format, ipsc->tex_data);
#endif
				if (img_is_empty(img)) { // is NOT OK
					err = Error::FAILED;
					_log("Incorrect uncompressed image data.", LogLevel::LL_ERROR);
					GRNotifications::add_notification("Stream Error", "Incorrect uncompressed image data.", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				}
				break;
			}
			case ImageCompressionType::COMPRESSION_JPG: {
				err = img->load_jpg_from_buffer(ipsc->tex_data);
				if ((int)err || img_is_empty(img)) { // is NOT OK
					_log("Can't decode JPG image.", LogLevel::LL_ERROR);
					GRNotifications::add_notification("Stream Error", "Can't decode JPG image. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				}
				break;
			}
			case ImageCompressionType::COMPRESSION_PNG: {
				err = img->load_png_from_buffer(ipsc->tex_data);
				if ((int)err || img_is_empty(img)) { // is NOT OK
					_log("Can't decode PNG image.", LogLevel::LL_ERROR);
					GRNotifications::add_notification("Stream Error", "Can't decode PNG image. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				}
				break;
			}
			default:
				_log("Not implemented image decoder type: " + str((int)type), LogLevel::LL_ERROR);
				break;
		}

		if (!(int)err) { // is OK
			TimeCount("Create Image Time");
			dev->call_deferred("_update_texture_from_image", img);

			if (dev->signal_connection_state != StreamState::STREAM_ACTIVE) {
				dev->call_deferred("_update_stream_texture_state", StreamState::STREAM_ACTIVE);
			}
		}

		ipsc->_is_processing_img = false;
	}
}

GRDevice::AuthResult GRClient::_auth_on_server(GRClient *dev, Ref<PacketPeerStream> &ppeer) {
#define wait_packet(_n)                                                                        \
	time = (uint32_t)OS::get_singleton()->get_ticks_msec();                                    \
	while (ppeer->get_available_packet_count() == 0) {                                         \
		if (OS::get_singleton()->get_ticks_msec() - time > 150) {                              \
			_log("Connection timeout. Disconnecting. Waited: " + str(_n), LogLevel::LL_DEBUG); \
			goto timeout;                                                                      \
		}                                                                                      \
		if (!con->is_connected_to_host()) {                                                    \
			return GRDevice::AuthResult::Error;                                                \
		}                                                                                      \
		sleep_usec(1_ms);                                                                      \
	}
#define packet_error_check(_t)              \
	if ((int)err) {                         \
		_log(_t, LogLevel::LL_DEBUG);       \
		return GRDevice::AuthResult::Error; \
	}

	Ref<StreamPeerTCP> con = ppeer->get_stream_peer();
	String address = CONNECTION_ADDRESS(con);
	uint32_t time = 0;

	Error err = Error::OK;
	Variant ret;
	// GET first packet
	wait_packet("first_packet");
#ifndef GDNATIVE_LIBRARY
	err = ppeer->get_var(ret);
	packet_error_check("Can't get first authorization packet from server. Code: " + str((int)err));
#else
	err = Error::OK;
	ret = ppeer->get_var();
#endif

	if ((int)ret == (int)GRDevice::AuthResult::RefuseConnection) {
		_log("Connection refused", LogLevel::LL_ERROR);
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
		err = Error::OK;
		ret = ppeer->get_var();
#endif

		if ((int)ret == (int)GRDevice::AuthResult::OK) {
			return GRDevice::AuthResult::OK;
		} else {
			GRDevice::AuthResult r = (GRDevice::AuthResult)(int)ret;
			switch (r) {
				case GRDevice::AuthResult::Error:
					_log("Can't connect to server", LogLevel::LL_ERROR);
					return r;
				case GRDevice::AuthResult::VersionMismatch:
					_log("Version mismatch", LogLevel::LL_ERROR);
					return r;
				case GRDevice::AuthResult::IncorrectPassword:
					_log("Incorrect password", LogLevel::LL_ERROR);
					return r;
			}
		}
	}

	return GRDevice::AuthResult::Error;

timeout:
	con->disconnect_from_host();
	_log("Connection timeout. Disconnecting", LogLevel::LL_NORMAL);
	return GRDevice::AuthResult::Timeout;

#undef wait_packet
#undef packet_error_check
}

//////////////////////////////////////////////
///////////// INPUT COLLECTOR ////////////////
//////////////////////////////////////////////

void GRInputCollector::_update_stream_rect() {
	if (!dev || dev->get_status() != GRDevice::WorkingStatus::STATUS_WORKING)
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
		buttons.clear();
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
		touches.clear();
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
	METHOD_REG(GRInputCollector, _notification);

	METHOD_REG(GRInputCollector, _input);

	METHOD_REG(GRInputCollector, is_capture_on_focus);
	METHOD_REG(GRInputCollector, set_capture_on_focus);
	METHOD_REG(GRInputCollector, is_capture_when_hover);
	METHOD_REG(GRInputCollector, set_capture_when_hover);
	METHOD_REG(GRInputCollector, is_capture_pointer);
	METHOD_REG(GRInputCollector, set_capture_pointer);

	register_property<GRInputCollector, bool>("capture_on_focus", &GRInputCollector::set_capture_on_focus, &GRInputCollector::is_capture_on_focus, false);
	register_property<GRInputCollector, bool>("capture_when_hover", &GRInputCollector::set_capture_when_hover, &GRInputCollector::is_capture_when_hover, true);
	register_property<GRInputCollector, bool>("capture_pointer", &GRInputCollector::set_capture_pointer, &GRInputCollector::is_capture_pointer, true);
}

#endif

void GRInputCollector::_input(Ref<InputEvent> ie) {
	if (!parent || (capture_only_when_control_in_focus && !parent->has_focus()) ||
			(dev && dev->get_status() != GRDevice::WorkingStatus::STATUS_WORKING) ||
			!dev->is_stream_active() || !is_inside_tree()) {
		return;
	}

	_TS_LOCK_;
	if (collected_input_data.size() >= 256) {
		collected_input_data.resize(0);
	}
	_TS_UNLOCK_;

	_update_stream_rect();

	if (ie.is_null()) {
		_log("InputEvent is null", LogLevel::LL_ERROR);
		return;
	}

	{
		Ref<InputEventMouseButton> iemb = ie;
		if (iemb.is_valid()) {
			int idx = (int)iemb->get_button_index();

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
			int idx = (int)iest->get_index();
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
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
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
			release_pva_write(w);
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

	_TS_LOCK_;

	s->set_sensors(sensors);
	collected_input_data.push_back(s);
	res->set_input_data(collected_input_data);
	collected_input_data.resize(0);

	_TS_UNLOCK_;
	return res;
}

void GRInputCollector::_init() {
	LEAVE_IF_EDITOR();
	_TS_LOCK_;
	parent = nullptr;
	set_process(true);
	set_process_input(true);
	sensors.resize(4);
	_TS_UNLOCK_;
}

void GRInputCollector::_deinit() {
	LEAVE_IF_EDITOR();
	_TS_LOCK_;
	sensors.resize(0);
	collected_input_data.resize(0);
	if (this_in_client)
		*this_in_client = nullptr;
	mouse_buttons.clear();
	screen_touches.clear();
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
	METHOD_REG(GRTextureRect, _notification);

	METHOD_REG(GRTextureRect, _tex_size_changed);
}

#endif

void GRTextureRect::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
	}
}

void GRTextureRect::_init() {
	LEAVE_IF_EDITOR();
	connect("resized", this, "_tex_size_changed");
}

void GRTextureRect::_deinit() {
	if (this_in_client)
		*this_in_client = nullptr;
	LEAVE_IF_EDITOR();
	disconnect("resized", this, "_tex_size_changed");
}

#endif // !NO_GODOTREMOTE_CLIENT

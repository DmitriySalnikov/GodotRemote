/* GRClient.cpp */

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRClient.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRResources.h"
#include "GodotRemote.h"
#include "UDPSocket.h"

#ifndef GDNATIVE_LIBRARY

#include "core/input_map.h"
#include "core/io/file_access_pack.h"
#include "core/io/ip.h"
#include "core/io/resource_loader.h"
#include "core/io/tcp_server.h"
#include "core/io/udp_server.h"
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

#include <Control.hpp>
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
#include <Shader.hpp>
#include <TCP_Server.hpp>
#include <Texture.hpp>
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
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_texture_from_image), "image"), &GRClient::_update_texture_from_image);
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_stream_texture_state), "state"), &GRClient::_update_stream_texture_state);
	ClassDB::bind_method(D_METHOD(NAMEOF(_force_update_stream_viewport_signals)), &GRClient::_force_update_stream_viewport_signals);
	ClassDB::bind_method(D_METHOD(NAMEOF(_viewport_size_changed)), &GRClient::_viewport_size_changed);
	ClassDB::bind_method(D_METHOD(NAMEOF(_load_custom_input_scene), "path", "pck_data", "orig_size", "is_compressed", "compression_type"), &GRClient::_load_custom_input_scene);
	ClassDB::bind_method(D_METHOD(NAMEOF(_remove_custom_input_scene)), &GRClient::_remove_custom_input_scene);
	ClassDB::bind_method(D_METHOD(NAMEOF(_on_node_deleting), "var_name"), &GRClient::_on_node_deleting);
	ClassDB::bind_method(D_METHOD(NAMEOF(_thread_connection), "user_data"), &GRClient::_thread_connection);

	ClassDB::bind_method(D_METHOD(NAMEOF(set_control_to_show_in), "control_node", "position_in_node"), &GRClient::set_control_to_show_in, DEFVAL(0));
	ClassDB::bind_method(D_METHOD(NAMEOF(set_custom_no_signal_texture), "texture"), &GRClient::set_custom_no_signal_texture);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_custom_no_signal_vertical_texture), "texture"), &GRClient::set_custom_no_signal_vertical_texture);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_custom_no_signal_material), "material"), &GRClient::set_custom_no_signal_material);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_address_port), "ip", "port"), &GRClient::set_address_port);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_address), "ip"), &GRClient::set_address);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_server_setting), "setting", "value"), &GRClient::set_server_setting);
	ClassDB::bind_method(D_METHOD(NAMEOF(disable_overriding_server_settings)), &GRClient::disable_overriding_server_settings);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_avg_delay)), &GRClient::get_avg_delay);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_min_delay)), &GRClient::get_min_delay);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_max_delay)), &GRClient::get_max_delay);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_stream_aspect_ratio)), &GRClient::get_stream_aspect_ratio);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_custom_input_scene)), &GRClient::get_custom_input_scene);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_address)), &GRClient::get_address);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_stream_active)), &GRClient::is_stream_active);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_connected_to_host)), &GRClient::is_connected_to_host);

	ClassDB::bind_method(D_METHOD(NAMEOF(set_decoder_threads_count), "count"), &GRClient::set_decoder_threads_count);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_decoder_threads_count)), &GRClient::get_decoder_threads_count);

	ClassDB::bind_method(D_METHOD(NAMEOF(break_connection_async)), &GRClient::break_connection_async);

	ADD_SIGNAL(MethodInfo("custom_input_scene_added"));
	ADD_SIGNAL(MethodInfo("custom_input_scene_removed"));

	ADD_SIGNAL(MethodInfo("auto_connection_listener_status_changed", PropertyInfo(Variant::BOOL, "is_listening")));
	ADD_SIGNAL(MethodInfo("auto_connection_list_changed", PropertyInfo(Variant::ARRAY, "available_connections")));

	ADD_SIGNAL(MethodInfo("stream_state_changed", PropertyInfo(Variant::INT, "state")));
	ADD_SIGNAL(MethodInfo("stream_aspect_ratio_changed", PropertyInfo(Variant::REAL, "ratio")));
	ADD_SIGNAL(MethodInfo("connection_state_changed", PropertyInfo(Variant::BOOL, "is_connected")));
	ADD_SIGNAL(MethodInfo("mouse_mode_changed", PropertyInfo(Variant::INT, "mouse_mode")));
	ADD_SIGNAL(MethodInfo("server_settings_received", PropertyInfo(Variant::DICTIONARY, "settings")));
	ADD_SIGNAL(MethodInfo("server_quality_hint_setting_received", PropertyInfo(Variant::STRING, "quality_hint")));

	// SETGET
	ClassDB::bind_method(D_METHOD(NAMEOF(set_capture_on_focus), "val"), &GRClient::set_capture_on_focus);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_capture_when_hover), "val"), &GRClient::set_capture_when_hover);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_capture_pointer), "val"), &GRClient::set_capture_pointer);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_capture_input), "val"), &GRClient::set_capture_input);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_connection_type), "type"), &GRClient::set_connection_type);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_target_send_fps), "fps"), &GRClient::set_target_send_fps);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_stretch_mode), "mode"), &GRClient::set_stretch_mode);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_texture_filtering), "is_filtered"), &GRClient::set_texture_filtering);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_password), "password"), &GRClient::set_password);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_device_id), "id"), &GRClient::set_device_id);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_viewport_orientation_syncing), "is_syncing"), &GRClient::set_viewport_orientation_syncing);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_viewport_aspect_ratio_syncing), "is_syncing"), &GRClient::set_viewport_aspect_ratio_syncing);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_server_settings_syncing), "is_syncing"), &GRClient::set_server_settings_syncing);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_current_auto_connect_address), "address"), &GRClient::set_current_auto_connect_address);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_current_auto_connect_port), "port"), &GRClient::set_current_auto_connect_port);

	ClassDB::bind_method(D_METHOD(NAMEOF(is_capture_on_focus)), &GRClient::is_capture_on_focus);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_capture_when_hover)), &GRClient::is_capture_when_hover);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_capture_pointer)), &GRClient::is_capture_pointer);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_capture_input)), &GRClient::is_capture_input);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_connection_type)), &GRClient::get_connection_type);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_target_send_fps)), &GRClient::get_target_send_fps);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_stretch_mode)), &GRClient::get_stretch_mode);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_texture_filtering)), &GRClient::get_texture_filtering);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_password)), &GRClient::get_password);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_device_id)), &GRClient::get_device_id);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_viewport_orientation_syncing)), &GRClient::is_viewport_orientation_syncing);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_viewport_aspect_ratio_syncing)), &GRClient::is_viewport_aspect_ratio_syncing);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_server_settings_syncing)), &GRClient::is_server_settings_syncing);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_current_auto_connect_address)), &GRClient::get_current_auto_connect_address);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_current_auto_connect_port)), &GRClient::get_current_auto_connect_port);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), NAMEOF(set_capture_on_focus), NAMEOF(is_capture_on_focus));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), NAMEOF(set_capture_when_hover), NAMEOF(is_capture_when_hover));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_pointer"), NAMEOF(set_capture_pointer), NAMEOF(is_capture_pointer));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_input"), NAMEOF(set_capture_input), NAMEOF(is_capture_input));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_type", PROPERTY_HINT_ENUM, "WiFi,ADB"), NAMEOF(set_connection_type), NAMEOF(get_connection_type));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_send_fps", PROPERTY_HINT_RANGE, "1,1000"), NAMEOF(set_target_send_fps), NAMEOF(get_target_send_fps));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stretch_mode", PROPERTY_HINT_ENUM, "Fill,Keep Aspect"), NAMEOF(set_stretch_mode), NAMEOF(get_stretch_mode));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "texture_filtering"), NAMEOF(set_texture_filtering), NAMEOF(get_texture_filtering));
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "password"), NAMEOF(set_password), NAMEOF(get_password));
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "device_id"), NAMEOF(set_device_id), NAMEOF(get_device_id));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "viewport_orientation_syncing"), NAMEOF(set_viewport_orientation_syncing), NAMEOF(is_viewport_orientation_syncing));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "viewport_aspect_ratio_syncing"), NAMEOF(set_viewport_aspect_ratio_syncing), NAMEOF(is_viewport_aspect_ratio_syncing));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "server_settings_syncing"), NAMEOF(set_server_settings_syncing), NAMEOF(is_server_settings_syncing));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "current_auto_connect_address"), NAMEOF(set_current_auto_connect_address), NAMEOF(get_current_auto_connect_address));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "current_auto_connect_port"), NAMEOF(set_current_auto_connect_port), NAMEOF(get_current_auto_connect_port));

	BIND_ENUM_CONSTANT(CONNECTION_ADB);
	BIND_ENUM_CONSTANT(CONNECTION_WiFi);
	BIND_ENUM_CONSTANT(CONNECTION_AUTO);

	BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT);
	BIND_ENUM_CONSTANT(STRETCH_FILL);

	BIND_ENUM_CONSTANT(STREAM_NO_SIGNAL);
	BIND_ENUM_CONSTANT(STREAM_ACTIVE);
	BIND_ENUM_CONSTANT(STREAM_NO_IMAGE);
}

#else

void GRClient::_register_methods() {
	METHOD_REG(GRClient, _notification);
	METHOD_REG(GRClient, _thread_connection);

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

	METHOD_REG(GRClient, get_avg_delay);
	METHOD_REG(GRClient, get_min_delay);
	METHOD_REG(GRClient, get_max_delay);
	METHOD_REG(GRClient, get_stream_aspect_ratio);

	METHOD_REG(GRClient, set_decoder_threads_count);
	METHOD_REG(GRClient, get_decoder_threads_count);

	METHOD_REG(GRClient, break_connection_async);

	register_signal<GRClient>("auto_connection_listener_status_changed", "is_listening", GODOT_VARIANT_TYPE_BOOL);
	register_signal<GRClient>("auto_connection_list_changed", "available_connections", GODOT_VARIANT_TYPE_ARRAY);

	register_signal<GRClient>("custom_input_scene_added", Dictionary::make());
	register_signal<GRClient>("custom_input_scene_removed", Dictionary::make());

	register_signal<GRClient>("stream_state_changed", "state", GODOT_VARIANT_TYPE_INT);
	register_signal<GRClient>("stream_aspect_ratio_changed", "ratio", GODOT_VARIANT_TYPE_REAL);
	register_signal<GRClient>("connection_state_changed", "is_connected", GODOT_VARIANT_TYPE_BOOL);
	register_signal<GRClient>("mouse_mode_changed", "mouse_mode", GODOT_VARIANT_TYPE_INT);
	register_signal<GRClient>("server_settings_received", "settings", GODOT_VARIANT_TYPE_DICTIONARY);
	register_signal<GRClient>("server_quality_hint_setting_received", "quality_hint", GODOT_VARIANT_TYPE_STRING);

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
	METHOD_REG(GRClient, set_current_auto_connect_address);
	METHOD_REG(GRClient, set_current_auto_connect_port);

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
	METHOD_REG(GRClient, get_current_auto_connect_address);
	METHOD_REG(GRClient, get_current_auto_connect_port);

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
	register_property<GRClient, String>("current_auto_connect_address", &GRClient::set_current_auto_connect_address, &GRClient::get_current_auto_connect_address, "None");
	register_property<GRClient, int>("current_auto_connect_port", &GRClient::set_current_auto_connect_port, &GRClient::get_current_auto_connect_port, 0);
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
		}
		case NOTIFICATION_EXIT_TREE: {
			is_deleting = true;
			if (get_status() == (int)WorkingStatus::STATUS_WORKING) {
				_internal_call_only_deffered_stop();
			}
			break;
		}
		case NOTIFICATION_PROCESS: {
			FrameMark;
			break;
		}
	}
}

void GRClient::_init() {
	set_name("GodotRemoteClient");
	LEAVE_IF_EDITOR();
	TracyAppInfo("Godot Remote Client", 20);
	set_process(true);

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

	Scoped_lock(stream_mutex);
	stream_manager = shared_new(GRStreamDecodersManager);
	stream_manager->set_gr_client(this);
	stream_manager->set_threads_count(default_decoder_threads_count);
}

void GRClient::_deinit() {
	LEAVE_IF_EDITOR();

	Scoped_lock(stream_mutex);
	if (stream_manager) {
		stream_manager = nullptr;
	}

	is_deleting = true;
	if (get_status() == (int)WorkingStatus::STATUS_WORKING) {
		_internal_call_only_deffered_stop();
	}
	//set_control_to_show_in(nullptr, 0);

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	no_signal_mat.unref();
	no_signal_image.unref();
	no_signal_vertical_image.unref();
#endif
}

void GRClient::_internal_call_only_deffered_start() {
	ZoneScopedNC("Client Start", tracy::Color::Green3);

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
		connection_mutex.lock();
		thread_connection->break_connection = true;
		thread_connection->stop_thread = true;
		connection_mutex.unlock();
		thread_connection->close_thread();
		memdelete(thread_connection);
		thread_connection = nullptr;
	}
	thread_connection = memnew(ConnectionThreadParamsClient);
	thread_connection->peer.instance();
	Thread_start(thread_connection->thread_ref, this, _thread_connection, thread_connection);

	call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_NO_SIGNAL);
	set_status(WorkingStatus::STATUS_WORKING);
}

void GRClient::_internal_call_only_deffered_stop() {
	ZoneScopedNC("Client Stop", tracy::Color::Red3);

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
		connection_mutex.lock();
		thread_connection->break_connection = true;
		thread_connection->stop_thread = true;
		connection_mutex.unlock();
		thread_connection->close_thread();
		memdelete(thread_connection);
		thread_connection = nullptr;
	}

	_send_queue_resize(0);

	call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_NO_SIGNAL);
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
			control_to_show_in->is_connected("resized", this, NAMEOF(_viewport_size_changed))) {
		control_to_show_in->disconnect("resized", this, NAMEOF(_viewport_size_changed));
		control_to_show_in->disconnect("tree_exiting", this, NAMEOF(_on_node_deleting));
	}

	_remove_custom_input_scene();

	control_to_show_in = ctrl;

	if (control_to_show_in && !control_to_show_in->is_queued_for_deletion()) {
		control_to_show_in->connect("resized", this, NAMEOF(_viewport_size_changed));

		tex_shows_stream = memnew(GRTextureRect);
		input_collector = memnew(GRInputCollector);

		tex_shows_stream->connect("tree_exiting", this, NAMEOF(_on_node_deleting), vec_args({ (int)DeletingVarName::TEXTURE_TO_SHOW_STREAM }));
		input_collector->connect("tree_exiting", this, NAMEOF(_on_node_deleting), vec_args({ (int)DeletingVarName::INPUT_COLLECTOR }));
		control_to_show_in->connect("tree_exiting", this, NAMEOF(_on_node_deleting), vec_args({ (int)DeletingVarName::CONTROL_TO_SHOW_STREAM }));

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
		call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_NO_SIGNAL);
		call_deferred(NAMEOF(_force_update_stream_viewport_signals)); // force update if client connected faster than scene loads
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

void GRClient::_stop_decoder() {
	Scoped_lock(stream_mutex);
	if (stream_manager) {
		stream_manager->set_active(false);
	}
}

void GRClient::_push_pack_to_decoder(std::shared_ptr<GRPacketStreamData> pack) {
	Scoped_lock(stream_mutex);
	if (stream_manager) {
		stream_manager->push_packet_to_decode(pack);
	}
}

void GRClient::_image_lost() {
	if (signal_connection_state != StreamState::STREAM_NO_IMAGE) {
		call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_NO_IMAGE);
		_reset_counters();
	}
}

void GRClient::_display_new_image(PoolByteArray data, int width, int height, uint64_t delay) {
	ZoneScopedNC("Displaying new Image", tracy::Color::LightGoldenrod3);
	Ref<Image> img = newref(Image);
#ifndef GDNATIVE_LIBRARY
	img->create(width, height, false, Image::Format::FORMAT_RGB8, data);
#else
	img->create_from_data(width, height, false, Image::Format::FORMAT_RGB8, data);
#endif

	if (!img_is_empty(img)) {
		call_deferred(NAMEOF(_update_texture_from_image), img);
		TracyPlot("FPS", int64_t(float(1000000.0 / (get_time_usec() - prev_shown_frame_time))));
		_update_avg_delay(delay);
		_update_avg_fps(get_time_usec() - prev_shown_frame_time);
		prev_shown_frame_time = get_time_usec();

		if (signal_connection_state != StreamState::STREAM_ACTIVE) {
			call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_ACTIVE);
		}
	}
}

void GRClient::set_decoder_threads_count(int count) {
	Scoped_lock(stream_mutex);
	if (stream_manager)
		stream_manager->set_threads_count(count);
}

int GRClient::get_decoder_threads_count() {
	Scoped_lock(stream_mutex);
	if (stream_manager)
		return stream_manager->get_threads_count();
	return default_decoder_threads_count;
}

void GRClient::set_custom_no_signal_texture(Ref<Texture> custom_tex) {
	custom_no_signal_texture = custom_tex;
	call_deferred(NAMEOF(_update_stream_texture_state), signal_connection_state);
}

void GRClient::set_custom_no_signal_vertical_texture(Ref<Texture> custom_tex) {
	custom_no_signal_vertical_texture = custom_tex;
	call_deferred(NAMEOF(_update_stream_texture_state), signal_connection_state);
}

void GRClient::set_custom_no_signal_material(Ref<Material> custom_mat) {
	custom_no_signal_material = custom_mat;
	call_deferred(NAMEOF(_update_stream_texture_state), signal_connection_state);
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
	if (connection_type != (ConnectionType)type) {
		break_connection_async();
		connection_type = (ConnectionType)type;
	}
}

ENUM_ARG(GRClient::ConnectionType)
GRClient::get_connection_type() {
	return connection_type;
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
	call_deferred(NAMEOF(_update_stream_texture_state), signal_connection_state);
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

Array GRClient::get_found_auto_connection_addresses() {
	Scoped_lock(ts_lock);
	Array arr;
	for (auto i : found_server_addresses) {
		Dictionary dict;
		dict["version"] = i->version;
		dict["ip"] = i->sender_ip_address;
		dict["project_name"] = i->project_name;
		dict["port"] = i->port;
		dict["addresses"] = vec_to_arr(i->recieved_from_addresses);

		Ref<Image> img;
		if (i->icon_data.size() > 0) {
			img = newref(Image);
			img->load_png_from_buffer(i->icon_data);
		}
		dict["icon"] = img;

		arr.append(dict);
	}
	return arr;
}

bool GRClient::set_address(String ip) {
	return set_address_port(ip, port);
}

bool GRClient::set_address_port(String ip, uint16_t _port) {
	ZoneScopedNC("Set Server Address and Port", tracy::Color::Blue4);
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
			break_connection();
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
			break_connection();
			all_ok = true;
		} else {
			_log("Can't resolve address for " + ip, LogLevel::LL_ERROR);
			GRNotifications::add_notification("Resolve Address Error", "Can't resolve address: " + ip, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
		}
	}

	return all_ok;
}

void GRClient::set_input_buffer_size(int mb) {
	input_buffer_size_in_mb = mb;
	restart();
}

float GRClient::get_avg_delay() {
	return delay_counter.get_avg();
}

float GRClient::get_min_delay() {
	return delay_counter.get_min();
}

float GRClient::get_max_delay() {
	return delay_counter.get_max();
}

float GRClient::get_stream_aspect_ratio() {
	return _prev_stream_aspect_ratio;
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
		call_deferred(NAMEOF(_viewport_size_changed)); // force update screen aspect
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

void GRClient::set_current_auto_connect_address(String _address) {
	Scoped_lock(ts_lock);
	if (current_auto_connect_server_address != _address) {
		break_connection_async();
		current_auto_connect_server_address = _address;
	}
}

String GRClient::get_current_auto_connect_address() {
	Scoped_lock(ts_lock);
	return current_auto_connect_server_address;
}

void GRClient::set_current_auto_connect_port(int _port) {
	Scoped_lock(ts_lock);
	if (current_auto_connect_server_port != _port) {
		break_connection_async();
		current_auto_connect_server_port = _port;
	}
}

int GRClient::get_current_auto_connect_port() {
	Scoped_lock(ts_lock);
	return current_auto_connect_server_port;
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

	call_deferred(NAMEOF(_viewport_size_changed)); // force update screen aspect ratio
}

void GRClient::_load_custom_input_scene(String path, PoolByteArray pck_data, int orig_size, bool is_compressed, int compression_type) {
	ZoneScopedNC("Load Custom Input Scene", tracy::Color::Cornsilk2);

	_remove_custom_input_scene();

	if (path.empty() || pck_data.size() == 0) {
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
		if (is_compressed) {
			err = decompress_bytes(pck_data, orig_size, scene_data, compression_type);
		} else {
			scene_data = pck_data;
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
#if VERSION_MINOR >= 3 || (VERSION_MINOR >= 2 && VERSION_PATCH > 3)
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
				Ref<PackedScene> pck = ResourceLoader::load(path, "", false, &err);
#else
				Ref<PackedScene> pck = ResourceLoader::get_singleton()->load(path, "", false);
				err = pck->can_instance() ? Error::OK : Error::FAILED;
#endif
				if ((int)err) {
					_log("Can't load scene file: " + path + ", code: " + str((int)err), LogLevel::LL_ERROR);
				} else {

					custom_input_scene = pck->instance();
					if (!custom_input_scene) {
						_log("Can't instance scene from PCK file: " + custom_input_scene_tmp_pck_file + ", scene: " + path, LogLevel::LL_ERROR);
					} else {

						control_to_show_in->add_child(custom_input_scene);
						custom_input_scene->connect("tree_exiting", this, NAMEOF(_on_node_deleting), vec_args({ (int)DeletingVarName::CUSTOM_INPUT_SCENE }));

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
		ZoneScopedNC("Remove Custom Input Scene", tracy::Color::Brown1);

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
			send_queue_mutex.lock();
			std::shared_ptr<GRPacketClientStreamOrientation> packet = _find_queued_packet_by_type<GRPacketClientStreamOrientation>(GRPacket::PacketType::ClientStreamOrientation);
			if (packet) {
				packet->set_vertical(is_vertical == ScreenOrientation::VERTICAL);
				send_queue_mutex.unlock();
				goto ratio_sync;
			}
			send_queue_mutex.unlock();

			if (!packet) {
				packet = shared_new(GRPacketClientStreamOrientation);
				packet->set_vertical(is_vertical == ScreenOrientation::VERTICAL);
				send_packet(packet);
			}
		}
	}

ratio_sync:

	if (_viewport_aspect_ratio_syncing) {
		Vector2 size = control_to_show_in->get_size();

		send_queue_mutex.lock();
		std::shared_ptr<GRPacketStreamAspectRatio> packet = _find_queued_packet_by_type<GRPacketStreamAspectRatio>(GRPacket::PacketType::StreamAspectRatio);
		if (packet) {
			packet->set_aspect(size.x / size.y);
			send_queue_mutex.unlock();
			return;
		}
		send_queue_mutex.unlock();

		if (!packet) {
			packet = shared_new(GRPacketStreamAspectRatio);
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
	ZoneScopedNC("Updating stream texture", tracy::Color::Cornsilk1);
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
				tex_shows_stream->set_stretch_mode(stretch_mode == StretchMode::STRETCH_KEEP_ASPECT ? TextureRect::STRETCH_KEEP_ASPECT_CENTERED : TextureRect::STRETCH_SCALE);
				tex_shows_stream->set_material(nullptr);
				tex_shows_stream->set_texture(nullptr);
				break;
			default:
				_log("Wrong stream state!", LogLevel::LL_ERROR);
				break;
		}

		if (signal_connection_state != _stream_state) {
			call_deferred(NAMEOF(emit_signal), "stream_state_changed", _stream_state);
			signal_connection_state = (StreamState)_stream_state;
		}
	}
}

void GRClient::_update_avg_delay(uint64_t delay) {
	delay_counter.update(delay, (int)round(Engine::get_singleton()->get_frames_per_second()));
}

void GRClient::_reset_counters() {
	GRDevice::_reset_counters();
	sync_time_client = 0;
	sync_time_server = 0;
	delay_counter.reset();
}

void GRClient::set_server_setting(ENUM_ARG(TypesOfServerSettings) param, Variant value) {
	send_queue_mutex.lock();
	std::shared_ptr<GRPacketServerSettings> packet = _find_queued_packet_by_type<GRPacketServerSettings>(GRPacket::PacketType::ServerSettings);
	if (packet) {
		packet->add_setting(param, value);
		send_queue_mutex.unlock();
		return;
	}
	send_queue_mutex.unlock();

	if (!packet) {
		packet = shared_new(GRPacketServerSettings);
		packet->add_setting(param, value);
		send_packet(packet);
	}
}

void GRClient::disable_overriding_server_settings() {
	set_server_setting(TypesOfServerSettings::SERVER_SETTINGS_USE_INTERNAL, true);
}

void GRClient::break_connection_async() {
	if (thread_connection) {
		thread_connection->break_connection = true;
	}
}

void GRClient::break_connection() {
	if (thread_connection) {
		thread_connection->break_connection = true;
		while (!thread_connection->connection_finished) {
			sleep_usec(1_ms);
		}
	}
}

//////////////////////////////////////////////
////////////////// THREAD ////////////////////
//////////////////////////////////////////////

void GRClient::_thread_connection(Variant p_userdata) {
	Thread_set_name("Client Connection");

	ConnectionThreadParamsClient *con_thread = VARIANT_OBJ_CAST_TO(p_userdata, ConnectionThreadParamsClient);
	Ref<StreamPeerTCP> con = con_thread->peer;
	Error err = Error::OK;
	bool is_udp_cant_be_started = false;
	String prev_connecting_password = "";

#ifdef AUTO_CONNECTION_ENABLED
	std::shared_ptr<UdpListen> udp_server;
	auto emit_auto_connections_status_changed = [&](bool status) {
		call_deferred(NAMEOF(emit_signal), "auto_connection_listener_status_changed", status);
	};
	auto emit_auto_connection_list_changed = [&]() {
		call_deferred(NAMEOF(emit_signal), "auto_connection_list_changed", get_found_auto_connection_addresses());
	};
	auto close_udp_connection = [&]() {
		udp_server = nullptr;
		emit_auto_connections_status_changed(false);
	};
#endif

	GRDevice::AuthResult prev_auth_error = GRDevice::AuthResult::OK;
	bool first_connection_signal_emitted = false;
	auto emit_first_connection_error = [&]() {
		if (!first_connection_signal_emitted) {
			first_connection_signal_emitted = true;
			call_deferred(NAMEOF(emit_signal), "connection_state_changed", false);
		}
	};

	const String con_error_title = "Connection Error";

	while (!con_thread->stop_thread) {
		FrameMarkNamed("Client Connection Waiting Loop");

		if (get_time_usec() - prev_valid_connection_time > 1000_ms) {
			call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_NO_SIGNAL);
			call_deferred(NAMEOF(_remove_custom_input_scene));
		}

		if (con->get_status() == StreamPeerTCP::STATUS_CONNECTED || con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
			con->disconnect_from_host();
		}

		_send_queue_resize(0);

#ifndef GDNATIVE_LIBRARY
		IP_Address adr;
#else
		String adr;
#endif

		bool auto_mode_ready_to_connect = false;
		switch (connection_type) {
			case GRClient::CONNECTION_WiFi: {
				is_udp_cant_be_started = false;
				if (server_address.is_valid_ip_address()) {
					adr = server_address;
					if (adr.is_valid_ip()) {
					} else {
						_log("Address is invalid: " + server_address, LogLevel::LL_ERROR);
						if (prev_auth_error != GRDevice::AuthResult::Error)
							GRNotifications::add_notification("Resolve Address Error", "Address is invalid: " + server_address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
						prev_auth_error = GRDevice::AuthResult::Error;
					}
				} else {
					adr = IP::get_singleton()->resolve_hostname(adr);
					if (adr.is_valid_ip()) {
						_log("Resolved address for " + server_address + "\n" + adr, LogLevel::LL_DEBUG);
					} else {
						_log("Can't resolve address for " + server_address, LogLevel::LL_ERROR);
						if (prev_auth_error != GRDevice::AuthResult::Error)
							GRNotifications::add_notification("Resolve Address Error", "Can't resolve address: " + server_address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
						prev_auth_error = GRDevice::AuthResult::Error;
					}
				}
				break;
			}
			case GRClient::CONNECTION_ADB: {
				is_udp_cant_be_started = false;
#ifndef GDNATIVE_LIBRARY
				adr = IP_Address("127.0.0.1");
#else
				adr = "127.0.0.1";
#endif
				break;
			}
#ifdef AUTO_CONNECTION_ENABLED
			case GRClient::CONNECTION_AUTO: {
				ZoneScopedNC("Scanning for available servers", tracy::Color::DarkMagenta);

				if (!is_udp_cant_be_started) {
					if (!udp_server) {
						udp_server = shared_new(UdpListen);
						if (!udp_server->Listen(AUTO_CONNECTION_PORT)) {
							udp_server = nullptr;
							is_udp_cant_be_started = true;
							_log("Can't start listening on port " + str(AUTO_CONNECTION_PORT) + " for auto connection mode. Error: " + str((int)err), LogLevel::LL_ERROR);
							is_auto_mode_active = false;
							emit_auto_connections_status_changed(false);
							sleep_usec(250_ms);
							continue;
						}
						emit_auto_connections_status_changed(true);
						is_auto_mode_active = true;
					}
				} else {
					sleep_usec(250_ms);
					continue;
				}

				bool is_error_in_for = false;
				bool is_list_changed = false;
				Ref<StreamPeerBuffer> buf = newref(StreamPeerBuffer);

				// get info from connections
				uint64_t start_time = get_time_usec();
				while (get_time_usec() - start_time < 16_ms) {
					size_t size;
					IpAddress adrs;

					const char *ptr = udp_server->Read(size, adrs, 0);
					if (!ptr) {
						break;
					} else {
						PoolByteArray data;
						data.resize((int)size);
						{
							auto dw = data.write();
							memcpy(dw.ptr(), ptr, size);
						}

						if (data.size() > 4) {
							buf->set_data_array(data);
							buf->seek(4);

							if (GRUtils::validate_packet(data.read().ptr())) {
								Variant var = buf->get_var(false);
								if (var.get_type() == Variant::Type::DICTIONARY) {
									Dictionary dict = var;

									String version;
									String project_name;
									int port = 0;
									PoolStringArray all_serever_addresses;
									uint64_t server_hash = 0;
									PoolByteArray icon_data;

									if (dict.has("version")) {
										PoolByteArray ver = dict["version"];
										if (!GRUtils::validate_version(ver)) {
											continue;
										} else {
											version = str(ver[0]) + "." + str(ver[1]) + "." + str(ver[2]);
										}
									}

									if (dict.has("project_name"))
										project_name = dict["project_name"];
									if (dict.has("port"))
										port = dict["port"];
									if (dict.has("addresses"))
										all_serever_addresses = dict["addresses"];
									if (dict.has("server_hash"))
										server_hash = dict["server_hash"];
									if (dict.has("icon_data"))
										icon_data = dict["icon_data"];

									{
										ts_lock.lock();

										std::shared_ptr<AvailableServerAddress> available_server;
										for (auto a : found_server_addresses) {
											bool has_address = false;
											for (int x = 0; x < a->all_serever_addresses.size(); x++) {
												if (a->all_serever_addresses[x] == adrs.GetText()) {
													has_address = true;
													break;
												}
											}

											if (has_address &&
													a->port == port &&
													a->version == version &&
													a->project_name == project_name) {
												available_server = a;
											}
										}

										bool list_changed = false;
										if (!available_server) {
											available_server = shared_new(AvailableServerAddress);
											available_server->port = port;
											available_server->version = version;
											available_server->project_name = project_name;
											available_server->icon_data = icon_data;

											found_server_addresses.push_back(available_server);
											is_list_changed = true;
										}

										available_server->sender_ip_address = adrs.GetText();
										available_server->all_serever_addresses = all_serever_addresses;
										available_server->server_hash = server_hash;
										available_server->time_added = get_time_usec();

										if (!is_vector_contains(available_server->recieved_from_addresses, adrs.GetText())) {
											available_server->recieved_from_addresses.push_back(adrs.GetText());
											is_list_changed = true;
										}

										ts_lock.unlock();
									}
								}
							}
						} else {
							// just ignore any wrong packet
						}
					}
				}

				if (is_error_in_for) {
					continue;
				}

				// clear outdated servers
				ts_lock.lock();
				for (int i = (int)found_server_addresses.size() - 1; i >= 0; i--) {
					if (found_server_addresses.size() > 0) {
						if (get_time_usec() - found_server_addresses[i]->time_added > 1500_ms) {
							found_server_addresses.erase(found_server_addresses.begin() + i);
							is_list_changed = true;
						}
					} else {
						break;
					}
				}

				for (auto s : found_server_addresses) {
					if (s->sender_ip_address == current_auto_connect_server_address &&
							s->port == current_auto_connect_server_port) {
						auto_mode_ready_to_connect = true;

						adr = s->sender_ip_address;
						port = s->port;
						break;
					}
				}

				if (is_list_changed) {
					emit_auto_connection_list_changed();
				}
				ts_lock.unlock();

				if (!auto_mode_ready_to_connect) {
					sleep_usec(50_ms);
					continue;
				}

				break;
			}
#endif
			default:
				_log("Not supported connection type: " + str(connection_type), LogLevel::LL_ERROR);
				break;
		}

		String address = (String)adr + ":" + str(port);
		err = con->connect_to_host(adr, port);

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

			emit_first_connection_error();

			sleep_usec(250_ms);
			continue;
		}

		{
			ZoneScopedNC("Connecting...", tracy::Color::DeepSkyBlue4);
			while (con->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
				sleep_usec(1_ms);
			}
		}

		if (con->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			_log("Connection timed out with " + address, LogLevel::LL_DEBUG);
			if (prev_auth_error != GRDevice::AuthResult::Timeout) {
				GRNotifications::add_notification(con_error_title, "Connection timed out: " + address, GRNotifications::NotificationIcon::ICON_WARNING, true, 1.f);
				prev_auth_error = GRDevice::AuthResult::Timeout;
			}
			emit_first_connection_error();

			sleep_usec(200_ms);
			continue;
		}

		con->set_no_delay(true);

		bool long_wait = false;

		Ref<PacketPeerStream> ppeer = newref(PacketPeerStream);
		ppeer->set_stream_peer(con);
		ppeer->set_input_buffer_max_size(input_buffer_size_in_mb * 1024 * 1024);

		String notif_error_text;
		GRDevice::AuthResult res = _auth_on_server(ppeer);
		bool force_show_error = false;
		switch (res) {
			case GRDevice::AuthResult::OK: {
				_log("Successful connected to " + address, LogLevel::LL_NORMAL);

				call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_NO_IMAGE);

				con_thread->break_connection = false;
				con_thread->connection_finished = false;
				con_thread->peer = con;
				con_thread->ppeer = ppeer;

				is_connection_working = true;
				first_connection_signal_emitted = false;

				call_deferred(NAMEOF(emit_signal), "connection_state_changed", true);
				call_deferred(NAMEOF(_force_update_stream_viewport_signals)); // force update screen aspect ratio and orientation
				GRNotifications::add_notification("Connected", "Connected to " + address, GRNotifications::NotificationIcon::ICON_SUCCESS, true, 1.f);

#ifdef AUTO_CONNECTION_ENABLED
				close_udp_connection();
#endif

				//////////////////////////////////////////////////////////////////////////
				// Connection Loop Enter point

				_connection_loop(con_thread);

				con_thread->peer.unref();
				con_thread->ppeer.unref();

				is_connection_working = false;
				call_deferred(NAMEOF(emit_signal), "connection_state_changed", false);
				call_deferred(NAMEOF(emit_signal), "mouse_mode_changed", Input::MouseMode::MOUSE_MODE_VISIBLE);
				break;
			}
			case GRDevice::AuthResult::Error:
				notif_error_text = "Can't connect to " + address;
				break;
			case GRDevice::AuthResult::Timeout:
				notif_error_text = "Timeout\n" + address;
				break;
			case GRDevice::AuthResult::RefuseConnection:
				notif_error_text = "Connection refused\n" + address;
				break;
			case GRDevice::AuthResult::VersionMismatch:
				notif_error_text = "Version mismatch\n" + address;
				break;
			case GRDevice::AuthResult::IncorrectPassword:
				notif_error_text = "Incorrect password\n" + address;

				if (prev_connecting_password != password) {
					force_show_error = true;
					prev_connecting_password = password;
				}
				break;
			case GRDevice::AuthResult::PasswordRequired:
				notif_error_text = "Required password but it's not implemented.... " + address;
				break;
			default: {
				notif_error_text = "Unknown error code: " + str((int)res) + "\n" + address;
				if (res != prev_auth_error)
					GRNotifications::add_notification(con_error_title, "Unknown error code: " + str((int)res) + "\n" + address, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
				_log("Unknown error code: " + str((int)res) + ". Disconnecting. " + address, LogLevel::LL_NORMAL);
				break;
			}
		}

		// check only for errors
		switch (res) {
			case GRDevice::AuthResult::OK:
				break;
			case GRDevice::AuthResult::Error:
			case GRDevice::AuthResult::Timeout:
			case GRDevice::AuthResult::RefuseConnection:
			case GRDevice::AuthResult::VersionMismatch:
			case GRDevice::AuthResult::IncorrectPassword:
			case GRDevice::AuthResult::PasswordRequired: {
				long_wait = true;
				emit_first_connection_error();
				if (!notif_error_text.empty()) {
					if (res != prev_auth_error || force_show_error) {
						GRNotifications::add_notification(con_error_title, notif_error_text, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
						_log(notif_error_text, LogLevel::LL_ERROR);
					} else {
						_log(notif_error_text, LogLevel::LL_DEBUG);
					}
				}
				break;
			}
		}
		prev_auth_error = res;

		Ref<StreamPeerTCP> tcp_peer = ppeer->get_stream_peer();
		if (tcp_peer.is_valid()) {
			tcp_peer->disconnect_from_host();
		}
		ppeer->set_output_buffer_max_size(0);
		ppeer->set_input_buffer_max_size(0);

		if (con->is_connected_to_host()) {
			con->disconnect_from_host();
		}

		if (long_wait) {
			sleep_usec(888_ms);
		}
	}

#ifdef AUTO_CONNECTION_ENABLED
	ts_lock.lock();
	found_server_addresses.resize(0);
	emit_auto_connection_list_changed();
	close_udp_connection();
	ts_lock.unlock();
#endif

	call_deferred(NAMEOF(_update_stream_texture_state), StreamState::STREAM_NO_SIGNAL);
	_log("Connection thread stopped", LogLevel::LL_DEBUG);
}

//////////////////////////////////////////////////////////////////////////
// CONNECTION LOOP

void GRClient::_connection_loop(ConnectionThreadParamsClient *con_thread) {
	Ref<StreamPeerTCP> connection = con_thread->peer;
	Ref<PacketPeerStream> ppeer = con_thread->ppeer;

	Error err = Error::OK;
	String address = CONNECTION_ADDRESS(connection);
	String log_error_text = "";

	_reset_counters();
	if (input_collector)
		input_collector->set_process(true);

	uint64_t time64 = get_time_usec();
	uint64_t prev_send_input_time = time64;
	uint64_t prev_ping_sending_time = time64;

	bool ping_sended = false;

	while (!con_thread->break_connection && !con_thread->stop_thread && connection->is_connected_to_host()) {
		ZoneScopedNC("Client Loop", tracy::Color::OrangeRed4);
		Scoped_lock(connection_mutex);

		uint64_t cycle_start_time = get_time_usec();

		bool nothing_happens = true;
		uint64_t start_while_time = 0;
		prev_valid_connection_time = time64;
		int send_data_time_us = (1000000 / send_data_fps);

		///////////////////////////////////////////////////////////////////
		// SENDING
		{
			ZoneScopedNC("Send Data", tracy::Color::IndianRed);
			// INPUT
			time64 = get_time_usec();
			if ((time64 - prev_send_input_time) > send_data_time_us) {
				prev_send_input_time = time64;
				nothing_happens = false;

				if (input_collector) {
					ZoneScopedNC("Send Collected Input", tracy::Color::DarkGoldenrod);
					std::shared_ptr<GRPacketInputData> pack = input_collector->get_collected_input_data();

					if (pack) {
						err = ppeer->put_var(pack->get_data());
						if ((int)err) {
							_log("Put input data failed with code: " + str((int)err), LogLevel::LL_ERROR);
							goto end_send;
						}
					} else {
						_log("Can't get input data from input collector", LogLevel::LL_ERROR);
					}
				}
			}

			// PING
			time64 = get_time_usec();
			if ((time64 - prev_ping_sending_time) > 100_ms && !ping_sended) {
				ZoneScopedNC("Send Ping", tracy::Color::CadetBlue);
				nothing_happens = false;
				ping_sended = true;

				auto pack = shared_new(GRPacketPing);
				err = ppeer->put_var(pack->get_data());
				prev_ping_sending_time = time64;

				if ((int)err) {
					_log("Send ping failed with code: " + str((int)err), LogLevel::LL_ERROR);
					goto end_send;
				}
			}

			// SEND QUEUE
			start_while_time = get_time_usec();
			while (!send_queue.empty() && (get_time_usec() - start_while_time) <= send_data_time_us / 2) {
				ZoneScopedNC("Send Queued Data", tracy::Color::DarkOliveGreen1);
				std::shared_ptr<GRPacket> packet = _send_queue_pop_front();

				if (packet) {
					err = ppeer->put_var(packet->get_data());

					if ((int)err) {
						_log("Put data from queue failed with code: " + str((int)err), LogLevel::LL_ERROR);
						goto end_send;
					}
				}
			}
		}
	end_send:

		if (!connection->is_connected_to_host()) {
			log_error_text = "After sending.";
			continue;
		}

		///////////////////////////////////////////////////////////////////
		// RECEIVING
		{
			ZoneScopedNC("Receive Data", tracy::Color::IndianRed1);

			// Get some packets
			start_while_time = get_time_usec();
			while (ppeer->get_available_packet_count() > 0 && (get_time_usec() - start_while_time) <= send_data_time_us / 2) {
				ZoneScopedNC("Get Available Packet", tracy::Color::IndianRed4);
				nothing_happens = false;

#ifndef GDNATIVE_LIBRARY
				Variant buf;
				err = ppeer->get_var(buf);
#else
				Variant buf = ppeer->get_var();
#endif

				if ((int)err)
					goto end_recv;

				std::shared_ptr<GRPacket> pack = GRPacket::create(buf);
				if (!pack) {
					_log("Incorrect GRPacket", LogLevel::LL_ERROR);
					continue;
				}

				GRPacket::PacketType type = pack->get_type();

				switch (type) {
					// Stream Data
					case GRPacket::PacketType::StreamDataImage:
					case GRPacket::PacketType::StreamDataH264: {
						shared_cast_def(GRPacketStreamData, data, pack);
						if (!data) {
							_log("Incorrect GRPacketStreamData", LogLevel::LL_ERROR);
							continue;
						}

						_push_pack_to_decoder(data);
						break;
					}
					// Other
					case GRPacket::PacketType::SyncTime: {
						shared_cast_def(GRPacketSyncTime, data, pack);
						if (!data) {
							_log("Incorrect GRPacketSyncTime", LogLevel::LL_ERROR);
							continue;
						}

						sync_time_client = get_time_usec();
						sync_time_server = data->get_time();
						sync_time_delta = sync_time_client - sync_time_server;

						break;
					}
					case GRPacket::PacketType::ServerSettings: {
						if (!_server_settings_syncing) {
							continue;
						}

						shared_cast_def(GRPacketServerSettings, data, pack);
						if (!data) {
							_log("Incorrect GRPacketServerSettings", LogLevel::LL_ERROR);
							continue;
						}

						call_deferred(NAMEOF(emit_signal), "server_settings_received", map_to_dict(data->get_settings()));
						break;
					}
					case GRPacket::PacketType::MouseModeSync: {
						shared_cast_def(GRPacketMouseModeSync, data, pack);
						if (!data) {
							_log("Incorrect GRPacketMouseModeSync", LogLevel::LL_ERROR);
							continue;
						}

						call_deferred(NAMEOF(emit_signal), "mouse_mode_changed", data->get_mouse_mode());
						break;
					}
					case GRPacket::PacketType::CustomInputScene: {
						shared_cast_def(GRPacketCustomInputScene, data, pack);
						if (!data) {
							_log("Incorrect GRPacketCustomInputScene", LogLevel::LL_ERROR);
							continue;
						}

						call_deferred(NAMEOF(_load_custom_input_scene), data->get_scene_path(), data->get_scene_data(), data->get_original_size(), data->is_compressed(), data->get_compression_type());
						break;
					}
					case GRPacket::PacketType::StreamAspectRatio: {
						shared_cast_def(GRPacketStreamAspectRatio, data, pack);
						if (!data) {
							_log("Incorrect GRPacketStreamAspectRatio", LogLevel::LL_ERROR);
							continue;
						}
						_prev_stream_aspect_ratio = data->get_aspect();
						call_deferred(NAMEOF(emit_signal), "stream_aspect_ratio_changed", data->get_aspect());
						break;
					}
					case GRPacket::PacketType::ServerStreamQualityHint: {
						shared_cast_def(GRPacketServerStreamQualityHint, data, pack);
						if (!data) {
							_log("Incorrect GRPacketServerStreamQualityHint", LogLevel::LL_ERROR);
							continue;
						}
						call_deferred(NAMEOF(emit_signal), "server_quality_hint_setting_received", data->get_hint());
						break;
					}
					case GRPacket::PacketType::CustomUserData: {
						shared_cast_def(GRPacketCustomUserData, data, pack);
						if (!data) {
							_log("Incorrect GRPacketCustomUserData", LogLevel::LL_ERROR);
							continue;
						}
						call_deferred(NAMEOF(emit_signal), "user_data_received", data->get_packet_id(), data->get_user_data());
						break;
					}
					case GRPacket::PacketType::Ping: {
						auto pack = shared_new(GRPacketPong);
						err = ppeer->put_var(pack->get_data());
						if ((int)err) {
							_log("Send pong failed with code: " + str((int)err), LogLevel::LL_NORMAL);
							continue;
						}
						break;
					}
					case GRPacket::PacketType::Pong: {
						_update_avg_ping(get_time_usec() - prev_ping_sending_time);
						ping_sended = false;
						break;
					}
					default:
						_log("Not supported packet type! " + str((int)type), LogLevel::LL_WARNING);
						break;
				}
			}
		}
	end_recv:

		if (!connection->is_connected_to_host()) {
			log_error_text = "After receiving.";
			continue;
		}

		if (nothing_happens)
			sleep_usec(1_ms);
	}

	if (input_collector)
		input_collector->set_process(false);

	_prev_stream_aspect_ratio = 0;
	_stop_decoder();
	_send_queue_resize(0);

	if (connection->is_connected_to_host()) {
		_log("Closing connection to " + address, LogLevel::LL_NORMAL);
		GRNotifications::add_notification("Disconnected", "Closing connection to " + address, GRNotifications::NotificationIcon::ICON_FAIL, true, 1.f);
	} else {
		_log("Lost connection to " + address + ". " + log_error_text, LogLevel::LL_ERROR);
		GRNotifications::add_notification("Disconnected", "Lost connection to " + address, GRNotifications::NotificationIcon::ICON_FAIL, true, 1.f);
	}

	con_thread->break_connection = true;
	con_thread->connection_finished = true;
}

GRDevice::AuthResult GRClient::_auth_on_server(Ref<PacketPeerStream> &ppeer) {
#define wait_packet(_n)                                                                            \
	{                                                                                              \
		ZoneScopedNC("Wait Auth Packet: " #_n, tracy::Color::DarkCyan);                            \
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
		}                                                                                          \
	}
#define packet_error_check(_t)              \
	if ((int)err) {                         \
		_log(_t, LogLevel::LL_DEBUG);       \
		return GRDevice::AuthResult::Error; \
	}
	ZoneScopedNC("Authorization", tracy::Color::Cyan3);

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
		data["id"] = device_id;
		data["version"] = get_gr_version();
		data["password"] = password;

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
			return (GRDevice::AuthResult)(int)ret;
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
				if (tex.is_valid()) {
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
				} else {
					// TODO check if it can be simplified
					Vector2 rect_size = texture_rect->get_size();
					float _ratio = dev->get_stream_aspect_ratio();
					if (_ratio == 0) {
						goto fill;
					}

					int n_w = (int)rect_size.x,
						n_h = (int)(rect_size.y / _ratio);
					if (_ratio >= (rect_size.x / rect_size.y)) {
						n_w = (int)rect_size.x;
						n_h = (int)(n_w / _ratio);

						n_w = n_w - n_w % 4;
						n_h = n_h - n_h % 4;
						stream_rect = Rect2(Vector2(0, (rect_size.y - n_h) / 2), Vector2(float(n_w), float(n_h)));
						return;
					} else {
						n_h = (int)rect_size.y;
						n_w = (int)(n_h * _ratio);
						float a2 = float(n_w) / n_h;
						if (n_w > rect_size.x) {
							n_w = (int)rect_size.x;
							n_h = (int)(n_w * a2);
						}

						n_w = n_w - n_w % 4;
						n_h = n_h - n_h % 4;
						stream_rect = Rect2(Vector2((rect_size.x - n_w) / 2, 0), Vector2(float(n_w), float(n_h)));
						return;
					}
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
	std::shared_ptr<GRInputDataEvent> data = GRInputDataEvent::parse_event(ie, stream_rect);
	if (data) {
		Scoped_lock(ts_lock);
		collected_input_data.push_back(data);
	}
}

void GRInputCollector::_release_pointers() {
	{
		auto buttons = mouse_buttons.keys();
		for (int i = 0; i < buttons.size(); i++) {
			if (mouse_buttons[buttons[i]]) {
				Ref<InputEventMouseButton> iemb = newref(InputEventMouseButton);
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
				Ref<InputEventScreenTouch> iest = newref(InputEventScreenTouch);
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

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_on_focus"), NAMEOF(set_capture_on_focus), NAMEOF(is_capture_on_focus));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_when_hover"), NAMEOF(set_capture_when_hover), NAMEOF(is_capture_when_hover));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "capture_pointer"), NAMEOF(set_capture_pointer), NAMEOF(is_capture_pointer));
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
	ZoneScopedNC("Input Collector Process", tracy::Color::Goldenrod);

	if (!parent || (capture_only_when_control_in_focus && !parent->has_focus()) ||
			(dev && dev->get_status() != GRDevice::WorkingStatus::STATUS_WORKING) ||
			!dev->is_stream_active() || !is_inside_tree()) {
		return;
	}

	{
		Scoped_lock(ts_lock);
		if (collected_input_data.size() >= 512) {
			collected_input_data.resize(0);
		}
	}

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
			ZoneScopedNC("Input Collector Sensors", tracy::Color::Goldenrod2);
			Scoped_lock(ts_lock);
			auto w = sensors.write();
			w[0] = Input::get_singleton()->get_accelerometer();
			w[1] = Input::get_singleton()->get_gravity();
			w[2] = Input::get_singleton()->get_gyroscope();
			w[3] = Input::get_singleton()->get_magnetometer();
			release_pva_write(w);
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

std::shared_ptr<GRPacketInputData> GRInputCollector::get_collected_input_data() {
	ZoneScopedNC("Get Input Collector Data", tracy::Color::Goldenrod3);
	auto res = shared_new(GRPacketInputData);
	auto s = shared_new(GRInputDeviceSensorsData);

	Scoped_lock(ts_lock);

	s->set_sensors(sensors);
	collected_input_data.push_back(s);
	res->set_input_data(collected_input_data);
	collected_input_data.resize(0);
	return res;
}

void GRInputCollector::_init() {
	LEAVE_IF_EDITOR();
	Scoped_lock(ts_lock);

	parent = nullptr;
	set_process_input(true);
	sensors.resize(4);
}

void GRInputCollector::_deinit() {
	LEAVE_IF_EDITOR();
	Scoped_lock(ts_lock);

	sensors.resize(0);
	collected_input_data.resize(0);
	if (this_in_client)
		*this_in_client = nullptr;
	mouse_buttons.clear();
	screen_touches.clear();
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

#endif // NO_GODOTREMOTE_CLIENT

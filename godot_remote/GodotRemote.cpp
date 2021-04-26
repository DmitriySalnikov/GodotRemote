/* GodotRemote.cpp */
#include "GodotRemote.h"
#include "GRClient.h"
#include "GRServer.h"

#ifndef GDNATIVE_LIBRARY
#include "editor/editor_node.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#else
#include <GlobalConstants.hpp>
#include <ProjectSettings.hpp>
#include <SceneTree.hpp>
using namespace godot;
#endif

#if defined(GODOT_REMOTE_TRACY_ENABLED) && defined(TRACY_ENABLE)
#include "GRProfilerViewportMiniPreview.h"
#endif

using namespace GRUtils;

GodotRemote *GodotRemote::singleton = nullptr;
bool GodotRemote::is_init_completed = false;

GR_PS_NAME_TYPE GodotRemote::ps_general_autoload_name = "debug/godot_remote/general/autostart";
GR_PS_NAME_TYPE GodotRemote::ps_general_use_static_port_name = "debug/godot_remote/general/use_static_port";
GR_PS_NAME_TYPE GodotRemote::ps_general_port_name = "debug/godot_remote/general/port";
GR_PS_NAME_TYPE GodotRemote::ps_general_auto_connection_port_name = "debug/godot_remote/general/auto_connection_port";
GR_PS_NAME_TYPE GodotRemote::ps_general_loglevel_name = "debug/godot_remote/general/log_level";

GR_PS_NAME_TYPE GodotRemote::ps_notifications_enabled_name = "debug/godot_remote/notifications/notifications_enabled";
GR_PS_NAME_TYPE GodotRemote::ps_noticications_position_name = "debug/godot_remote/notifications/notifications_position";
GR_PS_NAME_TYPE GodotRemote::ps_notifications_duration_name = "debug/godot_remote/notifications/notifications_duration";

GR_PS_NAME_TYPE GodotRemote::ps_server_image_encoder_threads_count_name = "debug/godot_remote/server/image_encoder_threads_count";
GR_PS_NAME_TYPE GodotRemote::ps_server_config_adb_name = "debug/godot_remote/server/configure_adb_on_play";
GR_PS_NAME_TYPE GodotRemote::ps_server_stream_skip_frames_name = "debug/godot_remote/server/skip_frames";
GR_PS_NAME_TYPE GodotRemote::ps_server_stream_enabled_name = "debug/godot_remote/server/video_stream_enabled";
GR_PS_NAME_TYPE GodotRemote::ps_server_compression_type_name = "debug/godot_remote/server/compression_type";
GR_PS_NAME_TYPE GodotRemote::ps_server_stream_quality_name = "debug/godot_remote/server/stream_quality";
GR_PS_NAME_TYPE GodotRemote::ps_server_jpg_buffer_mb_size_name = "debug/godot_remote/server/jpg_compress_buffer_size_mbytes";
GR_PS_NAME_TYPE GodotRemote::ps_server_auto_adjust_scale_name = "debug/godot_remote/server/auto_adjust_scale";
GR_PS_NAME_TYPE GodotRemote::ps_server_scale_of_sending_stream_name = "debug/godot_remote/server/scale_of_sending_stream";
GR_PS_NAME_TYPE GodotRemote::ps_server_password_name = "debug/godot_remote/server/password";
GR_PS_NAME_TYPE GodotRemote::ps_server_target_fps_name = "debug/godot_remote/server/target_fps";

GR_PS_NAME_TYPE GodotRemote::ps_server_custom_input_scene_name = "debug/godot_remote/server_custom_input_scene/custom_input_scene";
GR_PS_NAME_TYPE GodotRemote::ps_server_custom_input_scene_compressed_name = "debug/godot_remote/server_custom_input_scene/send_custom_input_scene_compressed";
GR_PS_NAME_TYPE GodotRemote::ps_server_custom_input_scene_compression_type_name = "debug/godot_remote/server_custom_input_scene/custom_input_scene_compression_type";

GodotRemote *GodotRemote::get_singleton() {
	return singleton;
}

void GodotRemote::_init() {
	START_TRACY

	if (!singleton)
		singleton = this;

	register_and_load_settings();
	GRUtils::init();

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
	call_deferred(NAMEOF(_prepare_editor));
#endif
#endif

	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
	godot_remote_root_node = memnew(Node);
	godot_remote_root_node->set_name("GodotRemoteRoot");
#else
	godot_remote_root_node = this;
#endif

	call_deferred(NAMEOF(_create_autoload_nodes));
	if (is_autostart) {
		call_deferred(NAMEOF(create_and_start_device), DeviceType::DEVICE_AUTO);
	}

	is_init_completed = true;
}

void GodotRemote::_deinit() {
#if !defined(GDNATIVE_LIBRARY) && defined(TOOLS_ENABLED)
	Thread_close(adb_config_thread);
#endif

	LEAVE_IF_EDITOR();
	remove_remote_device();

	if (singleton == this) {
		singleton = nullptr;
	}
	GRUtils::deinit();

	STOP_TRACY
}

void GodotRemote::GodotRemote::print_str(String txt) {
#ifndef GDNATIVE_LIBRARY
	OS::get_singleton()->print(txt.ascii().get_data());
	OS::get_singleton()->print("\n");

#else
	Godot::print(txt);
#endif
}

void GodotRemote::GodotRemote::print_error_str(String txt, String func, String file, int line) {
#ifndef GDNATIVE_LIBRARY
	OS::get_singleton()->print_error(func.ascii().get_data(), file.ascii().get_data(), line, 0, txt.ascii().get_data(), Logger::ErrorType::ERR_ERROR);
#else
	if (file != "") {
		int idx = file.find("godot_remote");
		if (idx != -1)
			file = file.substr(file.find("godot_remote"), file.length());
	}
	Godot::print_error(txt, func, file, line);
#endif
}

void GodotRemote::GodotRemote::print_warning_str(String txt, String func, String file, int line) {
#ifndef GDNATIVE_LIBRARY
	OS::get_singleton()->print_error(func.ascii().get_data(), file.ascii().get_data(), line, 0, txt.ascii().get_data(), Logger::ErrorType::ERR_WARNING);
#else
	if (file != "") {
		int idx = file.find("godot_remote");
		if (idx != -1)
			file = file.substr(file.find("godot_remote"), file.length());
	}
	Godot::print_warning(txt, func, file, line);
#endif
}

#ifndef GDNATIVE_LIBRARY

void GodotRemote::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_create_autoload_nodes)), &GodotRemote::_create_autoload_nodes);
#if defined(TOOLS_ENABLED)
	ClassDB::bind_method(D_METHOD(NAMEOF(_adb_port_forwarding)), &GodotRemote::_adb_port_forwarding);
	ClassDB::bind_method(D_METHOD(NAMEOF(_run_emitted)), &GodotRemote::_run_emitted);
	ClassDB::bind_method(D_METHOD(NAMEOF(_prepare_editor)), &GodotRemote::_prepare_editor);
	ClassDB::bind_method(D_METHOD(NAMEOF(_adb_config_thread), "user_data"), &GodotRemote::_adb_config_thread);
#endif

	ClassDB::bind_method(D_METHOD(NAMEOF(create_and_start_device), "device_type"), &GodotRemote::create_and_start_device, DEFVAL(DeviceType::DEVICE_AUTO));
	ClassDB::bind_method(D_METHOD(NAMEOF(create_remote_device), "device_type"), &GodotRemote::create_remote_device, DEFVAL(DeviceType::DEVICE_AUTO));
	ClassDB::bind_method(D_METHOD(NAMEOF(start_remote_device)), &GodotRemote::start_remote_device);
	ClassDB::bind_method(D_METHOD(NAMEOF(remove_remote_device)), &GodotRemote::remove_remote_device);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_device)), &GodotRemote::get_device);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_version)), &GodotRemote::get_version);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_gdnative)), &GodotRemote::is_gdnative);

	// GRNotifications
	ClassDB::bind_method(D_METHOD(NAMEOF(notifications_connect), "signal", "inst", "method", "binds", "flags"), &GodotRemote::notifications_connect);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_notification), "title"), &GodotRemote::get_notification);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_all_notifications)), &GodotRemote::get_all_notifications);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_notifications_with_title), "title"), &GodotRemote::get_notifications_with_title);

	ClassDB::bind_method(D_METHOD(NAMEOF(add_notification_or_append_string), "title", "text", "icon", "add_to_new_line", "duration_multiplier"), &GodotRemote::add_notification_or_append_string, DEFVAL(true), DEFVAL(1.f));
	ClassDB::bind_method(D_METHOD(NAMEOF(add_notification_or_update_line), "title", "id", "text", "icon", "duration_multiplier"), &GodotRemote::add_notification_or_update_line, DEFVAL(1.f));
	ClassDB::bind_method(D_METHOD(NAMEOF(add_notification), "title", "text", "notification_icon", "update_existing", "duration_multiplier"), &GodotRemote::add_notification, DEFVAL((int)GRNotifications::NotificationIcon::ICON_NONE), DEFVAL(true), DEFVAL(1.f));
	ClassDB::bind_method(D_METHOD(NAMEOF(remove_notification), "title", "is_all_entries"), &GodotRemote::remove_notification, DEFVAL(true));
	ClassDB::bind_method(D_METHOD(NAMEOF(remove_notification_exact), "notification"), &GodotRemote::remove_notification_exact);
	ClassDB::bind_method(D_METHOD(NAMEOF(clear_notifications)), &GodotRemote::clear_notifications);

	ClassDB::bind_method(D_METHOD(NAMEOF(set_notifications_layer), "position"), &GodotRemote::set_notifications_layer);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_notifications_position), "position"), &GodotRemote::set_notifications_position);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_notifications_enabled), "enabled"), &GodotRemote::set_notifications_enabled);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_notifications_duration), "duration"), &GodotRemote::set_notifications_duration);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_notifications_style), "style"), &GodotRemote::set_notifications_style);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_notifications_layer)), &GodotRemote::get_notifications_layer);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_notifications_position)), &GodotRemote::get_notifications_position);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_notifications_enabled)), &GodotRemote::get_notifications_enabled);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_notifications_duration)), &GodotRemote::get_notifications_duration);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_notifications_style)), &GodotRemote::get_notifications_style);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "notifications_layer"), NAMEOF(set_notifications_layer), NAMEOF(get_notifications_layer));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "notifications_position"), NAMEOF(set_notifications_position), NAMEOF(get_notifications_position));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "notifications_enabled"), NAMEOF(set_notifications_enabled), NAMEOF(get_notifications_enabled));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "notifications_duration"), NAMEOF(set_notifications_duration), NAMEOF(get_notifications_duration));
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "notifications_style"), NAMEOF(set_notifications_style), NAMEOF(get_notifications_style));

	ADD_SIGNAL(MethodInfo("device_added"));
	ADD_SIGNAL(MethodInfo("device_removed"));

	BIND_ENUM_CONSTANT(DEVICE_AUTO);
	BIND_ENUM_CONSTANT(DEVICE_SERVER);
	BIND_ENUM_CONSTANT(DEVICE_CLIENT);

	// GRUtils

	BIND_ENUM_CONSTANT(LL_NONE);
	BIND_ENUM_CONSTANT(LL_DEBUG);
	BIND_ENUM_CONSTANT(LL_NORMAL);
	BIND_ENUM_CONSTANT(LL_WARNING);
	BIND_ENUM_CONSTANT(LL_ERROR);

	ClassDB::bind_method(D_METHOD(NAMEOF(set_log_level), "level"), &GodotRemote::set_log_level);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_2d_safe_area), "canvas_item"), &GodotRemote::get_2d_safe_area);

	ClassDB::bind_method(D_METHOD(NAMEOF(set_gravity), "value"), &GodotRemote::set_gravity);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_accelerometer), "value"), &GodotRemote::set_accelerometer);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_magnetometer), "value"), &GodotRemote::set_magnetometer);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_gyroscope), "value"), &GodotRemote::set_gyroscope);

	ClassDB::bind_method(D_METHOD(NAMEOF(print_str), "txt"), &GodotRemote::print_str);
	ClassDB::bind_method(D_METHOD(NAMEOF(print_error_str), "txt", "func", "file", "line"), &GodotRemote::print_error_str);
	ClassDB::bind_method(D_METHOD(NAMEOF(print_warning_str), "txt", "func", "file", "line"), &GodotRemote::print_warning_str);
	// Other Enums
}

#else

void GodotRemote::_register_methods() {
	METHOD_REG(GodotRemote, _notification);

	METHOD_REG(GodotRemote, _create_autoload_nodes);

	METHOD_REG(GodotRemote, create_and_start_device);
	METHOD_REG(GodotRemote, create_remote_device);
	METHOD_REG(GodotRemote, start_remote_device);
	METHOD_REG(GodotRemote, remove_remote_device);

	METHOD_REG(GodotRemote, get_device);
	METHOD_REG(GodotRemote, get_version);
	METHOD_REG(GodotRemote, is_gdnative);

	// GRNotifications
	METHOD_REG(GodotRemote, notifications_connect);

	METHOD_REG(GodotRemote, get_notification);
	METHOD_REG(GodotRemote, get_all_notifications);
	METHOD_REG(GodotRemote, get_notifications_with_title);

	METHOD_REG(GodotRemote, add_notification_or_append_string);
	METHOD_REG(GodotRemote, add_notification_or_update_line);
	METHOD_REG(GodotRemote, add_notification);
	METHOD_REG(GodotRemote, remove_notification);
	METHOD_REG(GodotRemote, remove_notification_exact);
	METHOD_REG(GodotRemote, clear_notifications);

	METHOD_REG(GodotRemote, set_notifications_layer);
	METHOD_REG(GodotRemote, set_notifications_position);
	METHOD_REG(GodotRemote, set_notifications_enabled);
	METHOD_REG(GodotRemote, set_notifications_duration);
	METHOD_REG(GodotRemote, set_notifications_style);

	METHOD_REG(GodotRemote, get_notifications_layer);
	METHOD_REG(GodotRemote, get_notifications_position);
	METHOD_REG(GodotRemote, get_notifications_enabled);
	METHOD_REG(GodotRemote, get_notifications_duration);
	METHOD_REG(GodotRemote, get_notifications_style);

	register_property<GodotRemote, int>("notifications_layer", &GodotRemote::set_notifications_layer, &GodotRemote::get_notifications_layer, 128);
	register_property<GodotRemote, int>("notifications_position", &GodotRemote::set_notifications_position, &GodotRemote::get_notifications_position, GRNotifications::NotificationsPosition::TOP_CENTER);
	register_property<GodotRemote, bool>("notifications_enabled", &GodotRemote::set_notifications_enabled, &GodotRemote::get_notifications_enabled, true);
	register_property<GodotRemote, float>("notifications_duration", &GodotRemote::set_notifications_duration, &GodotRemote::get_notifications_duration, 3.f);
	register_property<GodotRemote, Ref<GRNotificationStyle> >("notifications_style", &GodotRemote::set_notifications_style, &GodotRemote::get_notifications_style, nullptr);

	register_signal<GodotRemote>("device_added", Dictionary::make());
	register_signal<GodotRemote>("device_removed", Dictionary::make());

	METHOD_REG(GodotRemote, set_log_level);
	METHOD_REG(GodotRemote, get_2d_safe_area);

	METHOD_REG(GodotRemote, set_gravity);
	METHOD_REG(GodotRemote, set_accelerometer);
	METHOD_REG(GodotRemote, set_magnetometer);
	METHOD_REG(GodotRemote, set_gyroscope);

	METHOD_REG(GodotRemote, print_str);
	METHOD_REG(GodotRemote, print_error_str);
	METHOD_REG(GodotRemote, print_warning_str);

	CONST_FAKE_REG(GodotRemote);

	// GodotRemote
	CONST_REG(GodotRemote, DeviceType, DEVICE_AUTO);
	CONST_REG(GodotRemote, DeviceType, DEVICE_SERVER);
	CONST_REG(GodotRemote, DeviceType, DEVICE_CLIENT);

	CONST_REG(GodotRemote, LogLevel, LL_NONE);
	CONST_REG(GodotRemote, LogLevel, LL_DEBUG);
	CONST_REG(GodotRemote, LogLevel, LL_NORMAL);
	CONST_REG(GodotRemote, LogLevel, LL_WARNING);
	CONST_REG(GodotRemote, LogLevel, LL_ERROR);

	// GRNotifications
	CONST_REG(GRNotifications, NotificationIcon, ICON_NONE);
	CONST_REG(GRNotifications, NotificationIcon, ICON_ERROR);
	CONST_REG(GRNotifications, NotificationIcon, ICON_WARNING);
	CONST_REG(GRNotifications, NotificationIcon, ICON_SUCCESS);
	CONST_REG(GRNotifications, NotificationIcon, ICON_FAIL);

	CONST_REG(GRNotifications, NotificationsPosition, TOP_LEFT);
	CONST_REG(GRNotifications, NotificationsPosition, TOP_CENTER);
	CONST_REG(GRNotifications, NotificationsPosition, TOP_RIGHT);
	CONST_REG(GRNotifications, NotificationsPosition, BOTTOM_LEFT);
	CONST_REG(GRNotifications, NotificationsPosition, BOTTOM_CENTER);
	CONST_REG(GRNotifications, NotificationsPosition, BOTTOM_RIGHT);

	// GRDevice
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_USE_INTERNAL);
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_VIDEO_STREAM_ENABLED);
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_COMPRESSION_TYPE);
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_STREAM_QUALITY);
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_SKIP_FRAMES);
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_RENDER_SCALE);
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_TARGET_FPS);
	CONST_REG(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_THREADS_NUMBER);

	CONST_REG(GRDevice, ImageCompressionType, COMPRESSION_JPG);
	CONST_REG(GRDevice, ImageCompressionType, COMPRESSION_H264);

	CONST_REG(GRDevice, WorkingStatus, STATUS_STARTING);
	CONST_REG(GRDevice, WorkingStatus, STATUS_STOPPING);
	CONST_REG(GRDevice, WorkingStatus, STATUS_WORKING);
	CONST_REG(GRDevice, WorkingStatus, STATUS_STOPPED);

#ifndef NO_GODOTREMOTE_CLIENT
	// GRClient
	CONST_REG(GRClient, ConnectionType, CONNECTION_ADB);
	CONST_REG(GRClient, ConnectionType, CONNECTION_WiFi);
	CONST_REG(GRClient, ConnectionType, CONNECTION_AUTO);

	CONST_REG(GRClient, StretchMode, STRETCH_KEEP_ASPECT);
	CONST_REG(GRClient, StretchMode, STRETCH_FILL);

	CONST_REG(GRClient, StreamState, STREAM_NO_SIGNAL);
	CONST_REG(GRClient, StreamState, STREAM_ACTIVE);
	CONST_REG(GRClient, StreamState, STREAM_NO_IMAGE);
#endif // NO_GODOTREMOTE_CLIENT
}
#endif

void GodotRemote::_notification(int p_notification) {
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

Node *GodotRemote::GodotRemote::get_root_node() {
	return godot_remote_root_node;
}

GRDevice *GodotRemote::get_device() {
	return device;
}

String GodotRemote::get_version() {
	PoolByteArray ver = get_gr_version();
	return str_arr(ver, true, 0, ".", false);
}

bool GodotRemote::is_gdnative() {
#ifndef GDNATIVE_LIBRARY
	return false;
#else
	return true;
#endif
}

bool GodotRemote::create_remote_device(ENUM_ARG(DeviceType) type) {
	if (!godot_remote_root_node) return false;
	remove_remote_device();

	GRDevice *d = nullptr;

	switch (type) {
			// automatically start server if it not a standalone build
		case DeviceType::DEVICE_AUTO:
			if (!OS::get_singleton()->has_feature("standalone")) {
#ifndef NO_GODOTREMOTE_SERVER
				d = memnew(GRServer);
#else
				ERR_FAIL_V_MSG(false, "Server not included in this build!");
#endif
			}
			break;
		case DeviceType::DEVICE_SERVER:
#ifndef NO_GODOTREMOTE_SERVER
			d = memnew(GRServer);
#else
			ERR_FAIL_V_MSG(false, "Server not included in this build!");
#endif
			break;
		case DeviceType::DEVICE_CLIENT:
#ifndef NO_GODOTREMOTE_CLIENT
			d = memnew(GRClient);
#else
			ERR_FAIL_V_MSG(false, "Client not included in this build!");
#endif
			break;
		default:
			ERR_FAIL_V_MSG(false, "Not allowed type!");
			break;
	}

	if (d) {
		device = d;
		call_deferred(NAMEOF(emit_signal), "device_added");
		godot_remote_root_node->call_deferred("add_child", device);
		godot_remote_root_node->call_deferred("move_child", device, 0);
		return true;
	}

	return false;
}

bool GodotRemote::start_remote_device() {
	if (device && !device->is_queued_for_deletion()) {
		device->start();
		return true;
	}
	return false;
}

bool GodotRemote::remove_remote_device() {
	if (device && !device->is_queued_for_deletion()) {
#ifdef MANUAL_TRACY
		if (device->get_status() == (int)GRDevice::WorkingStatus::STATUS_WORKING) {
			device->call(NAMEOF(_internal_call_only_deffered_stop));
		}
#else
		device->stop();
#endif
		//memdelete(device);
		device = nullptr;
		call_deferred(NAMEOF(emit_signal), "device_removed");
		return true;
	}
	return false;
}

void GodotRemote::register_and_load_settings() {
#ifndef GDNATIVE_LIBRARY
#define DEF_SET(var, name, def_val, info_type, info_hint_type, info_hint_string) \
	var = GLOBAL_DEF(name, def_val);                                             \
	ProjectSettings::get_singleton()->set_custom_property_info(name, PropertyInfo(info_type, name, info_hint_type, info_hint_string))
#define DEF_SET_ENUM(var, type, name, def_val, info_type, info_hint_type, info_hint_string) \
	var = (type)(int)GLOBAL_DEF(name, def_val);                                             \
	ProjectSettings::get_singleton()->set_custom_property_info(name, PropertyInfo(info_type, name, info_hint_type, info_hint_string))
#define DEF_(name, def_val, info_type, info_hint_type, info_hint_string) \
	GLOBAL_DEF(name, def_val);                                           \
	ProjectSettings::get_singleton()->set_custom_property_info(name, PropertyInfo(info_type, name, info_hint_type, info_hint_string))

#else

#define DEF_SET(var, name, def_val, info_type, info_hint_type, info_hint_string) \
	{                                                                            \
		Dictionary d;                                                            \
		d["name"] = name;                                                        \
		d["type"] = info_type;                                                   \
		d["hint"] = GlobalConstants::info_hint_type;                             \
		d["hint_string"] = info_hint_string;                                     \
		var = GLOBAL_DEF(name, def_val);                                         \
		ProjectSettings::get_singleton()->add_property_info(d);                  \
	}
#define DEF_SET_ENUM(var, type, name, def_val, info_type, info_hint_type, info_hint_string) \
	{                                                                                       \
		Dictionary d;                                                                       \
		d["name"] = name;                                                                   \
		d["type"] = info_type;                                                              \
		d["hint"] = GlobalConstants::info_hint_type;                                        \
		d["hint_string"] = info_hint_string;                                                \
		var = (type)(int)GLOBAL_DEF(name, def_val);                                         \
		ProjectSettings::get_singleton()->add_property_info(d);                             \
	}
#define DEF_(name, def_val, info_type, info_hint_type, info_hint_string) \
	{                                                                    \
		Dictionary d;                                                    \
		d["name"] = name;                                                \
		d["type"] = info_type;                                           \
		d["hint"] = GlobalConstants::info_hint_type;                     \
		d["hint_string"] = info_hint_string;                             \
		GLOBAL_DEF(name, def_val);                                       \
		ProjectSettings::get_singleton()->add_property_info(d);          \
	}

#endif

	DEF_SET(is_autostart, ps_general_autoload_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_general_use_static_port_name, false, Variant::BOOL, PROPERTY_HINT_RANGE, "");
	DEF_(ps_general_port_name, PORT_STATIC_CONNECTION, Variant::INT, PROPERTY_HINT_RANGE, "0,65535");
	DEF_(ps_general_auto_connection_port_name, PORT_AUTO_CONNECTION, Variant::INT, PROPERTY_HINT_RANGE, "0,65535");
	DEF_(ps_general_loglevel_name, LogLevel::LL_NORMAL, Variant::INT, PROPERTY_HINT_ENUM, "Debug,Normal,Warning,Error,None");

	DEF_(ps_notifications_enabled_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_noticications_position_name, GRNotifications::NotificationsPosition::TOP_CENTER, Variant::INT, PROPERTY_HINT_ENUM, "TopLeft,TopCenter,TopRight,BottomLeft,BottomCenter,BottomRight");
	DEF_(ps_notifications_duration_name, 3.f, Variant::REAL, PROPERTY_HINT_RANGE, "0,100, 0.01");

	// const server settings
	DEF_(ps_server_image_encoder_threads_count_name, 2, Variant::INT, PROPERTY_HINT_RANGE, "1,16");
	DEF_(ps_server_config_adb_name, false, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_server_custom_input_scene_name, "", Variant::STRING, PROPERTY_HINT_FILE, "*.tscn,*.scn");
#ifndef GDNATIVE_LIBRARY
	DEF_(ps_server_custom_input_scene_compressed_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_server_custom_input_scene_compression_type_name, 0, Variant::INT, PROPERTY_HINT_ENUM, "FastLZ,DEFLATE,zstd,gzip");
#endif
	DEF_(ps_server_jpg_buffer_mb_size_name, 8, Variant::INT, PROPERTY_HINT_RANGE, "1,128");

	// only server can change this settings
	DEF_(ps_server_password_name, "", Variant::STRING, PROPERTY_HINT_NONE, "");
	DEF_(ps_server_target_fps_name, 60, Variant::INT, PROPERTY_HINT_NONE, "1,1000");

	// client can change this settings
	DEF_(ps_server_stream_enabled_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_server_compression_type_name, 1 /*GRDevice::ImageCompressionType::JPG*/, Variant::INT, PROPERTY_HINT_ENUM, "Uncompressed (deprecated),JPG,PNG (deprecated),H264");
	DEF_(ps_server_stream_skip_frames_name, 0, Variant::INT, PROPERTY_HINT_RANGE, "0,1000");
	DEF_(ps_server_scale_of_sending_stream_name, 0.75f, Variant::REAL, PROPERTY_HINT_RANGE, "0,1,0.01");
	DEF_(ps_server_stream_quality_name, 85, Variant::INT, PROPERTY_HINT_RANGE, "0,100");
	//DEF_(ps_server_auto_adjust_scale_name, false, Variant::BOOL, PROPERTY_HINT_NONE, "");

#undef DEF_SET
#undef DEF_SET_ENUM
#undef DEF_
}

void GodotRemote::_create_autoload_nodes() {
	if (ST()) {
#ifndef GDNATIVE_LIBRARY
		ST()->get_root()->add_child(godot_remote_root_node);
		ST()->get_root()->move_child(godot_remote_root_node, 0);
#endif

		GRNotifications *notif = memnew(GRNotifications);
		godot_remote_root_node->add_child(notif);
		godot_remote_root_node->move_child(notif, 0);

#if defined(GODOT_REMOTE_TRACY_ENABLED) && defined(TRACY_ENABLE)
		GRProfilerViewportMiniPreview *mini_profiler_preview = memnew(GRProfilerViewportMiniPreview);
		godot_remote_root_node->add_child(mini_profiler_preview);
#endif
	}
}

void GodotRemote::create_and_start_device(ENUM_ARG(DeviceType) type) {
	create_remote_device(type);
	start_remote_device();
}

#if !defined(GDNATIVE_LIBRARY) && defined(TOOLS_ENABLED)
// TODO need to try get every device IDs and setup forwarding for each
#include "TinyProcessLib.hpp"
#include "editor/editor_export.h"
#include "editor/editor_settings.h"

void GodotRemote::_prepare_editor() {
	if (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint()) {
		if (EditorNode::get_singleton())
			EditorNode::get_singleton()->connect("play_pressed", this, NAMEOF(_run_emitted));
	}
}

void GodotRemote::_run_emitted() {
	// call_deferred because debugger can't connect to game if process blocks thread on start
	if ((bool)GET_PS(ps_server_config_adb_name)) {
		if ((bool)GET_PS(ps_general_use_static_port_name)) {
			call_deferred(NAMEOF(_adb_port_forwarding));
		} else {
			_log("\"debug/godot_remote/server/configure_adb_on_play\" option not available without \"debug/godot_remote/general/use_static_port\"", LogLevel::LL_WARNING);
		}
	}
}

void GodotRemote::_adb_port_forwarding() {
#if VERSION_MINOR >= 3 || (VERSION_MINOR >= 2 && VERSION_PATCH > 3)
	String sdk = EditorSettings::get_singleton()->get_setting("export/android/android_sdk_path");
#else
	String sdk = EditorSettings::get_singleton()->get_setting("export/android/adb");
#endif

#if defined(_MSC_VER) || defined(_WIN64) || defined(_WIN32)
#else
	sdk = "ok?";
#endif

	if (!sdk.empty()) {
		if (!is_adb_timer_active) {
			is_adb_timer_active = true;
			Thread_close(adb_config_thread);
			Thread_start(adb_config_thread, this, _adb_config_thread, Variant());
		}
	} else {
		_log("Android SDK path not specified.", LogLevel::LL_DEBUG);
	}
}

void GodotRemote::_adb_config_thread(Variant user_data) {
	Thread_set_name("ADB Configuration thread");
	is_adb_timer_active = false;

#if VERSION_MINOR >= 3 || (VERSION_MINOR >= 2 && VERSION_PATCH > 3)
	String adb = (String(EditorSettings::get_singleton()->get_setting("export/android/android_sdk_path")) + "/platform-tools/adb").replace("//", "/");
#else
	String adb = EditorSettings::get_singleton()->get_setting("export/android/adb");
#endif

#if defined(_MSC_VER) || defined(_WIN64) || defined(_WIN32)
	adb += ".exe";
#else
	adb = "adb";
#endif

	Mutex_define(adb_lock, "ADB Mutex lock");
	std::shared_ptr<TinyProcessLib::Process> proc;
	String std_output = "";
	int exit_code = 0;

	auto wait_adb = [&]() {
		uint64_t start_time = get_time_usec();
		while (!proc->try_get_exit_status(exit_code)) {
			if (get_time_usec() - start_time > 5000_ms) {
				proc->kill(true);
				return true;
			}

			sleep_usec(50_ms);
		}
		return false;
	};

	auto start_adb = [&](String start_line) {
		exit_code = 0;
		auto c = TinyProcessLib::Config();
		c.show_window = TinyProcessLib::Config::ShowWindow::hide;

		proc = std::make_shared<TinyProcessLib::Process>(
				(start_line).utf8().get_data(),
				TinyProcessLib::Process::string_type(),
				[&](const char *bytes, size_t n) {
					Scoped_lock(adb_lock);
					std_output += std::string(bytes, n).c_str();
				},
				[&](const char *bytes, size_t n) {
					Scoped_lock(adb_lock);
					std_output += std::string(bytes, n).c_str();
				},
				true,
				c);
	};

	auto is_output_with_error = [&]() {
		Vector<String> lines = std_output.replace("\r\n", "\n").replace("\r", "\n").split("\n");
		for (int i = 0; i < lines.size(); i++) {
			if (lines[i].begins_with("adb.exe:") || lines[i].begins_with("adb:")) {
				return true;
			}
		}
		return false;
	};

	auto print_adb_not_found = [&]() {
#if defined(_MSC_VER) || defined(_WIN64) || defined(_WIN32)
		_log("ADB: Can't start " + adb, LogLevel::LL_ERROR);
#else
		_log("ADB: Can't start adb", LogLevel::LL_ERROR);
#endif
	};

	int port = GET_PS(ps_general_port_name);

	// 1) get all available devices
	String devices_cmd = String("\"{0}\" devices").format(varray(adb));
	std_output = "";
	start_adb(devices_cmd);

	if (proc->get_id() == 0) {
		print_adb_not_found();
		return;
	} else {

		if (!wait_adb()) {
			// 2) check for errors
			if (exit_code != 0 || is_output_with_error()) {
				_log("ADB error:\n" + std_output, LogLevel::LL_ERROR);
				return;
			}

			std::vector<String> devices;
			String string = std_output.replace("\r\n", "\n").replace("\r", "\n");
			Vector<String> lines = string.split("\n");
			for (int i = 0; i < lines.size(); i++) {
				String line = lines[i];
				if (line.ends_with("device")) {
					line = line.trim_suffix("device").strip_edges();
					if (line.length() > 0) {
						devices.push_back(line);
					}
				}
			}

			if (devices.size() == 0) {
				_log("ADB: No devices found", LogLevel::LL_NORMAL);
				return;
			}

			_log("ADB Devices: " + str_arr(devices, true, 0, ", ", false), LogLevel::LL_NORMAL);

			// 3) run 'adb reverse' for each device
			for (auto dev : devices) {
				String reverse_cmd = String("\"{0}\" -s {1} reverse tcp:{2} tcp:{3}").format(varray(adb, dev, port, port));
				std_output = "";
				start_adb(reverse_cmd);

				if (proc->get_id() == 0) {
					print_adb_not_found();
					return;
				} else {
					if (wait_adb()) {
						_log("ADB freeze detected. Kill and ignore.", LogLevel::LL_WARNING);
						return;
					}
					if (exit_code != 0 || is_output_with_error()) {
						_log("ADB error:\n" + std_output, LogLevel::LL_ERROR);
						return;
					} else {
						_log("ADB port reverse configured for " + dev, LogLevel::LL_NORMAL);
					}
				}
			}

			_log("ADB configuration completed.", LogLevel::LL_DEBUG);
		} else {
			_log("ADB freeze detected. Kill and ignore.", LogLevel::LL_WARNING);
			return;
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// EXTERNAL FUNCTIONS

// GRNotifications
GRNotificationPanel *
GodotRemote::get_notification(String title) {
	return GRNotifications::get_notification(title);
}

int GodotRemote::GodotRemote::notifications_connect(String signal, Object *inst, String method, Array binds, int64_t flags) {
	if (GRNotifications::get_singleton()) {
#ifndef GDNATIVE_LIBRARY
		Vector<Variant> b;
		for (int i = 0; i < binds.size(); i++) {
			b.push_back(binds[i]);
		}
		return (int)GRNotifications::get_singleton()->connect(signal, inst, method, b, flags);
#else
		return (int)GRNotifications::get_singleton()->connect(signal, inst, method, binds, flags);
#endif
	}
	return (int)Error::FAILED;
}

Array GodotRemote::get_all_notifications() {
	return GRNotifications::get_all_notifications();
}
Array GodotRemote::get_notifications_with_title(String title) {
	return GRNotifications::get_notifications_with_title(title);
}
void GodotRemote::set_notifications_layer(int layer) {
	if (GRNotifications::get_singleton())
		GRNotifications::get_singleton()->set_layer(layer);
}
int GodotRemote::get_notifications_layer() {
	if (GRNotifications::get_singleton())
		return (int)GRNotifications::get_singleton()->get_layer();
	return 0;
}
void GodotRemote::set_notifications_position(ENUM_ARG(GRNotifications::NotificationsPosition) positon) {
	GRNotifications::set_notifications_position((GRNotifications::NotificationsPosition)positon);
}
ENUM_ARG(GRNotifications::NotificationsPosition)
GodotRemote::get_notifications_position() {
	return GRNotifications::get_notifications_position();
}
void GodotRemote::set_notifications_enabled(bool _enabled) {
	GRNotifications::set_notifications_enabled(_enabled);
}
bool GodotRemote::get_notifications_enabled() {
	return GRNotifications::get_notifications_enabled();
}
void GodotRemote::set_notifications_duration(float _duration) {
	GRNotifications::set_notifications_duration(_duration);
}
float GodotRemote::get_notifications_duration() {
	return GRNotifications::get_notifications_duration();
}
void GodotRemote::set_notifications_style(Ref<GRNotificationStyle> _style) {
	GRNotifications::set_notifications_style(_style);
}
Ref<GRNotificationStyle> GodotRemote::get_notifications_style() {
	return GRNotifications::get_notifications_style();
}
void GodotRemote::add_notification_or_append_string(String title, String text, ENUM_ARG(GRNotifications::NotificationIcon) icon, bool new_string, float duration_multiplier) {
	GRNotifications::add_notification_or_append_string(title, text, (GRNotifications::NotificationIcon)icon, new_string, duration_multiplier);
}
void GodotRemote::add_notification_or_update_line(String title, String id, String text, ENUM_ARG(GRNotifications::NotificationIcon) icon, float duration_multiplier) {
	GRNotifications::add_notification_or_update_line(title, id, text, (GRNotifications::NotificationIcon)icon, duration_multiplier);
}
void GodotRemote::add_notification(String title, String text, ENUM_ARG(GRNotifications::NotificationIcon) icon, bool update_existing, float duration_multiplier) {
	GRNotifications::add_notification(title, text, (GRNotifications::NotificationIcon)icon, update_existing, duration_multiplier);
}
void GodotRemote::remove_notification(String title, bool all_entries) {
	GRNotifications::remove_notification(title, all_entries);
}
void GodotRemote::remove_notification_exact(Node *_notif) {
	GRNotifications::remove_notification_exact(_notif);
}
void GodotRemote::clear_notifications() {
	GRNotifications::clear_notifications();
}
// GRNotifications end

// GRUtils functions binds for GDScript
void GodotRemote::set_log_level(ENUM_ARG(LogLevel) lvl) {
	GRUtils::set_log_level((LogLevel)lvl);
}
Rect2 GodotRemote::GodotRemote::get_2d_safe_area(CanvasItem *ci) {
	return GRUtils::get_2d_safe_area(ci);
}
void GodotRemote::set_gravity(const Vector3 &p_gravity) {
	GRUtils::set_gravity(p_gravity);
}
void GodotRemote::set_accelerometer(const Vector3 &p_accel) {
	GRUtils::set_accelerometer(p_accel);
}
void GodotRemote::set_magnetometer(const Vector3 &p_magnetometer) {
	GRUtils::set_magnetometer(p_magnetometer);
}
void GodotRemote::set_gyroscope(const Vector3 &p_gyroscope) {
	GRUtils::set_gyroscope(p_gyroscope);
}
// GRUtils end

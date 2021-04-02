/* GodotRemote.cpp */
#include "GodotRemote.h"
#include "GRClient.h"
#include "GRServer.h"

#ifndef GDNATIVE_LIBRARY
#include "editor/editor_node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#else
#include <GlobalConstants.hpp>
#include <ProjectSettings.hpp>
#include <SceneTree.hpp>
#include <Timer.hpp>
using namespace godot;
#endif

#if defined(GODOTREMOTE_TRACY_ENABLED) && defined(TRACY_ENABLE)
#include "GRProfilerViewportMiniPreview.h"
#endif

using namespace GRUtils;

GodotRemote *GodotRemote::singleton = nullptr;
bool GodotRemote::is_init_completed = false;

GR_PS_NAME_TYPE GodotRemote::ps_general_autoload_name = "debug/godot_remote/general/autostart";
GR_PS_NAME_TYPE GodotRemote::ps_general_port_name = "debug/godot_remote/general/port";
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
	if (!singleton)
		singleton = this;

	register_and_load_settings();
	LEAVE_IF_EDITOR();

	GRUtils::init();

	call_deferred(NAMEOF(_create_autoload_nodes));
	if (is_autostart)
		call_deferred(NAMEOF(create_and_start_device), DeviceType::DEVICE_AUTO);

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
	call_deferred(NAMEOF(_prepare_editor));
#endif
#endif
}

void GodotRemote::_deinit() {
	LEAVE_IF_EDITOR();
	remove_remote_device();

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
	call_deferred(NAMEOF(_adb_start_timer_timeout));
#endif
#endif
	if (singleton == this) {
		singleton = nullptr;
	}
	GRUtils::deinit();
}

#ifndef GDNATIVE_LIBRARY

void GodotRemote::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_create_autoload_nodes)), &GodotRemote::_create_autoload_nodes);
#ifdef TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD(NAMEOF(_adb_port_forwarding)), &GodotRemote::_adb_port_forwarding);
	ClassDB::bind_method(D_METHOD(NAMEOF(_run_emitted)), &GodotRemote::_run_emitted);
	ClassDB::bind_method(D_METHOD(NAMEOF(_prepare_editor)), &GodotRemote::_prepare_editor);
	ClassDB::bind_method(D_METHOD(NAMEOF(_adb_start_timer_timeout)), &GodotRemote::_adb_start_timer_timeout);
#endif

	ClassDB::bind_method(D_METHOD(NAMEOF(create_and_start_device), "device_type"), &GodotRemote::create_and_start_device, DEFVAL(DeviceType::DEVICE_AUTO));
	ClassDB::bind_method(D_METHOD(NAMEOF(create_remote_device), "device_type"), &GodotRemote::create_remote_device, DEFVAL(DeviceType::DEVICE_AUTO));
	ClassDB::bind_method(D_METHOD(NAMEOF(start_remote_device)), &GodotRemote::start_remote_device);
	ClassDB::bind_method(D_METHOD(NAMEOF(remove_remote_device)), &GodotRemote::remove_remote_device);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_device)), &GodotRemote::get_device);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_version)), &GodotRemote::get_version);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_gdnative)), &GodotRemote::is_gdnative);

	// GRNotifications
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

	ADD_PROPERTY(PropertyInfo(Variant::INT, "notifications_layer"), "set_notifications_layer", "get_notifications_layer");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "notifications_position"), "set_notifications_position", "get_notifications_position");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "notifications_enabled"), "set_notifications_enabled", "get_notifications_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "notifications_duration"), "set_notifications_duration", "get_notifications_duration");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "notifications_style"), "set_notifications_style", "get_notifications_style");

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

	ClassDB::bind_method(D_METHOD(NAMEOF(set_gravity), "value"), &GodotRemote::set_gravity);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_accelerometer), "value"), &GodotRemote::set_accelerometer);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_magnetometer), "value"), &GodotRemote::set_magnetometer);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_gyroscope), "value"), &GodotRemote::set_gyroscope);

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

	METHOD_REG(GodotRemote, set_gravity);
	METHOD_REG(GodotRemote, set_accelerometer);
	METHOD_REG(GodotRemote, set_magnetometer);
	METHOD_REG(GodotRemote, set_gyroscope);

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

GRDevice *GodotRemote::get_device() {
	return device;
}

String GodotRemote::get_version() {
	PoolByteArray ver = get_gr_version();
	return str(ver[0]) + "." + str(ver[1]) + "." + str(ver[2]);
}

bool GodotRemote::is_gdnative() {
#ifndef GDNATIVE_LIBRARY
	return false;
#else
	return true;
#endif
}

bool GodotRemote::create_remote_device(ENUM_ARG(DeviceType) type) {
	if (!ST()) return false;
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
		call_deferred("emit_signal", "device_added");
		ST()->get_root()->call_deferred("add_child", device);
		ST()->get_root()->call_deferred("move_child", device, 0);
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
		device->stop();
		device->queue_del();
		device = nullptr;
		call_deferred("emit_signal", "device_removed");
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
	DEF_(ps_general_port_name, 52341, Variant::INT, PROPERTY_HINT_RANGE, "0,65535");
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
	DEF_(ps_server_compression_type_name, 1 /*GRDevice::ImageCompressionType::JPG*/, Variant::INT, PROPERTY_HINT_ENUM, "Uncompressed,JPG,PNG (deprecated),H264");
	DEF_(ps_server_stream_skip_frames_name, 0, Variant::INT, PROPERTY_HINT_RANGE, "0,1000");
	DEF_(ps_server_scale_of_sending_stream_name, 0.75f, Variant::REAL, PROPERTY_HINT_RANGE, "0,1,0.01");
	DEF_(ps_server_stream_quality_name, 85, Variant::INT, PROPERTY_HINT_RANGE, "0,100");
	DEF_(ps_server_auto_adjust_scale_name, false, Variant::BOOL, PROPERTY_HINT_NONE, "");

#undef DEF_SET
#undef DEF_SET_ENUM
#undef DEF_
}

void GodotRemote::_create_autoload_nodes() {
	if (ST()) {
		GRNotifications *notif = memnew(GRNotifications);
		ST()->get_root()->add_child(notif);
		ST()->get_root()->move_child(notif, 0);

#if defined(GODOTREMOTE_TRACY_ENABLED) && defined(TRACY_ENABLE)
		GRProfilerViewportMiniPreview *mini_profiler_preview = memnew(GRProfilerViewportMiniPreview);
		ST()->get_root()->add_child(mini_profiler_preview);
#endif
	}
}

void GodotRemote::create_and_start_device(ENUM_ARG(DeviceType) type) {
	create_remote_device(type);
	start_remote_device();
}

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
// TODO need to try get every device IDs and setup forwarding for each
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
	if ((bool)GET_PS(ps_server_config_adb_name))
		call_deferred(NAMEOF(_adb_port_forwarding));
}

void GodotRemote::_adb_port_forwarding() {
	String adb = EditorSettings::get_singleton()->get_setting("export/android/adb");

	if (!adb.empty()) {
		if (!adb_start_timer || adb_start_timer->is_queued_for_deletion()) {
			adb_start_timer = memnew(Timer);
			SceneTree::get_singleton()->get_root()->add_child(adb_start_timer);
			adb_start_timer->set_one_shot(true);
			adb_start_timer->set_autostart(false);
			adb_start_timer->connect("timeout", this, NAMEOF(_adb_start_timer_timeout));
		}

		adb_start_timer->start(4.f);
	} else {
		_log("ADB path not specified.", LogLevel::LL_DEBUG);
	}
}

void GodotRemote::_adb_start_timer_timeout() {
	String adb = EditorSettings::get_singleton()->get_setting("export/android/adb");
	List<String> args;
	args.push_back("reverse");
	args.push_back("--no-rebind");
	args.push_back("tcp:" + str(GET_PS(ps_general_port_name)));
	args.push_back("tcp:" + str(GET_PS(ps_general_port_name)));

	Error err = OS::get_singleton()->execute(adb, args, true); // TODO freezes editor process on closing!!!!

	if (err) {
		String start_url = String("\"{0}\" reverse --no-rebind tcp:{1} tcp:{2}").format(varray(adb, GET_PS(ps_general_port_name), GET_PS(ps_general_port_name)));
		_log("Can't execute adb port forwarding: '" + start_url + "' error code: " + str(err), LogLevel::LL_ERROR);
	} else {
		_log("ADB port configuring completed", LogLevel::LL_NORMAL);
	}

	if (adb_start_timer && !adb_start_timer->is_queued_for_deletion() && adb_start_timer->is_inside_tree()) {
		adb_start_timer->queue_del();
		adb_start_timer = nullptr;
	}
}

#endif
#endif

//////////////////////////////////////////////////////////////////////////
// EXTERNAL FUNCTIONS

GRNotificationPanel *GodotRemote::get_notification(String title) {
	return GRNotifications::get_notification(title);
}

// GRNotifications
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

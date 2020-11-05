/* GodotRemote.cpp */
#include "GodotRemote.h"
#include "GRClient.h"
#include "GRDevice.h"
#include "GRServer.h"

#ifndef GDNATIVE_LIBRARY
#include "core/os/os.h"
#include "core/project_settings.h"
#include "editor/editor_node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/viewport.h"
#else
#include <OS.hpp>
#include <ClassDB.hpp>
#include <GlobalConstants.hpp>
#include <ProjectSettings.hpp>
#include <SceneTree.hpp>
#include <Timer.hpp>
#include <Viewport.hpp>
using namespace godot;
#endif

GodotRemote *GodotRemote::singleton = nullptr;

using namespace GRUtils;

const char* GodotRemote::ps_general_autoload_name = "debug/godot_remote/general/autostart";
const char* GodotRemote::ps_general_port_name = "debug/godot_remote/general/port";
const char* GodotRemote::ps_general_loglevel_name = "debug/godot_remote/general/log_level";

const char* GodotRemote::ps_notifications_enabled_name = "debug/godot_remote/notifications/notifications_enabled";
const char* GodotRemote::ps_noticications_position_name = "debug/godot_remote/notifications/notifications_position";
const char* GodotRemote::ps_notifications_duration_name = "debug/godot_remote/notifications/notifications_duration";

const char* GodotRemote::ps_server_config_adb_name = "debug/godot_remote/server/configure_adb_on_play";
const char* GodotRemote::ps_server_stream_skip_frames_name = "debug/godot_remote/server/skip_frames";
const char* GodotRemote::ps_server_stream_enabled_name = "debug/godot_remote/server/video_stream_enabled";
const char* GodotRemote::ps_server_compression_type_name = "debug/godot_remote/server/compression_type";
const char* GodotRemote::ps_server_jpg_quality_name = "debug/godot_remote/server/jpg_quality";
const char* GodotRemote::ps_server_jpg_buffer_mb_size_name = "debug/godot_remote/server/jpg_compress_buffer_size_mbytes";
const char* GodotRemote::ps_server_auto_adjust_scale_name = "debug/godot_remote/server/auto_adjust_scale";
const char* GodotRemote::ps_server_scale_of_sending_stream_name = "debug/godot_remote/server/scale_of_sending_stream";
const char* GodotRemote::ps_server_password_name = "debug/godot_remote/server/password";

const char* GodotRemote::ps_server_custom_input_scene_name = "debug/godot_remote/server_custom_input_scene/custom_input_scene";
const char* GodotRemote::ps_server_custom_input_scene_compressed_name = "debug/godot_remote/server_custom_input_scene/send_custom_input_scene_compressed";
const char* GodotRemote::ps_server_custom_input_scene_compression_type_name = "debug/godot_remote/server_custom_input_scene/custom_input_scene_compression_type";

GodotRemote *GodotRemote::get_singleton() {
	return singleton;
}

void GodotRemote::_init() {
	if (!singleton)
		singleton = this;
	GRUtils::init();

	register_and_load_settings();
	if (!Engine::get_singleton()->is_editor_hint()) {
		call_deferred("_create_notification_manager");
		if (is_autostart)
			call_deferred("create_and_start_device");
	}

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
	call_deferred("_prepare_editor");
#endif
#endif
}

void GodotRemote::_deinit() {
	remove_remote_device();
	_remove_notifications_manager();

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
	call_deferred("_adb_start_timer_timeout");
#endif
#endif
	if (singleton == this)
	{
		singleton = nullptr;
	}
	GRUtils::deinit();
}

#ifndef GDNATIVE_LIBRARY

void GodotRemote::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_create_notification_manager"), &GodotRemote::_create_notification_manager);

	ClassDB::bind_method(D_METHOD("create_and_start_device", "device_type"), &GodotRemote::create_and_start_device, DEFVAL(DeviceType::DEVICE_Auto));
	ClassDB::bind_method(D_METHOD("create_remote_device", "device_type"), &GodotRemote::create_remote_device, DEFVAL(DeviceType::DEVICE_Auto));
	ClassDB::bind_method(D_METHOD("start_remote_device"), &GodotRemote::start_remote_device);
	ClassDB::bind_method(D_METHOD("remove_remote_device"), &GodotRemote::remove_remote_device);
#ifdef TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD("_adb_port_forwarding"), &GodotRemote::_adb_port_forwarding);
	ClassDB::bind_method(D_METHOD("_run_emitted"), &GodotRemote::_run_emitted);
	ClassDB::bind_method(D_METHOD("_prepare_editor"), &GodotRemote::_prepare_editor);
	ClassDB::bind_method(D_METHOD("_adb_start_timer_timeout"), &GodotRemote::_adb_start_timer_timeout);
#endif

	ClassDB::bind_method(D_METHOD("get_device"), &GodotRemote::get_device);
	ClassDB::bind_method(D_METHOD("get_version"), &GodotRemote::get_version);

	BIND_ENUM_CONSTANT_CUSTOM(DeviceType::DEVICE_Auto, "Auto");
	BIND_ENUM_CONSTANT_CUSTOM(DeviceType::DEVICE_Development, "Development");
	BIND_ENUM_CONSTANT_CUSTOM(DeviceType::DEVICE_Standalone, "Standalone");

	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::USE_INTERNAL_SERVER_SETTINGS, "USE_INTERNAL_SERVER_SETTINGS");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::VIDEO_STREAM_ENABLED, "SERVER_PARAM_VIDEO_STREAM_ENABLED");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::COMPRESSION_TYPE, "SERVER_PARAM_COMPRESSION_TYPE");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::JPG_QUALITY, "SERVER_PARAM_JPG_QUALITY");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::SKIP_FRAMES, "SERVER_PARAM_SKIP_FRAMES");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::RENDER_SCALE, "SERVER_PARAM_RENDER_SCALE");

	// GRNotifications
	ClassDB::bind_method(D_METHOD("get_notification", "title"), &GodotRemote::get_notification);
	ClassDB::bind_method(D_METHOD("get_all_notifications"), &GodotRemote::get_all_notifications);
	ClassDB::bind_method(D_METHOD("get_notifications_with_title", "title"), &GodotRemote::get_notifications_with_title);

	ClassDB::bind_method(D_METHOD("add_notification_or_append_string", "title", "text", "icon", "add_to_new_line"), &GodotRemote::add_notification_or_append_string, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("add_notification_or_update_line", "title", "id", "text", "icon", "duration_multiplier"), &GodotRemote::add_notification_or_update_line, DEFVAL(1.f));
	ClassDB::bind_method(D_METHOD("add_notification", "title", "text", "notification_icon", "update_existing", "duration_multiplier"), &GodotRemote::add_notification, DEFVAL((int)NotificationIcon::None), DEFVAL(true), DEFVAL(1.f));
	ClassDB::bind_method(D_METHOD("remove_notification", "title", "is_all_entries"), &GodotRemote::remove_notification, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("remove_notification_exact", "notification"), &GodotRemote::remove_notification_exact);
	ClassDB::bind_method(D_METHOD("clear_notifications"), &GodotRemote::clear_notifications);

	ClassDB::bind_method(D_METHOD("set_notifications_layer", "position"), &GodotRemote::set_notifications_layer);
	ClassDB::bind_method(D_METHOD("set_notifications_position", "position"), &GodotRemote::set_notifications_position);
	ClassDB::bind_method(D_METHOD("set_notifications_enabled", "enabled"), &GodotRemote::set_notifications_enabled);
	ClassDB::bind_method(D_METHOD("set_notifications_duration", "duration"), &GodotRemote::set_notifications_duration);
	ClassDB::bind_method(D_METHOD("set_notifications_style", "style"), &GodotRemote::set_notifications_style);

	ClassDB::bind_method(D_METHOD("get_notifications_layer"), &GodotRemote::get_notifications_layer);
	ClassDB::bind_method(D_METHOD("get_notifications_position"), &GodotRemote::get_notifications_position);
	ClassDB::bind_method(D_METHOD("get_notifications_enabled"), &GodotRemote::get_notifications_enabled);
	ClassDB::bind_method(D_METHOD("get_notifications_duration"), &GodotRemote::get_notifications_duration);
	ClassDB::bind_method(D_METHOD("get_notifications_style"), &GodotRemote::get_notifications_style);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "notifications_layer"), "set_notifications_layer", "get_notifications_layer");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "notifications_position"), "set_notifications_position", "get_notifications_position");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "notifications_enabled"), "set_notifications_enabled", "get_notifications_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "notifications_duration"), "set_notifications_duration", "get_notifications_duration");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "notifications_style"), "set_notifications_style", "get_notifications_style");

	ADD_SIGNAL(MethodInfo("device_added"));
	ADD_SIGNAL(MethodInfo("device_removed"));

	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::None, "NOTIFICATION_ICON_NONE");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::_Error, "NOTIFICATION_ICON_ERROR");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::Warning, "NOTIFICATION_ICON_WARNING");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::Success, "NOTIFICATION_ICON_SUCCESS");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::Fail, "NOTIFICATION_ICON_FAIL");

	BIND_ENUM_CONSTANT_CUSTOM(NotificationsPosition::TL, "NOTIFICATIONS_POSITION_TL");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationsPosition::TC, "NOTIFICATIONS_POSITION_TC");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationsPosition::TR, "NOTIFICATIONS_POSITION_TR");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationsPosition::BL, "NOTIFICATIONS_POSITION_BL");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationsPosition::BC, "NOTIFICATIONS_POSITION_BC");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationsPosition::BR, "NOTIFICATIONS_POSITION_BR");

	// GRUtils
	BIND_ENUM_CONSTANT_CUSTOM(Subsampling::SUBSAMPLING_Y_ONLY, "Y_ONLY");
	BIND_ENUM_CONSTANT_CUSTOM(Subsampling::SUBSAMPLING_H1V1, "H1V1");
	BIND_ENUM_CONSTANT_CUSTOM(Subsampling::SUBSAMPLING_H2V1, "H2V1");
	BIND_ENUM_CONSTANT_CUSTOM(Subsampling::SUBSAMPLING_H2V2, "H2V2");

	BIND_ENUM_CONSTANT_CUSTOM(LogLevel::LL_None, "NONE");
	BIND_ENUM_CONSTANT_CUSTOM(LogLevel::LL_Debug, "DEBUG");
	BIND_ENUM_CONSTANT_CUSTOM(LogLevel::LL_Normal, "NORMAL");
	BIND_ENUM_CONSTANT_CUSTOM(LogLevel::LL_Warning, "WARNING");
	BIND_ENUM_CONSTANT_CUSTOM(LogLevel::LL_Error, "ERROR");

	BIND_ENUM_CONSTANT_CUSTOM(ImageCompressionType::Uncompressed, "IMAGE_COMPRESSION_UNCOMPRESSED");
	BIND_ENUM_CONSTANT_CUSTOM(ImageCompressionType::JPG, "IMAGE_COMPRESSION_JPG");
	BIND_ENUM_CONSTANT_CUSTOM(ImageCompressionType::PNG, "IMAGE_COMPRESSION_PNG");

	ClassDB::bind_method(D_METHOD("set_log_level", "level"), &GodotRemote::set_log_level);

	ClassDB::bind_method(D_METHOD("set_gravity", "value"), &GodotRemote::set_gravity);
	ClassDB::bind_method(D_METHOD("set_accelerometer", "value"), &GodotRemote::set_accelerometer);
	ClassDB::bind_method(D_METHOD("set_magnetometer", "value"), &GodotRemote::set_magnetometer);
	ClassDB::bind_method(D_METHOD("set_gyroscope", "value"), &GodotRemote::set_gyroscope);
}

#else

void GodotRemote::_register_methods() {
	register_method("_notification", &GodotRemote::_notification);

	register_method("_create_notification_manager", &GodotRemote::_create_notification_manager);

	register_method("create_and_start_device", &GodotRemote::create_and_start_device);
	register_method("create_remote_device", &GodotRemote::create_remote_device);
	register_method("start_remote_device", &GodotRemote::start_remote_device);
	register_method("remove_remote_device", &GodotRemote::remove_remote_device);

	register_method("get_device", &GodotRemote::get_device);
	register_method("get_version", &GodotRemote::get_version);

	BIND_ENUM_CONSTANT(singleton, DeviceType::DEVICE_Auto, "Auto");
	BIND_ENUM_CONSTANT(singleton, DeviceType::DEVICE_Development, "Development");
	BIND_ENUM_CONSTANT(singleton, DeviceType::DEVICE_Standalone, "Standalone");

	BIND_ENUM_CONSTANT(singleton, TypesOfServerSettings::USE_INTERNAL_SERVER_SETTINGS, "USE_INTERNAL_SERVER_SETTINGS");
	BIND_ENUM_CONSTANT(singleton, TypesOfServerSettings::VIDEO_STREAM_ENABLED, "SERVER_PARAM_VIDEO_STREAM_ENABLED");
	BIND_ENUM_CONSTANT(singleton, TypesOfServerSettings::COMPRESSION_TYPE, "SERVER_PARAM_COMPRESSION_TYPE");
	BIND_ENUM_CONSTANT(singleton, TypesOfServerSettings::JPG_QUALITY, "SERVER_PARAM_JPG_QUALITY");
	BIND_ENUM_CONSTANT(singleton, TypesOfServerSettings::SKIP_FRAMES, "SERVER_PARAM_SKIP_FRAMES");
	BIND_ENUM_CONSTANT(singleton, TypesOfServerSettings::RENDER_SCALE, "SERVER_PARAM_RENDER_SCALE");
	
	// GRNotifications
	register_method("get_notification", &GodotRemote::get_notification);

	register_method("get_all_notifications", &GodotRemote::get_all_notifications);
	register_method("get_notifications_with_title", &GodotRemote::get_notifications_with_title);

	register_method("add_notification_or_append_string", &GodotRemote::add_notification_or_append_string);
	register_method("add_notification_or_update_line", &GodotRemote::add_notification_or_update_line);
	register_method("add_notification", &GodotRemote::add_notification);
	register_method("remove_notification", &GodotRemote::remove_notification);
	register_method("remove_notification_exact", &GodotRemote::remove_notification_exact);
	register_method("clear_notifications", &GodotRemote::clear_notifications);

	register_method("set_notifications_layer", &GodotRemote::set_notifications_layer);
	register_method("set_notifications_position", &GodotRemote::set_notifications_position);
	register_method("set_notifications_enabled", &GodotRemote::set_notifications_enabled);
	register_method("set_notifications_duration", &GodotRemote::set_notifications_duration);
	register_method("set_notifications_style", &GodotRemote::set_notifications_style);

	register_method("get_notifications_layer", &GodotRemote::get_notifications_layer);
	register_method("get_notifications_position", &GodotRemote::get_notifications_position);
	register_method("get_notifications_enabled", &GodotRemote::get_notifications_enabled);
	register_method("get_notifications_duration", &GodotRemote::get_notifications_duration);
	register_method("get_notifications_style", &GodotRemote::get_notifications_style);

	//register_property<GodotRemote, int>("notifications_layer", &GodotRemote::set_notifications_layer, &GodotRemote::get_notifications_layer, 128);
	//register_property<GodotRemote, int>("notifications_position", &GodotRemote::set_notifications_position, &GodotRemote::get_notifications_position, NotificationsPosition::TC);
	//register_property<GodotRemote, bool>("notifications_enabled", &GodotRemote::set_notifications_enabled, &GodotRemote::get_notifications_enabled, true);
	//register_property<GodotRemote, float>("notifications_duration", &GodotRemote::set_notifications_duration, &GodotRemote::get_notifications_duration, 3.f);
	//register_property<GodotRemote, Object>("notifications_style", &GodotRemote::set_notifications_style, &GodotRemote::get_notifications_style, nullptr);

	register_signal<GodotRemote>("device_added", Dictionary::make());
	register_signal<GodotRemote>("device_removed", Dictionary::make());

	BIND_ENUM_CONSTANT(singleton, NotificationIcon::None, "NOTIFICATION_ICON_NONE");
	BIND_ENUM_CONSTANT(singleton, NotificationIcon::_Error, "NOTIFICATION_ICON_ERROR");
	BIND_ENUM_CONSTANT(singleton, NotificationIcon::Warning, "NOTIFICATION_ICON_WARNING");
	BIND_ENUM_CONSTANT(singleton, NotificationIcon::Success, "NOTIFICATION_ICON_SUCCESS");
	BIND_ENUM_CONSTANT(singleton, NotificationIcon::Fail, "NOTIFICATION_ICON_FAIL");

	BIND_ENUM_CONSTANT(singleton, NotificationsPosition::TL, "NOTIFICATIONS_POSITION_TL");
	BIND_ENUM_CONSTANT(singleton, NotificationsPosition::TC, "NOTIFICATIONS_POSITION_TC");
	BIND_ENUM_CONSTANT(singleton, NotificationsPosition::TR, "NOTIFICATIONS_POSITION_TR");
	BIND_ENUM_CONSTANT(singleton, NotificationsPosition::BL, "NOTIFICATIONS_POSITION_BL");
	BIND_ENUM_CONSTANT(singleton, NotificationsPosition::BC, "NOTIFICATIONS_POSITION_BC");
	BIND_ENUM_CONSTANT(singleton, NotificationsPosition::BR, "NOTIFICATIONS_POSITION_BR");

	// GRUtils
	BIND_ENUM_CONSTANT(singleton, Subsampling::SUBSAMPLING_Y_ONLY, "Y_ONLY");
	BIND_ENUM_CONSTANT(singleton, Subsampling::SUBSAMPLING_H1V1, "H1V1");
	BIND_ENUM_CONSTANT(singleton, Subsampling::SUBSAMPLING_H2V1, "H2V1");
	BIND_ENUM_CONSTANT(singleton, Subsampling::SUBSAMPLING_H2V2, "H2V2");

	BIND_ENUM_CONSTANT(singleton, LogLevel::LL_None, "NONE");
	BIND_ENUM_CONSTANT(singleton, LogLevel::LL_Debug, "DEBUG");
	BIND_ENUM_CONSTANT(singleton, LogLevel::LL_Normal, "NORMAL");
	BIND_ENUM_CONSTANT(singleton, LogLevel::LL_Warning, "WARNING");
	BIND_ENUM_CONSTANT(singleton, LogLevel::LL_Error, "ERROR");

	BIND_ENUM_CONSTANT(singleton, ImageCompressionType::Uncompressed, "IMAGE_COMPRESSION_UNCOMPRESSED");
	BIND_ENUM_CONSTANT(singleton, ImageCompressionType::JPG, "IMAGE_COMPRESSION_JPG");
	BIND_ENUM_CONSTANT(singleton, ImageCompressionType::PNG, "IMAGE_COMPRESSION_PNG");

	register_method("set_log_level", &GodotRemote::set_log_level);

	register_method("set_gravity", &GodotRemote::set_gravity);
	register_method("set_accelerometer", &GodotRemote::set_accelerometer);
	register_method("set_magnetometer", &GodotRemote::set_magnetometer);
	register_method("set_gyroscope", &GodotRemote::set_gyroscope);
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

GRDevice *GodotRemote::get_device() const {
	return device;
}

String GodotRemote::get_version() const {
	PoolByteArray ver = get_gr_version();
	return str(ver[0]) + "." + str(ver[1]) + "." + str(ver[2]);
}

bool GodotRemote::create_remote_device(DeviceType type) {
	remove_remote_device();

	GRDevice *d = nullptr;

	switch (type) {
		// automatically start server if it not a standalone build
		case GodotRemote::DEVICE_Auto:
			if (!OS::get_singleton()->has_feature("standalone")) {
#ifndef NO_GODOTREMOTE_SERVER
				d = memnew(GRServer);
#else
				ERR_FAIL_V_MSG(false, "Server not included in this build!");
#endif
			}
			break;
		case GodotRemote::DEVICE_Development:
#ifndef NO_GODOTREMOTE_SERVER
			d = memnew(GRServer);
#else
			ERR_FAIL_V_MSG(false, "Server not included in this build!");
#endif
			break;
		case GodotRemote::DEVICE_Standalone:
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
#define DEF_SET(var, name, def_val, def_val, info_type, info_hint_type, info_hint_string) \
	var = GLOBAL_DEF(name, def_val);                                                      \
	ProjectSettings::get_singleton()->set_custom_property_info(name, PropertyInfo(info_type, name, info_hint_type, info_hint_string))
#define DEF_SET_ENUM(var, type, name, def_val, def_val, info_type, info_hint_type, info_hint_string) \
	var = (type)(int)GLOBAL_DEF(name, def_val);                                                      \
	ProjectSettings::get_singleton()->set_custom_property_info(name, PropertyInfo(info_type, name, info_hint_type, info_hint_string))
#define DEF_(name, def_val, def_val, info_type, info_hint_type, info_hint_string) \
	GLOBAL_DEF(name, def_val);                                                    \
	ProjectSettings::get_singleton()->set_custom_property_info(name, PropertyInfo(info_type, name, info_hint_type, info_hint_string))

#else

#define DEF_SET(var, name, def_val, info_type, info_hint_type, info_hint_string) \
{                                                                                \
	Dictionary d;                                                                \
	d["name"] = name;                                                            \
	d["type"] = info_type;                                                       \
	d["hint"] = GlobalConstants::info_hint_type;                                 \
	d["hint_string"] = info_hint_string;                                         \
	var = GLOBAL_DEF(name, def_val);                                             \
	ProjectSettings::get_singleton()->add_property_info(d);                      \
}
#define DEF_SET_ENUM(var, type, name, def_val, info_type, info_hint_type, info_hint_string) \
{                                                                                           \
		Dictionary d;                                                                       \
		d["name"] = name;                                                                   \
		d["type"] = info_type;                                                              \
		d["hint"] = GlobalConstants::info_hint_type;                                        \
		d["hint_string"] = info_hint_string;                                                \
		var = (type)(int)GLOBAL_DEF(name, def_val);                                         \
		ProjectSettings::get_singleton()->add_property_info(d);                             \
}
#define DEF_(name, def_val, info_type, info_hint_type, info_hint_string) \
{                                                                        \
	Dictionary d;                                                        \
	d["name"] = name;                                                    \
	d["type"] = info_type;                                               \
	d["hint"] = GlobalConstants::info_hint_type;                         \
	d["hint_string"] = info_hint_string;                                 \
	GLOBAL_DEF(name, def_val);                                           \
	ProjectSettings::get_singleton()->add_property_info(d);              \
}

#endif

	DEF_SET(is_autostart, ps_general_autoload_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_general_port_name, 52341, Variant::INT, PROPERTY_HINT_RANGE, "0,65535");
	DEF_(ps_general_loglevel_name, LogLevel::LL_Normal, Variant::INT, PROPERTY_HINT_ENUM, "Debug,Normal,Warning,Error,None");

	DEF_(ps_notifications_enabled_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_noticications_position_name, NotificationsPosition::TC, Variant::INT, PROPERTY_HINT_ENUM, "TopLeft,TopCenter,TopRight,BottomLeft,BottomCenter,BottomRight");
	DEF_(ps_notifications_duration_name, 3.f, Variant::REAL, PROPERTY_HINT_RANGE, "0,100, 0.01");

	// const server settings
	DEF_(ps_server_config_adb_name, false, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_server_custom_input_scene_name, "", Variant::STRING, PROPERTY_HINT_FILE, "*.tscn,*.scn");
#ifndef GDNATIVE_LIBRARY
	DEF_(ps_server_custom_input_scene_compressed_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_server_custom_input_scene_compression_type_name, 0, Variant::INT, PROPERTY_HINT_ENUM, "FastLZ,DEFLATE,zstd,gzip");
#endif
	DEF_(ps_server_jpg_buffer_mb_size_name, 4, Variant::INT, PROPERTY_HINT_RANGE, "1,128");

	// only server can change this settings
	DEF_(ps_server_password_name, "", Variant::STRING, PROPERTY_HINT_NONE, "");

	// client can change this settings
	DEF_(ps_server_stream_enabled_name, true, Variant::BOOL, PROPERTY_HINT_NONE, "");
	DEF_(ps_server_compression_type_name, ImageCompressionType::JPG, Variant::INT, PROPERTY_HINT_ENUM, "Uncompressed,JPG,PNG");
	DEF_(ps_server_stream_skip_frames_name, 0, Variant::INT, PROPERTY_HINT_RANGE, "0,1000");
	DEF_(ps_server_scale_of_sending_stream_name, 0.25f, Variant::REAL, PROPERTY_HINT_RANGE, "0,1,0.01");
	DEF_(ps_server_jpg_quality_name, 80, Variant::INT, PROPERTY_HINT_RANGE, "0,100");
	DEF_(ps_server_auto_adjust_scale_name, false, Variant::BOOL, PROPERTY_HINT_NONE, "");

#undef DEF_SET
#undef DEF_SET_ENUM
#undef DEF_
}

void GodotRemote::_create_notification_manager() {
	GRNotifications *notif = memnew(GRNotifications);
	ST()->get_root()->call_deferred("add_child", notif);
	ST()->get_root()->call_deferred("move_child", notif, 0);
}

void GodotRemote::_remove_notifications_manager() {
	GRNotificationPanel::clear_styles();
	if (GRNotifications::get_singleton() && !GRNotifications::get_singleton()->is_queued_for_deletion()) {
		ST()->get_root()->remove_child(GRNotifications::get_singleton());
		memdelete(GRNotifications::get_singleton());
	}
}

void GodotRemote::create_and_start_device(DeviceType type) {
	create_remote_device(type);
	start_remote_device();
}

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
// TODO need to try get every device IDs and setup forwarding for each
#include "editor/editor_export.h"
#include "editor/editor_settings.h"

void GodotRemote::_prepare_editor() {
	if (Engine::get_singleton()->is_editor_hint()) {
		if (EditorNode::get_singleton())
			EditorNode::get_singleton()->connect("play_pressed", this, "_run_emitted");
	}
}

void GodotRemote::_run_emitted() {
	// call_deferred because debugger can't connect to game if process blocks thread on start
	if ((bool)GET_PS(ps_server_config_adb_name))
		call_deferred("_adb_port_forwarding");
}

void GodotRemote::_adb_port_forwarding() {
	String adb = EditorSettings::get_singleton()->get_setting("export/android/adb");

	if (!adb.empty()) {
		if (!adb_start_timer || adb_start_timer->is_queued_for_deletion()) {
			adb_start_timer = memnew(Timer);
			SceneTree::get_singleton()->get_root()->add_child(adb_start_timer);
			adb_start_timer->set_one_shot(true);
			adb_start_timer->set_autostart(false);
			adb_start_timer->connect("timeout", this, "_adb_start_timer_timeout");
		}

		adb_start_timer->start(4.f);
	} else {
		_log("ADB path not specified.", LogLevel::LL_Debug);
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
		_log("Can't execute adb port forwarding: '" + start_url + "' error code: " + str(err), LogLevel::LL_Error);
	} else {
		_log("ADB port configuring completed", LogLevel::LL_Normal);
	}

	if (adb_start_timer && !adb_start_timer->is_queued_for_deletion() && adb_start_timer->is_inside_tree()) {
		adb_start_timer->queue_delete();
		adb_start_timer = nullptr;
	}
}

#endif
#endif

//////////////////////////////////////////////////////////////////////////
// EXTERNAL FUNCTIONS

GRNotificationPanel *GodotRemote::get_notification(String title) const {
	return GRNotifications::get_notification(title);
}

// GRNotifications
Array GodotRemote::get_all_notifications() const {
	return GRNotifications::get_all_notifications();
}
Array GodotRemote::get_notifications_with_title(String title) const {
	return GRNotifications::get_notifications_with_title(title);
}
void GodotRemote::set_notifications_layer(int layer) const {
	if (GRNotifications::get_singleton())
		GRNotifications::get_singleton()->set_layer(layer);
}
int GodotRemote::get_notifications_layer() const {
	if (GRNotifications::get_singleton())
		return GRNotifications::get_singleton()->get_layer();
	return 0;
}
void GodotRemote::set_notifications_position(NotificationsPosition positon) const {
	GRNotifications::set_notifications_position(positon);
}
NotificationsPosition GodotRemote::get_notifications_position() const {
	return GRNotifications::get_notifications_position();
}
void GodotRemote::set_notifications_enabled(bool _enabled) const {
	GRNotifications::set_notifications_enabled(_enabled);
}
bool GodotRemote::get_notifications_enabled() const {
	return GRNotifications::get_notifications_enabled();
}
void GodotRemote::set_notifications_duration(float _duration) const {
	GRNotifications::set_notifications_duration(_duration);
}
float GodotRemote::get_notifications_duration() const {
	return GRNotifications::get_notifications_duration();
}
void GodotRemote::set_notifications_style(Ref<GRNotificationStyle> _style) const {
	GRNotifications::set_notifications_style(_style);
}
Ref<GRNotificationStyle> GodotRemote::get_notifications_style() const {
	return GRNotifications::get_notifications_style();
}
void GodotRemote::add_notification_or_append_string(String title, String text, NotificationIcon icon, bool new_string) {
	GRNotifications::add_notification_or_append_string(title, text, icon, new_string);
}
void GodotRemote::add_notification_or_update_line(String title, String id, String text, NotificationIcon icon, float duration_multiplier) const {
	GRNotifications::add_notification_or_update_line(title, id, text, icon, duration_multiplier);
}
void GodotRemote::add_notification(String title, String text, NotificationIcon icon, bool update_existing, float duration_multiplier) const {
	GRNotifications::add_notification(title, text, icon, update_existing, duration_multiplier);
}
void GodotRemote::remove_notification(String title, bool all_entries) const {
	GRNotifications::remove_notification(title, all_entries);
}
void GodotRemote::remove_notification_exact(Node *_notif) const {
	GRNotifications::remove_notification_exact(_notif);
}
void GodotRemote::clear_notifications() const {
	GRNotifications::clear_notifications();
}
// GRNotifications end

// GRUtils functions binds for GDScript
void GodotRemote::set_log_level(LogLevel lvl) const {
	GRUtils::set_log_level((LogLevel)lvl);
}
void GodotRemote::set_gravity(const Vector3 &p_gravity) const {
	GRUtils::set_gravity(p_gravity);
}
void GodotRemote::set_accelerometer(const Vector3 &p_accel) const {
	GRUtils::set_accelerometer(p_accel);
}
void GodotRemote::set_magnetometer(const Vector3 &p_magnetometer) const {
	GRUtils::set_magnetometer(p_magnetometer);
}
void GodotRemote::set_gyroscope(const Vector3 &p_gyroscope) const {
	GRUtils::set_gyroscope(p_gyroscope);
}
// GRUtils end

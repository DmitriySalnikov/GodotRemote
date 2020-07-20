/* GodotRemote.cpp */
#include "GodotRemote.h"
#include "GRClient.h"
#include "GRDevice.h"
#include "GRNotifications.h"
#include "GRServer.h"
#include "core/os/os.h"
#include "core/project_settings.h"
#include "editor/editor_node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"

GodotRemote *GodotRemote::singleton = nullptr;

using namespace GRUtils;

String GodotRemote::ps_autoload_name = "debug/godot_remote/general/autostart";
String GodotRemote::ps_port_name = "debug/godot_remote/general/port";
String GodotRemote::ps_loglevel_name = "debug/godot_remote/general/log_level";

String GodotRemote::ps_notifications_enabled_name = "debug/godot_remote/notifications/notifications_enabled";
String GodotRemote::ps_noticications_position_name = "debug/godot_remote/notifications/noticications_position";
String GodotRemote::ps_notifications_duration_name = "debug/godot_remote/notifications/notifications_duration";

String GodotRemote::ps_config_adb_name = "debug/godot_remote/server/configure_adb_on_play";
String GodotRemote::ps_server_stream_fps_name = "debug/godot_remote/server/stream_fps";
String GodotRemote::ps_server_compression_type_name = "debug/godot_remote/server/compression_type";
String GodotRemote::ps_jpg_quality_name = "debug/godot_remote/server/jpg_quality";
String GodotRemote::ps_jpg_buffer_mb_size_name = "debug/godot_remote/server/jpg_compress_buffer_size_mbytes";
String GodotRemote::ps_auto_adjust_scale_name = "debug/godot_remote/server/auto_adjust_scale";
String GodotRemote::ps_scale_of_sending_stream_name = "debug/godot_remote/server/scale_of_sending_stream";
String GodotRemote::ps_password_name = "debug/godot_remote/server/password";

GodotRemote *GodotRemote::get_singleton() {
	return singleton;
}

GodotRemote::GodotRemote() {
	if (!singleton)
		singleton = this;

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint())
		if (EditorNode::get_singleton())
			EditorNode::get_singleton()->connect("play_pressed", this, "_run_emitted");
#endif

	register_and_load_settings();
	if (!Engine::get_singleton()->is_editor_hint()) {
		call_deferred("_create_notification_manager");
		if (is_autostart)
			call_deferred("create_and_start_device");
	}
}

GodotRemote::~GodotRemote() {
	remove_remote_device();
	_remove_notifications_manager();
}

void GodotRemote::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_create_notification_manager"), &GodotRemote::_create_notification_manager);

	ClassDB::bind_method(D_METHOD("create_and_start_device", "device_type"), &GodotRemote::create_and_start_device, DEFVAL(DeviceType::DEVICE_Auto));
	ClassDB::bind_method(D_METHOD("create_remote_device", "device_type"), &GodotRemote::create_remote_device, DEFVAL(DeviceType::DEVICE_Auto));
	ClassDB::bind_method(D_METHOD("start_remote_device"), &GodotRemote::start_remote_device);
	ClassDB::bind_method(D_METHOD("remove_remote_device"), &GodotRemote::remove_remote_device);
#ifdef TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD("_adb_port_forwarding"), &GodotRemote::_adb_port_forwarding);
	ClassDB::bind_method(D_METHOD("_run_emitted"), &GodotRemote::_run_emitted);
#endif

	ClassDB::bind_method(D_METHOD("get_device"), &GodotRemote::get_device);

	BIND_ENUM_CONSTANT(DEVICE_Auto);
	BIND_ENUM_CONSTANT(DEVICE_Development);
	BIND_ENUM_CONSTANT(DEVICE_Standalone);

	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::USE_INTERNAL_SERVER_SETTINGS, "USE_INTERNAL_SERVER_SETTINGS");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::COMPRESSION_TYPE, "SERVER_PARAM_COMPRESSION_TYPE");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::JPG_QUALITY, "SERVER_PARAM_JPG_QUALITY");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::SEND_FPS, "SERVER_PARAM_SEND_FPS");
	BIND_ENUM_CONSTANT_CUSTOM(TypesOfServerSettings::RENDER_SCALE, "SERVER_PARAM_RENDER_SCALE");

	// GRNotifications
	ClassDB::bind_method(D_METHOD("get_notification", "title"), &GodotRemote::get_notification);
	ClassDB::bind_method(D_METHOD("get_all_notifications"), &GodotRemote::get_all_notifications);
	ClassDB::bind_method(D_METHOD("get_notifications_with_title", "title"), &GodotRemote::get_notifications_with_title);

	ClassDB::bind_method(D_METHOD("add_notification_or_append_string", "title", "text", "add_to_new_line"), &GodotRemote::add_notification_or_append_string, DEFVAL(true));
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

	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::None, "NOTIFICATION_ICON_NONE");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::Error, "NOTIFICATION_ICON_ERROR");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::Warning, "NOTIFICATION_ICON_WARNING");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::Success, "NOTIFICATION_ICON_SUCCESS");
	BIND_ENUM_CONSTANT_CUSTOM(NotificationIcon::Fail, "NOTIFICATION_ICON_FAIL");

	BIND_ENUM_CONSTANT_CUSTOM(GRNotifications::NotificationsPosition::TL, "NOTIFICATIONS_POSITION_TL");
	BIND_ENUM_CONSTANT_CUSTOM(GRNotifications::NotificationsPosition::TC, "NOTIFICATIONS_POSITION_TC");
	BIND_ENUM_CONSTANT_CUSTOM(GRNotifications::NotificationsPosition::TR, "NOTIFICATIONS_POSITION_TR");
	BIND_ENUM_CONSTANT_CUSTOM(GRNotifications::NotificationsPosition::BL, "NOTIFICATIONS_POSITION_BL");
	BIND_ENUM_CONSTANT_CUSTOM(GRNotifications::NotificationsPosition::BC, "NOTIFICATIONS_POSITION_BC");
	BIND_ENUM_CONSTANT_CUSTOM(GRNotifications::NotificationsPosition::BR, "NOTIFICATIONS_POSITION_BR");

	// GRUtils
	BIND_ENUM_CONSTANT(SUBSAMPLING_Y_ONLY);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H1V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V2);

	BIND_ENUM_CONSTANT(LL_None);
	BIND_ENUM_CONSTANT(LL_Debug);
	BIND_ENUM_CONSTANT(LL_Normal);
	BIND_ENUM_CONSTANT(LL_Warning);
	BIND_ENUM_CONSTANT(LL_Error);

	BIND_ENUM_CONSTANT_CUSTOM(ImageCompressionType::Uncompressed, "IMAGE_COMPRESSION_UNCOMPRESSED");
	BIND_ENUM_CONSTANT_CUSTOM(ImageCompressionType::JPG, "IMAGE_COMPRESSION_JPG");
	BIND_ENUM_CONSTANT_CUSTOM(ImageCompressionType::PNG, "IMAGE_COMPRESSION_PNG");

	ClassDB::bind_method(D_METHOD("set_log_level", "level"), &GodotRemote::set_log_level);

	ClassDB::bind_method(D_METHOD("set_gravity", "value"), &GodotRemote::set_gravity);
	ClassDB::bind_method(D_METHOD("set_accelerometer", "value"), &GodotRemote::set_accelerometer);
	ClassDB::bind_method(D_METHOD("set_magnetometer", "value"), &GodotRemote::set_magnetometer);
	ClassDB::bind_method(D_METHOD("set_gyroscope", "value"), &GodotRemote::set_gyroscope);
}

void GodotRemote::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_PREDELETE:
			break;
	}
}

GRDevice *GodotRemote::get_device() const {
	return device;
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
		SceneTree::get_singleton()->get_root()->call_deferred("add_child", device);
		SceneTree::get_singleton()->get_root()->call_deferred("move_child", device, 0);
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
		device->queue_delete();
		device = nullptr;
		return true;
	}
	return false;
}

void GodotRemote::register_and_load_settings() {
#define DEF_SET(var, name, def_val, info) \
	var = GLOBAL_DEF(name, def_val);      \
	ProjectSettings::get_singleton()->set_custom_property_info(name, info)
#define DEF_SET_ENUM(var, type, name, def_val, info) \
	var = (type)(int)GLOBAL_DEF(name, def_val);      \
	ProjectSettings::get_singleton()->set_custom_property_info(name, info)
#define DEF_(name, def_val, info) \
	GLOBAL_DEF(name, def_val);    \
	ProjectSettings::get_singleton()->set_custom_property_info(name, info)

	DEF_SET(is_autostart, ps_autoload_name, true, PropertyInfo(Variant::BOOL, ps_autoload_name));
	DEF_(ps_port_name, 52341, PropertyInfo(Variant::INT, ps_port_name, PROPERTY_HINT_RANGE, "0,65535"));
	DEF_(ps_loglevel_name, GRUtils::LogLevel::LL_Normal, PropertyInfo(Variant::INT, ps_loglevel_name, PROPERTY_HINT_ENUM, "Debug,Normal,Warning,Error,None"));

	DEF_(ps_notifications_enabled_name, true, PropertyInfo(Variant::BOOL, ps_notifications_enabled_name));
	DEF_(ps_noticications_position_name, (int)GRNotifications::NotificationsPosition::TC, PropertyInfo(Variant::INT, ps_noticications_position_name, PROPERTY_HINT_ENUM, "TopLeft,TopCenter,TopRight,BottomLeft,BottomCenter,BottomRight"));
	DEF_(ps_notifications_duration_name, 3.f, PropertyInfo(Variant::REAL, ps_notifications_duration_name, PROPERTY_HINT_RANGE, "0,100, 0.01"));

	// const server settings
	DEF_(ps_config_adb_name, true, PropertyInfo(Variant::BOOL, ps_config_adb_name));
	DEF_(ps_jpg_buffer_mb_size_name, 4, PropertyInfo(Variant::INT, ps_jpg_buffer_mb_size_name, PROPERTY_HINT_RANGE, "1,128"));

	// only server can change this settings
	DEF_(ps_password_name, "", PropertyInfo(Variant::STRING, ps_password_name));

	// client can change this settings
	DEF_(ps_server_compression_type_name, (int)ImageCompressionType::JPG, PropertyInfo(Variant::INT, ps_server_compression_type_name, PROPERTY_HINT_ENUM, "Uncompressed,JPG,PNG"));
	DEF_(ps_server_stream_fps_name, 60, PropertyInfo(Variant::INT, ps_server_stream_fps_name, PROPERTY_HINT_RANGE, "1,1000"));
	DEF_(ps_scale_of_sending_stream_name, 0.3f, PropertyInfo(Variant::REAL, ps_scale_of_sending_stream_name, PROPERTY_HINT_RANGE, "0,1,0.01"));
	DEF_(ps_jpg_quality_name, 75, PropertyInfo(Variant::INT, ps_jpg_quality_name, PROPERTY_HINT_RANGE, "0,100"));
	DEF_(ps_auto_adjust_scale_name, false, PropertyInfo(Variant::BOOL, ps_auto_adjust_scale_name));

#undef DEF_SET
#undef DEF_SET_ENUM
#undef DEF_
}

void GodotRemote::_create_notification_manager() {
	GRNotifications *notif = memnew(GRNotifications);
	SceneTree::get_singleton()->get_root()->call_deferred("add_child", notif);
	SceneTree::get_singleton()->get_root()->call_deferred("move_child", notif, 0);
}

void GodotRemote::_remove_notifications_manager() {
	GRNotificationPanel::clear_styles();
	if (GRNotifications::get_singleton() && !GRNotifications::get_singleton()->is_queued_for_deletion()) {
		SceneTree::get_singleton()->get_root()->remove_child(GRNotifications::get_singleton());
		memdelete(GRNotifications::get_singleton());
	}
}

void GodotRemote::create_and_start_device(DeviceType type) {
	create_remote_device(type);
	start_remote_device();
}

#ifdef TOOLS_ENABLED
// TODO need to try get every device IDs and setup forwarding for each
#include "editor/editor_export.h"
#include "editor/editor_settings.h"

void GodotRemote::_run_emitted() {
	// call_deferred because debugger can't connect to game if process blocks thread on start
	if (GET_PS(ps_config_adb_name))
		call_deferred("adb_port_forwarding");
}

void GodotRemote::_adb_port_forwarding() {
	List<String> args;
	args.push_back("reverse");
	args.push_back("tcp:" + str(GET_PS(ps_port_name)));
	args.push_back("tcp:" + str(GET_PS(ps_port_name)));

	String res;
	Error err = OS::get_singleton()->execute(EditorSettings::get_singleton()->get_setting("export/android/adb"), args, true, nullptr, &res, nullptr, true);
	if (err == OK) {
		_log("ADB result: \n" + res);
	}
}

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
void GodotRemote::set_notifications_position(int positon) const {
	GRNotifications::set_notifications_position(positon);
}
int GodotRemote::get_notifications_position() const {
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
void GodotRemote::add_notification_or_append_string(String title, String text, bool new_string) {
	GRNotifications::add_notification_or_append_string(title, text, new_string);
}
void GodotRemote::add_notification_or_update_line(String title, String id, String text, int icon, float duration_multiplier) const {
	GRNotifications::add_notification_or_update_line(title, id, text, icon, duration_multiplier);
}
void GodotRemote::add_notification(String title, String text, int icon, bool update_existing, float duration_multiplier) const {
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
void GodotRemote::set_log_level(int lvl) const {
	GRUtils::set_log_level((GRUtils::LogLevel)lvl);
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

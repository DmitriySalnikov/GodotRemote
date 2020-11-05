/* GodotRemote.h */
#pragma once

#include "GRNotifications.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#include "core/image.h"
#include "core/pool_vector.h"
#include "core/reference.h"

#else

#include <Godot.hpp>
#include <Array.hpp>
#include <PoolArrays.hpp>
#include <Node.hpp>
#include <Ref.hpp>
#include <String.hpp>
#include <Image.hpp>
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY
class GodotRemote : public Reference {
	GD_CLASS(GodotRemote, Reference);
#else
class GodotRemote : public Node {
	GD_CLASS(GodotRemote, Node);
#endif

	friend class GRDevice;
	static GodotRemote *singleton;

public:
	enum DeviceType : int {
		DEVICE_Auto = 0,
		DEVICE_Development = 1,
		DEVICE_Standalone = 2,
	};

	static const char* ps_general_autoload_name;
	static const char* ps_general_port_name;
	static const char* ps_general_loglevel_name;

	static const char* ps_notifications_enabled_name;
	static const char* ps_noticications_position_name;
	static const char* ps_notifications_duration_name;

	static const char* ps_server_config_adb_name;
	static const char* ps_server_custom_input_scene_name;
	static const char* ps_server_custom_input_scene_compressed_name;
	static const char* ps_server_custom_input_scene_compression_type_name;
	static const char* ps_server_stream_skip_frames_name;
	static const char* ps_server_stream_enabled_name;
	static const char* ps_server_compression_type_name;
	static const char* ps_server_jpg_buffer_mb_size_name;
	static const char* ps_server_jpg_quality_name;
	static const char* ps_server_scale_of_sending_stream_name;
	static const char* ps_server_auto_adjust_scale_name;
	static const char* ps_server_password_name;

private:
	bool is_autostart = false;
	bool is_notifications_enabled = true;

	class GRDevice *device = nullptr;

	void register_and_load_settings();
#ifndef GDNATIVE_LIBRARY
#endif

	void _create_notification_manager();
	void _remove_notifications_manager();

#ifndef GDNATIVE_LIBRARY
#ifdef TOOLS_ENABLED
	int64_t adb_pid = 0;
	class Timer *adb_start_timer = nullptr;

	void _prepare_editor();
	void _run_emitted();
	void _adb_port_forwarding();
	void _adb_start_timer_timeout();
#endif
#endif

protected:

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();
	void _bind_constants();
protected:
#endif

	void _notification(int p_notification);

public:
	// GRNotifications
	class GRNotificationPanel *get_notification(String title) const;
	Array get_all_notifications() const;
	Array get_notifications_with_title(String title) const;

	void set_notifications_layer(int layer) const;
	int get_notifications_layer() const;

	void set_notifications_position(ENUM_ARG(NotificationsPosition) positon) const;
	NotificationsPosition get_notifications_position() const;

	void set_notifications_enabled(bool _enabled) const;
	bool get_notifications_enabled() const;

	void set_notifications_duration(float _duration) const;
	float get_notifications_duration() const;

	void set_notifications_style(Ref<class GRNotificationStyle> _style) const;
	Ref<class GRNotificationStyle> get_notifications_style() const;

	void add_notification_or_append_string(String title, String text, ENUM_ARG(NotificationIcon) icon, bool new_string DEF_ARG(= true), float duration_multiplier DEF_ARG(= 1.f));
	void add_notification_or_update_line(String title, String id, String text, ENUM_ARG(NotificationIcon) icon, float duration_multiplier DEF_ARG(= 1.f)) const;
	void add_notification(String title, String text, ENUM_ARG(NotificationIcon) icon, bool update_existing DEF_ARG(= true), float duration_multiplier DEF_ARG(= 1.f)) const;
	void remove_notification(String title, bool all_entries = true) const;
	void remove_notification_exact(Node *_notif) const;
	void clear_notifications() const;
	// GRNotifications end

	// GRUtils functions binds for GDScript
	void set_log_level(ENUM_ARG(LogLevel) lvl) const;
	void set_gravity(const Vector3 &p_gravity) const;
	void set_accelerometer(const Vector3 &p_accel) const;
	void set_magnetometer(const Vector3 &p_magnetometer) const;
	void set_gyroscope(const Vector3 &p_gyroscope) const;
	// GRUtils end

	class GRDevice *get_device() const;
	class String get_version() const;

	// must be call_deffered
	void create_and_start_device(ENUM_ARG(DeviceType) type DEF_ARG(= DeviceType::DEVICE_Auto));
	bool create_remote_device(ENUM_ARG(DeviceType) type DEF_ARG(= DeviceType::DEVICE_Auto));
	bool start_remote_device();
	bool remove_remote_device();

	static GodotRemote *get_singleton();
	void _init();
	void _deinit();
};

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(GodotRemote::DeviceType)
#endif
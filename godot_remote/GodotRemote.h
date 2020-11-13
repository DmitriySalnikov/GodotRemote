/* GodotRemote.h */
#pragma once

#include "GRNotifications.h"
#include "GRUtils.h"
#include "GRDevice.h"

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
		DEVICE_AUTO = 0,
		DEVICE_SERVER = 1,
		DEVICE_CLIENT = 2,
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

	CONST_FAKE_SET();

	// GodotRemote

	CONST_GET(GodotRemote, DeviceType, DEVICE_AUTO);
	CONST_GET(GodotRemote, DeviceType, DEVICE_SERVER);
	CONST_GET(GodotRemote, DeviceType, DEVICE_CLIENT);

	CONST_GET(GodotRemote, TypesOfServerSettings, USE_INTERNAL_SERVER_SETTINGS);
	CONST_GET(GodotRemote, TypesOfServerSettings, VIDEO_STREAM_ENABLED);
	CONST_GET(GodotRemote, TypesOfServerSettings, COMPRESSION_TYPE);
	CONST_GET(GodotRemote, TypesOfServerSettings, JPG_QUALITY);
	CONST_GET(GodotRemote, TypesOfServerSettings, SKIP_FRAMES);
	CONST_GET(GodotRemote, TypesOfServerSettings, RENDER_SCALE);

	CONST_GET(GodotRemote, NotificationIcon, None);
	CONST_GET(GodotRemote, NotificationIcon, _Error);
	CONST_GET(GodotRemote, NotificationIcon, Warning);
	CONST_GET(GodotRemote, NotificationIcon, Success);
	CONST_GET(GodotRemote, NotificationIcon, Fail);

	CONST_GET(GodotRemote, NotificationsPosition, TL);
	CONST_GET(GodotRemote, NotificationsPosition, TC);
	CONST_GET(GodotRemote, NotificationsPosition, TR);
	CONST_GET(GodotRemote, NotificationsPosition, BL);
	CONST_GET(GodotRemote, NotificationsPosition, BC);
	CONST_GET(GodotRemote, NotificationsPosition, BR);

	CONST_GET(GodotRemote, Subsampling, SUBSAMPLING_Y_ONLY);
	CONST_GET(GodotRemote, Subsampling, SUBSAMPLING_H1V1);
	CONST_GET(GodotRemote, Subsampling, SUBSAMPLING_H2V1);
	CONST_GET(GodotRemote, Subsampling, SUBSAMPLING_H2V2);

	CONST_GET(GodotRemote, LogLevel, LL_None);
	CONST_GET(GodotRemote, LogLevel, LL_Debug);
	CONST_GET(GodotRemote, LogLevel, LL_Normal);
	CONST_GET(GodotRemote, LogLevel, LL_Warning);
	CONST_GET(GodotRemote, LogLevel, LL_Error);

	CONST_GET(GodotRemote, ImageCompressionType, Uncompressed);
	CONST_GET(GodotRemote, ImageCompressionType, JPG);
	CONST_GET(GodotRemote, ImageCompressionType, PNG);

	// Other constants
	// GRDevice

	CONST_GET(GRDevice, WorkingStatus, Starting);
	CONST_GET(GRDevice, WorkingStatus, Stopped);
	CONST_GET(GRDevice, WorkingStatus, Stopping);
	CONST_GET(GRDevice, WorkingStatus, Working);
	
	CONST_GET(GRDevice, InputType, _NoneIT);
	CONST_GET(GRDevice, InputType, _InputDeviceSensors);
	CONST_GET(GRDevice, InputType, _InputEvent);
	CONST_GET(GRDevice, InputType, _InputEventAction);
	CONST_GET(GRDevice, InputType, _InputEventGesture);
	CONST_GET(GRDevice, InputType, _InputEventJoypadButton);
	CONST_GET(GRDevice, InputType, _InputEventJoypadMotion);
	CONST_GET(GRDevice, InputType, _InputEventKey);
	CONST_GET(GRDevice, InputType, _InputEventMagnifyGesture);
	CONST_GET(GRDevice, InputType, _InputEventMIDI);
	CONST_GET(GRDevice, InputType, _InputEventMouse);
	CONST_GET(GRDevice, InputType, _InputEventMouseButton);
	CONST_GET(GRDevice, InputType, _InputEventMouseMotion);
	CONST_GET(GRDevice, InputType, _InputEventPanGesture);
	CONST_GET(GRDevice, InputType, _InputEventScreenDrag);
	CONST_GET(GRDevice, InputType, _InputEventScreenTouch);
	CONST_GET(GRDevice, InputType, _InputEventWithModifiers);
	CONST_GET(GRDevice, InputType, _InputEventMAX);

	// GRClient

	CONST_GET(GRClient, ConnectionType, CONNECTION_ADB);
	CONST_GET(GRClient, ConnectionType, CONNECTION_WiFi);

	CONST_GET(GRClient, StretchMode, STRETCH_KEEP_ASPECT);
	CONST_GET(GRClient, StretchMode, STRETCH_FILL);

	CONST_GET(GRClient, StreamState, STREAM_NO_SIGNAL);
	CONST_GET(GRClient, StreamState, STREAM_ACTIVE);
	CONST_GET(GRClient, StreamState, STREAM_NO_IMAGE);

protected:
#endif

	void _notification(int p_notification);

public:
	// GRNotifications
	class GRNotificationPanel *get_notification(String title);
	Array get_all_notifications();
	Array get_notifications_with_title(String title);

	void set_notifications_layer(int layer);
	int get_notifications_layer();

	void set_notifications_position(ENUM_ARG(NotificationsPosition) positon);
	ENUM_ARG(NotificationsPosition) get_notifications_position();

	void set_notifications_enabled(bool _enabled);
	bool get_notifications_enabled();

	void set_notifications_duration(float _duration);
	float get_notifications_duration();

	void set_notifications_style(Ref<class GRNotificationStyle> _style);
	Ref<class GRNotificationStyle> get_notifications_style();

	void add_notification_or_append_string(String title, String text, ENUM_ARG(NotificationIcon) icon, bool new_string DEF_ARG(= true), float duration_multiplier DEF_ARG(= 1.f));
	void add_notification_or_update_line(String title, String id, String text, ENUM_ARG(NotificationIcon) icon, float duration_multiplier DEF_ARG(= 1.f));
	void add_notification(String title, String text, ENUM_ARG(NotificationIcon) icon, bool update_existing DEF_ARG(= true), float duration_multiplier DEF_ARG(= 1.f));
	void remove_notification(String title, bool all_entries = true);
	void remove_notification_exact(Node *_notif);
	void clear_notifications();
	// GRNotifications end

	// GRUtils functions binds for GDScript
	void set_log_level(ENUM_ARG(LogLevel) lvl);
	void set_gravity(const Vector3 &p_gravity);
	void set_accelerometer(const Vector3 &p_accel);
	void set_magnetometer(const Vector3 &p_magnetometer);
	void set_gyroscope(const Vector3 &p_gyroscope);
	// GRUtils end

	class GRDevice *get_device();
	class String get_version();

	// must be call_deffered
	void create_and_start_device(ENUM_ARG(DeviceType) type DEF_ARG(= DeviceType::DEVICE_AUTO));
	bool create_remote_device(ENUM_ARG(DeviceType) type DEF_ARG(= DeviceType::DEVICE_AUTO));
	bool start_remote_device();
	bool remove_remote_device();

	static GodotRemote *get_singleton();
	void _init();
	void _deinit();
};

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(GodotRemote::DeviceType)
#endif
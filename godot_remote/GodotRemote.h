/* GodotRemote.h */
#pragma once

#include "GRDevice.h"
#include "GRNotifications.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#include "core/image.h"
#include "core/pool_vector.h"
#include "core/reference.h"

#else
#include "GRClient.h"

#include <Array.hpp>
#include <Godot.hpp>
#include <Image.hpp>
#include <Node.hpp>
#include <PoolArrays.hpp>
#include <Ref.hpp>
#include <String.hpp>
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY
class GodotRemote : public Object {
	GD_CLASS(GodotRemote, Object);
#else
class GodotRemote : public Node {
	GD_CLASS(GodotRemote, Node);
#endif

	friend class GRDevice;
	static GodotRemote *singleton;
	static bool is_init_completed;

public:
#ifndef GDNATIVE_LIBRARY
#define GR_PS_NAME_TYPE String
#else
#define GR_PS_NAME_TYPE const char *
#endif

	enum DeviceType : int {
		DEVICE_AUTO = 0,
		DEVICE_SERVER = 1,
		DEVICE_CLIENT = 2,
	};

	enum LogLevel : int {
		LL_DEBUG = 0,
		LL_NORMAL = 1,
		LL_WARNING = 2,
		LL_ERROR = 3,
		LL_NONE,
	};

	static GR_PS_NAME_TYPE ps_general_autoload_name;
	static GR_PS_NAME_TYPE ps_general_port_name;
	static GR_PS_NAME_TYPE ps_general_loglevel_name;

	static GR_PS_NAME_TYPE ps_notifications_enabled_name;
	static GR_PS_NAME_TYPE ps_noticications_position_name;
	static GR_PS_NAME_TYPE ps_notifications_duration_name;

	static GR_PS_NAME_TYPE ps_server_config_adb_name;
	static GR_PS_NAME_TYPE ps_server_stream_skip_frames_name;
	static GR_PS_NAME_TYPE ps_server_stream_enabled_name;
	static GR_PS_NAME_TYPE ps_server_compression_type_name;
	static GR_PS_NAME_TYPE ps_server_jpg_quality_name;
	static GR_PS_NAME_TYPE ps_server_jpg_buffer_mb_size_name;
	static GR_PS_NAME_TYPE ps_server_auto_adjust_scale_name;
	static GR_PS_NAME_TYPE ps_server_scale_of_sending_stream_name;
	static GR_PS_NAME_TYPE ps_server_password_name;

	static GR_PS_NAME_TYPE ps_server_custom_input_scene_name;
	static GR_PS_NAME_TYPE ps_server_custom_input_scene_compressed_name;
	static GR_PS_NAME_TYPE ps_server_custom_input_scene_compression_type_name;

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

	CONST_GET(GodotRemote, LogLevel, LL_NONE);
	CONST_GET(GodotRemote, LogLevel, LL_DEBUG);
	CONST_GET(GodotRemote, LogLevel, LL_NORMAL);
	CONST_GET(GodotRemote, LogLevel, LL_WARNING);
	CONST_GET(GodotRemote, LogLevel, LL_ERROR);

	// GRNotivications
	CONST_GET(GRNotifications, NotificationIcon, ICON_NONE);
	CONST_GET(GRNotifications, NotificationIcon, ICON_ERROR);
	CONST_GET(GRNotifications, NotificationIcon, ICON_WARNING);
	CONST_GET(GRNotifications, NotificationIcon, ICON_SUCCESS);
	CONST_GET(GRNotifications, NotificationIcon, ICON_FAIL);

	CONST_GET(GRNotifications, NotificationsPosition, TOP_LEFT);
	CONST_GET(GRNotifications, NotificationsPosition, TOP_CENTER);
	CONST_GET(GRNotifications, NotificationsPosition, TOP_RIGHT);
	CONST_GET(GRNotifications, NotificationsPosition, BOTTOM_LEFT);
	CONST_GET(GRNotifications, NotificationsPosition, BOTTOM_CENTER);
	CONST_GET(GRNotifications, NotificationsPosition, BOTTOM_RIGHT);

	// GRInputData
	CONST_GET(GRInputData, InputType, _NoneIT);
	CONST_GET(GRInputData, InputType, _InputDeviceSensors);
	CONST_GET(GRInputData, InputType, _InputEvent);
	CONST_GET(GRInputData, InputType, _InputEventAction);
	CONST_GET(GRInputData, InputType, _InputEventGesture);
	CONST_GET(GRInputData, InputType, _InputEventJoypadButton);
	CONST_GET(GRInputData, InputType, _InputEventJoypadMotion);
	CONST_GET(GRInputData, InputType, _InputEventKey);
	CONST_GET(GRInputData, InputType, _InputEventMagnifyGesture);
	CONST_GET(GRInputData, InputType, _InputEventMIDI);
	CONST_GET(GRInputData, InputType, _InputEventMouse);
	CONST_GET(GRInputData, InputType, _InputEventMouseButton);
	CONST_GET(GRInputData, InputType, _InputEventMouseMotion);
	CONST_GET(GRInputData, InputType, _InputEventPanGesture);
	CONST_GET(GRInputData, InputType, _InputEventScreenDrag);
	CONST_GET(GRInputData, InputType, _InputEventScreenTouch);
	CONST_GET(GRInputData, InputType, _InputEventWithModifiers);
	CONST_GET(GRInputData, InputType, _InputEventMAX);

	// GRPacket
	CONST_GET(GRPacket, PacketType, NonePacket);
	CONST_GET(GRPacket, PacketType, SyncTime);
	CONST_GET(GRPacket, PacketType, ImageData);
	CONST_GET(GRPacket, PacketType, InputData);
	CONST_GET(GRPacket, PacketType, ServerSettings);
	CONST_GET(GRPacket, PacketType, MouseModeSync);
	CONST_GET(GRPacket, PacketType, CustomInputScene);
	CONST_GET(GRPacket, PacketType, ClientStreamOrientation);
	CONST_GET(GRPacket, PacketType, ClientStreamAspect);
	CONST_GET(GRPacket, PacketType, CustomUserData);
	CONST_GET(GRPacket, PacketType, Ping);
	CONST_GET(GRPacket, PacketType, Pong);
	
	// GRDevice
	CONST_GET(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_USE_INTERNAL);
	CONST_GET(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_VIDEO_STREAM_ENABLED);
	CONST_GET(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_COMPRESSION_TYPE);
	CONST_GET(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_JPG_QUALITY);
	CONST_GET(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_SKIP_FRAMES);
	CONST_GET(GRDevice, TypesOfServerSettings, SERVER_SETTINGS_RENDER_SCALE);

	CONST_GET(GRDevice, ImageCompressionType, COMPRESSION_UNCOMPRESSED);
	CONST_GET(GRDevice, ImageCompressionType, COMPRESSION_JPG);
	CONST_GET(GRDevice, ImageCompressionType, COMPRESSION_PNG);

	CONST_GET(GRDevice, Subsampling, SUBSAMPLING_Y_ONLY);
	CONST_GET(GRDevice, Subsampling, SUBSAMPLING_H1V1);
	CONST_GET(GRDevice, Subsampling, SUBSAMPLING_H2V1);
	CONST_GET(GRDevice, Subsampling, SUBSAMPLING_H2V2);

	CONST_GET(GRDevice, WorkingStatus, STATUS_STOPPED);
	CONST_GET(GRDevice, WorkingStatus, STATUS_WORKING);
	CONST_GET(GRDevice, WorkingStatus, STATUS_STOPPING);
	CONST_GET(GRDevice, WorkingStatus, STATUS_STARTING);

#ifndef NO_GODOTREMOTE_CLIENT
	// GRClient
	CONST_GET(GRClient, ConnectionType, CONNECTION_ADB);
	CONST_GET(GRClient, ConnectionType, CONNECTION_WiFi);

	CONST_GET(GRClient, StretchMode, STRETCH_KEEP_ASPECT);
	CONST_GET(GRClient, StretchMode, STRETCH_FILL);

	CONST_GET(GRClient, StreamState, STREAM_NO_SIGNAL);
	CONST_GET(GRClient, StreamState, STREAM_ACTIVE);
	CONST_GET(GRClient, StreamState, STREAM_NO_IMAGE);
#endif // !NO_GODOTREMOTE_CLIENT

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

	void set_notifications_position(ENUM_ARG(GRNotifications::NotificationsPosition) positon);
	ENUM_ARG(GRNotifications::NotificationsPosition) get_notifications_position();

	void set_notifications_enabled(bool _enabled);
	bool get_notifications_enabled();

	void set_notifications_duration(float _duration);
	float get_notifications_duration();

	void set_notifications_style(Ref<class GRNotificationStyle> _style);
	Ref<class GRNotificationStyle> get_notifications_style();

	void add_notification_or_append_string(String title, String text, ENUM_ARG(GRNotifications::NotificationIcon) icon, bool new_string DEF_ARG(= true), float duration_multiplier DEF_ARG(= 1.f));
	void add_notification_or_update_line(String title, String id, String text, ENUM_ARG(GRNotifications::NotificationIcon) icon, float duration_multiplier DEF_ARG(= 1.f));
	void add_notification(String title, String text, ENUM_ARG(GRNotifications::NotificationIcon) icon, bool update_existing DEF_ARG(= true), float duration_multiplier DEF_ARG(= 1.f));
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

	GRDevice *get_device();
	String get_version();
	bool is_gdnative();

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
VARIANT_ENUM_CAST(GodotRemote::LogLevel)
#endif

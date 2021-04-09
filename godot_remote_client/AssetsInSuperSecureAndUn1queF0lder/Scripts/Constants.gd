extends Node

var GodotRemote_DEVICE_AUTO : int
var GodotRemote_DEVICE_SERVER : int
var GodotRemote_DEVICE_CLIENT : int

var GodotRemote_LL_NONE : int
var GodotRemote_LL_DEBUG : int
var GodotRemote_LL_NORMAL : int
var GodotRemote_LL_WARNING : int
var GodotRemote_LL_ERROR : int

var GRNotifications_NOTIFICATION_ICON_NONE : int
var GRNotifications_NOTIFICATION_ICON_ERROR : int
var GRNotifications_NOTIFICATION_ICON_WARNING : int
var GRNotifications_NOTIFICATION_ICON_SUCCESS : int
var GRNotifications_NOTIFICATION_ICON_FAIL : int

var GRNotifications_NOTIFICATIONS_POSITION_TOP_LEFT : int
var GRNotifications_NOTIFICATIONS_POSITION_TOP_CENTER : int
var GRNotifications_NOTIFICATIONS_POSITION_TOP_RIGHT : int
var GRNotifications_NOTIFICATIONS_POSITION_BOTTOM_LEFT : int
var GRNotifications_NOTIFICATIONS_POSITION_BOTTOM_CENTER : int
var GRNotifications_NOTIFICATIONS_POSITION_BOTTOM_RIGHT : int

var GRDevice_STATUS_STARTING : int
var GRDevice_STATUS_STOPPING : int
var GRDevice_STATUS_WORKING : int
var GRDevice_STATUS_STOPPED : int

var GRDevice_USE_INTERNAL_SERVER_SETTINGS : int
var GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED : int
var GRDevice_SERVER_PARAM_COMPRESSION_TYPE : int
var GRDevice_SERVER_PARAM_STREAM_QUALITY : int
var GRDevice_SERVER_PARAM_SKIP_FRAMES : int
var GRDevice_SERVER_PARAM_TARGET_FPS : int
var GRDevice_SERVER_PARAM_RENDER_SCALE : int
var GRDevice_SERVER_SETTINGS_THREADS_NUMBER : int

var GRDevice_IMAGE_COMPRESSION_UNCOMPRESSED : int
var GRDevice_IMAGE_COMPRESSION_JPG : int
var GRDevice_IMAGE_COMPRESSION_PNG : int
var GRDevice_IMAGE_COMPRESSION_H264 : int

var GRClient_CONNECTION_ADB : int
var GRClient_CONNECTION_WiFi : int
var GRClient_CONNECTION_AUTO : int

var GRClient_STRETCH_KEEP_ASPECT : int
var GRClient_STRETCH_FILL : int

var GRClient_STREAM_NO_SIGNAL : int
var GRClient_STREAM_ACTIVE : int
var GRClient_STREAM_NO_IMAGE : int

func _enter_tree() -> void:
	_setup_constants()

func _setup_constants():
	GodotRemote_DEVICE_AUTO = get_enum_constant("GodotRemote", "DeviceType", "DEVICE_AUTO");
	GodotRemote_DEVICE_SERVER = get_enum_constant("GodotRemote", "DeviceType", "DEVICE_SERVER");
	GodotRemote_DEVICE_CLIENT = get_enum_constant("GodotRemote", "DeviceType", "DEVICE_CLIENT");

	GodotRemote_LL_NONE = get_enum_constant("GodotRemote", "LogLevel", "LL_NONE");
	GodotRemote_LL_DEBUG = get_enum_constant("GodotRemote", "LogLevel", "LL_DEBUG");
	GodotRemote_LL_NORMAL = get_enum_constant("GodotRemote", "LogLevel", "LL_NORMAL");
	GodotRemote_LL_WARNING = get_enum_constant("GodotRemote", "LogLevel", "LL_WARNING");
	GodotRemote_LL_ERROR = get_enum_constant("GodotRemote", "LogLevel", "LL_ERROR");

	GRNotifications_NOTIFICATION_ICON_NONE = get_enum_constant("GRNotifications", "NotificationIcon", "ICON_NONE");
	GRNotifications_NOTIFICATION_ICON_ERROR = get_enum_constant("GRNotifications", "NotificationIcon", "ICON_ERROR");
	GRNotifications_NOTIFICATION_ICON_WARNING = get_enum_constant("GRNotifications", "NotificationIcon", "ICON_WARNING");
	GRNotifications_NOTIFICATION_ICON_SUCCESS = get_enum_constant("GRNotifications", "NotificationIcon", "ICON_SUCCESS");
	GRNotifications_NOTIFICATION_ICON_FAIL = get_enum_constant("GRNotifications", "NotificationIcon", "ICON_FAIL");

	GRNotifications_NOTIFICATIONS_POSITION_TOP_LEFT = get_enum_constant("GRNotifications", "NotificationsPosition", "TOP_LEFT");
	GRNotifications_NOTIFICATIONS_POSITION_TOP_CENTER = get_enum_constant("GRNotifications", "NotificationsPosition", "TOP_CENTER");
	GRNotifications_NOTIFICATIONS_POSITION_TOP_RIGHT = get_enum_constant("GRNotifications", "NotificationsPosition", "TOP_RIGHT");
	GRNotifications_NOTIFICATIONS_POSITION_BOTTOM_LEFT = get_enum_constant("GRNotifications", "NotificationsPosition", "BOTTOM_LEFT");
	GRNotifications_NOTIFICATIONS_POSITION_BOTTOM_CENTER = get_enum_constant("GRNotifications", "NotificationsPosition", "BOTTOM_CENTER");
	GRNotifications_NOTIFICATIONS_POSITION_BOTTOM_RIGHT = get_enum_constant("GRNotifications", "NotificationsPosition", "BOTTOM_RIGHT");

	GRDevice_USE_INTERNAL_SERVER_SETTINGS = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_USE_INTERNAL");
	GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_VIDEO_STREAM_ENABLED");
	GRDevice_SERVER_PARAM_COMPRESSION_TYPE = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_COMPRESSION_TYPE");
	GRDevice_SERVER_PARAM_STREAM_QUALITY = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_STREAM_QUALITY");
	GRDevice_SERVER_PARAM_SKIP_FRAMES = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_SKIP_FRAMES");
	GRDevice_SERVER_PARAM_RENDER_SCALE = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_RENDER_SCALE");
	GRDevice_SERVER_PARAM_TARGET_FPS = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_TARGET_FPS");
	GRDevice_SERVER_SETTINGS_THREADS_NUMBER = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_THREADS_NUMBER");

	GRDevice_IMAGE_COMPRESSION_JPG = get_enum_constant("GRDevice", "ImageCompressionType", "COMPRESSION_JPG");
	GRDevice_IMAGE_COMPRESSION_H264 = get_enum_constant("GRDevice", "ImageCompressionType", "COMPRESSION_H264");

	GRDevice_STATUS_STARTING = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_STARTING");
	GRDevice_STATUS_STOPPING = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_STOPPING");
	GRDevice_STATUS_WORKING = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_WORKING");
	GRDevice_STATUS_STOPPED = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_STOPPED");

	GRClient_CONNECTION_ADB = get_enum_constant("GRClient", "ConnectionType", "CONNECTION_ADB");
	GRClient_CONNECTION_WiFi = get_enum_constant("GRClient", "ConnectionType", "CONNECTION_WiFi");
	GRClient_CONNECTION_AUTO = get_enum_constant("GRClient", "ConnectionType", "CONNECTION_AUTO");

	GRClient_STRETCH_KEEP_ASPECT = get_enum_constant("GRClient", "StretchMode", "STRETCH_KEEP_ASPECT");
	GRClient_STRETCH_FILL = get_enum_constant("GRClient", "StretchMode", "STRETCH_FILL");

	GRClient_STREAM_NO_SIGNAL = get_enum_constant("GRClient", "StreamState", "STREAM_NO_SIGNAL");
	GRClient_STREAM_ACTIVE = get_enum_constant("GRClient", "StreamState", "STREAM_ACTIVE");
	GRClient_STREAM_NO_IMAGE = get_enum_constant("GRClient", "StreamState", "STREAM_NO_IMAGE");

func get_enum_constant(_class : String, _enum : String, _value : String) -> int:
	if GodotRemote.is_gdnative():
		var name = "_get_%s_%s_%s"%[_class, _enum, _value]
		if GodotRemote.has_method(name):
			return int(GodotRemote.call(name))
	else:
		if ClassDB.class_has_integer_constant(_class, _value):
			return ClassDB.class_get_integer_constant(_class, _value)
	
	print("'%s' constant not found in class '%s'" % [_value, _class])
	return 0

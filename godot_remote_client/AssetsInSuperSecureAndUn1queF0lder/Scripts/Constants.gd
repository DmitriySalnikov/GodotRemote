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

var GRInputData_InputTypeNone : int
var GRInputData_InputDeviceSensors : int
var GRInputData_InputEvent : int
var GRInputData_InputEventAction : int
var GRInputData_InputEventGesture : int
var GRInputData_InputEventJoypadButton : int
var GRInputData_InputEventJoypadMotion : int
var GRInputData_InputEventKey : int
var GRInputData_InputEventMagnifyGesture : int
var GRInputData_InputEventMIDI : int
var GRInputData_InputEventMouse : int
var GRInputData_InputEventMouseButton : int
var GRInputData_InputEventMouseMotion : int
var GRInputData_InputEventPanGesture : int
var GRInputData_InputEventScreenDrag : int
var GRInputData_InputEventScreenTouch : int
var GRInputData_InputEventWithModifiers : int
var GRInputData_InputEventMAX : int

var GRPacket_NonePacket : int
var GRPacket_SyncTime : int
var GRPacket_ImageData : int
var GRPacket_InputData : int
var GRPacket_ServerSettings : int
var GRPacket_MouseModeSync : int
var GRPacket_CustomInputScene : int
var GRPacket_ClientStreamOrientation : int
var GRPacket_ClientStreamAspect : int
var GRPacket_CustomUserData : int
var GRPacket_Ping : int
var GRPacket_Pong : int

var GRDevice_STATUS_STARTING : int
var GRDevice_STATUS_STOPPING : int
var GRDevice_STATUS_WORKING : int
var GRDevice_STATUS_STOPPED : int

var GRDevice_SS_Y_ONLY : int
var GRDevice_SS_H1V1 : int
var GRDevice_SS_H2V1 : int
var GRDevice_SS_H2V2 : int

var GRDevice_USE_INTERNAL_SERVER_SETTINGS : int
var GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED : int
var GRDevice_SERVER_PARAM_COMPRESSION_TYPE : int
var GRDevice_SERVER_PARAM_JPG_QUALITY : int
var GRDevice_SERVER_PARAM_SKIP_FRAMES : int
var GRDevice_SERVER_PARAM_RENDER_SCALE : int

var GRDevice_IMAGE_COMPRESSION_UNCOMPRESSED : int
var GRDevice_IMAGE_COMPRESSION_JPG : int
var GRDevice_IMAGE_COMPRESSION_PNG : int

var GRClient_CONNECTION_ADB : int
var GRClient_CONNECTION_WiFi : int

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

	GRInputData_InputTypeNone = get_enum_constant("GRInputData", "InputType", "_NoneIT");
	GRInputData_InputDeviceSensors = get_enum_constant("GRInputData", "InputType", "_InputDeviceSensors");
	GRInputData_InputEvent = get_enum_constant("GRInputData", "InputType", "_InputEvent");
	GRInputData_InputEventAction = get_enum_constant("GRInputData", "InputType", "_InputEventAction");
	GRInputData_InputEventGesture = get_enum_constant("GRInputData", "InputType", "_InputEventGesture");
	GRInputData_InputEventJoypadButton = get_enum_constant("GRInputData", "InputType", "_InputEventJoypadButton");
	GRInputData_InputEventJoypadMotion = get_enum_constant("GRInputData", "InputType", "_InputEventJoypadMotion");
	GRInputData_InputEventKey = get_enum_constant("GRInputData", "InputType", "_InputEventKey");
	GRInputData_InputEventMagnifyGesture = get_enum_constant("GRInputData", "InputType", "_InputEventMagnifyGesture");
	GRInputData_InputEventMIDI = get_enum_constant("GRInputData", "InputType", "_InputEventMIDI");
	GRInputData_InputEventMouse = get_enum_constant("GRInputData", "InputType", "_InputEventMouse");
	GRInputData_InputEventMouseButton = get_enum_constant("GRInputData", "InputType", "_InputEventMouseButton");
	GRInputData_InputEventMouseMotion = get_enum_constant("GRInputData", "InputType", "_InputEventMouseMotion");
	GRInputData_InputEventPanGesture = get_enum_constant("GRInputData", "InputType", "_InputEventPanGesture");
	GRInputData_InputEventScreenDrag = get_enum_constant("GRInputData", "InputType", "_InputEventScreenDrag");
	GRInputData_InputEventScreenTouch = get_enum_constant("GRInputData", "InputType", "_InputEventScreenTouch");
	GRInputData_InputEventWithModifiers = get_enum_constant("GRInputData", "InputType", "_InputEventWithModifiers");
	GRInputData_InputEventMAX = get_enum_constant("GRInputData", "InputType", "_InputEventMAX");

	GRPacket_NonePacket = get_enum_constant("GRPacket", "PacketType", "NonePacket");
	GRPacket_SyncTime = get_enum_constant("GRPacket", "PacketType", "SyncTime");
	GRPacket_ImageData = get_enum_constant("GRPacket", "PacketType", "ImageData");
	GRPacket_InputData = get_enum_constant("GRPacket", "PacketType", "InputData");
	GRPacket_ServerSettings = get_enum_constant("GRPacket", "PacketType", "ServerSettings");
	GRPacket_MouseModeSync = get_enum_constant("GRPacket", "PacketType", "MouseModeSync");
	GRPacket_CustomInputScene = get_enum_constant("GRPacket", "PacketType", "CustomInputScene");
	GRPacket_ClientStreamOrientation = get_enum_constant("GRPacket", "PacketType", "ClientStreamOrientation");
	GRPacket_ClientStreamAspect = get_enum_constant("GRPacket", "PacketType", "ClientStreamAspect");
	GRPacket_CustomUserData = get_enum_constant("GRPacket", "PacketType", "CustomUserData");
	GRPacket_Ping = get_enum_constant("GRPacket", "PacketType", "Ping");
	GRPacket_Pong = get_enum_constant("GRPacket", "PacketType", "Pong");

	GRDevice_USE_INTERNAL_SERVER_SETTINGS = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_USE_INTERNAL");
	GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_VIDEO_STREAM_ENABLED");
	GRDevice_SERVER_PARAM_COMPRESSION_TYPE = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_COMPRESSION_TYPE");
	GRDevice_SERVER_PARAM_JPG_QUALITY = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_JPG_QUALITY");
	GRDevice_SERVER_PARAM_SKIP_FRAMES = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_SKIP_FRAMES");
	GRDevice_SERVER_PARAM_RENDER_SCALE = get_enum_constant("GRDevice", "TypesOfServerSettings", "SERVER_SETTINGS_RENDER_SCALE");

	GRDevice_IMAGE_COMPRESSION_UNCOMPRESSED = get_enum_constant("GRDevice", "ImageCompressionType", "COMPRESSION_UNCOMPRESSED");
	GRDevice_IMAGE_COMPRESSION_JPG = get_enum_constant("GRDevice", "ImageCompressionType", "COMPRESSION_JPG");
	GRDevice_IMAGE_COMPRESSION_PNG = get_enum_constant("GRDevice", "ImageCompressionType", "COMPRESSION_PNG");

	GRDevice_SS_Y_ONLY = get_enum_constant("GRDevice", "Subsampling", "SUBSAMPLING_Y_ONLY");
	GRDevice_SS_H1V1 = get_enum_constant("GRDevice", "Subsampling", "SUBSAMPLING_H1V1");
	GRDevice_SS_H2V1 = get_enum_constant("GRDevice", "Subsampling", "SUBSAMPLING_H2V1");
	GRDevice_SS_H2V2 = get_enum_constant("GRDevice", "Subsampling", "SUBSAMPLING_H2V2");

	GRDevice_STATUS_STARTING = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_STARTING");
	GRDevice_STATUS_STOPPING = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_STOPPING");
	GRDevice_STATUS_WORKING = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_WORKING");
	GRDevice_STATUS_STOPPED = get_enum_constant("GRDevice", "WorkingStatus", "STATUS_STOPPED");

	GRClient_CONNECTION_ADB = get_enum_constant("GRClient", "ConnectionType", "CONNECTION_ADB");
	GRClient_CONNECTION_WiFi = get_enum_constant("GRClient", "ConnectionType", "CONNECTION_WiFi");

	GRClient_STRETCH_KEEP_ASPECT = get_enum_constant("GRClient", "StretchMode", "STRETCH_KEEP_ASPECT");
	GRClient_STRETCH_FILL = get_enum_constant("GRClient", "StretchMode", "STRETCH_FILL");

	GRClient_STREAM_NO_SIGNAL = get_enum_constant("GRClient", "StreamState", "STREAM_NO_SIGNAL");
	GRClient_STREAM_ACTIVE = get_enum_constant("GRClient", "StreamState", "STREAM_ACTIVE");
	GRClient_STREAM_NO_IMAGE = get_enum_constant("GRClient", "StreamState", "STREAM_NO_IMAGE");

func get_enum_constant(_class : String, _enum : String, _value : String) -> int:
	if GodotRemote.is_gdnative():
		return int(GodotRemote.call("_get_%s_%s_%s"%[_class, _enum, _value]))
	else:
		return ClassDB.class_get_integer_constant(_class, _value)

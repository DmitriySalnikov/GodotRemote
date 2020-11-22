extends Node

var GodotRemote_DEVICE_AUTO : int
var GodotRemote_DEVICE_SERVER : int
var GodotRemote_DEVICE_CLIENT : int

var GodotRemote_USE_INTERNAL_SERVER_SETTINGS : int
var GodotRemote_SERVER_PARAM_VIDEO_STREAM_ENABLED : int
var GodotRemote_SERVER_PARAM_COMPRESSION_TYPE : int
var GodotRemote_SERVER_PARAM_JPG_QUALITY : int
var GodotRemote_SERVER_PARAM_SKIP_FRAMES : int
var GodotRemote_SERVER_PARAM_RENDER_SCALE : int

var GodotRemote_NOTIFICATION_ICON_NONE : int
var GodotRemote_NOTIFICATION_ICON_ERROR : int
var GodotRemote_NOTIFICATION_ICON_WARNING : int
var GodotRemote_NOTIFICATION_ICON_SUCCESS : int
var GodotRemote_NOTIFICATION_ICON_FAIL : int

var GodotRemote_NOTIFICATIONS_POSITION_TL : int
var GodotRemote_NOTIFICATIONS_POSITION_TC : int
var GodotRemote_NOTIFICATIONS_POSITION_TR : int
var GodotRemote_NOTIFICATIONS_POSITION_BL : int
var GodotRemote_NOTIFICATIONS_POSITION_BC : int
var GodotRemote_NOTIFICATIONS_POSITION_BR : int

var GodotRemote_SS_Y_ONLY : int
var GodotRemote_SS_H1V1 : int
var GodotRemote_SS_H2V1 : int
var GodotRemote_SS_H2V2 : int

var GodotRemote_LL_NONE : int
var GodotRemote_LL_DEBUG : int
var GodotRemote_LL_NORMAL : int
var GodotRemote_LL_WARNING : int
var GodotRemote_LL_ERROR : int

var GodotRemote_IMAGE_COMPRESSION_UNCOMPRESSED : int
var GodotRemote_IMAGE_COMPRESSION_JPG : int
var GodotRemote_IMAGE_COMPRESSION_PNG : int

var GRDevice_STATUS_STARTING : int
var GRDevice_STATUS_STOPPING : int
var GRDevice_STATUS_WORKING : int
var GRDevice_STATUS_STOPPED : int
var GRDevice_InputTypeNone : int
var GRDevice_InputDeviceSensors : int
var GRDevice_InputEvent : int
var GRDevice_InputEventAction : int
var GRDevice_InputEventGesture : int
var GRDevice_InputEventJoypadButton : int
var GRDevice_InputEventJoypadMotion : int
var GRDevice_InputEventKey : int
var GRDevice_InputEventMagnifyGesture : int
var GRDevice_InputEventMIDI : int
var GRDevice_InputEventMouse : int
var GRDevice_InputEventMouseButton : int
var GRDevice_InputEventMouseMotion : int
var GRDevice_InputEventPanGesture : int
var GRDevice_InputEventScreenDrag : int
var GRDevice_InputEventScreenTouch : int
var GRDevice_InputEventWithModifiers : int
var GRDevice_InputEventMAX : int

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
	if not GodotRemote.is_gdnative():
		
		GodotRemote_DEVICE_AUTO = GodotRemote.DEVICE_AUTO
		GodotRemote_DEVICE_SERVER = GodotRemote.DEVICE_SERVER
		GodotRemote_DEVICE_CLIENT = GodotRemote.DEVICE_CLIENT
		
		GodotRemote_USE_INTERNAL_SERVER_SETTINGS = GodotRemote.USE_INTERNAL_SERVER_SETTINGS
		GodotRemote_SERVER_PARAM_VIDEO_STREAM_ENABLED = GodotRemote.SERVER_PARAM_VIDEO_STREAM_ENABLED
		GodotRemote_SERVER_PARAM_COMPRESSION_TYPE = GodotRemote.SERVER_PARAM_COMPRESSION_TYPE
		GodotRemote_SERVER_PARAM_JPG_QUALITY = GodotRemote.SERVER_PARAM_JPG_QUALITY
		GodotRemote_SERVER_PARAM_SKIP_FRAMES = GodotRemote.SERVER_PARAM_SKIP_FRAMES
		GodotRemote_SERVER_PARAM_RENDER_SCALE = GodotRemote.SERVER_PARAM_RENDER_SCALE
		
		GodotRemote_NOTIFICATION_ICON_NONE = GodotRemote.NOTIFICATION_ICON_NONE
		GodotRemote_NOTIFICATION_ICON_ERROR = GodotRemote.NOTIFICATION_ICON_ERROR
		GodotRemote_NOTIFICATION_ICON_WARNING = GodotRemote.NOTIFICATION_ICON_WARNING
		GodotRemote_NOTIFICATION_ICON_SUCCESS = GodotRemote.NOTIFICATION_ICON_SUCCESS
		GodotRemote_NOTIFICATION_ICON_FAIL = GodotRemote.NOTIFICATION_ICON_FAIL
		
		GodotRemote_NOTIFICATIONS_POSITION_TL = GodotRemote.NOTIFICATIONS_POSITION_TL
		GodotRemote_NOTIFICATIONS_POSITION_TC = GodotRemote.NOTIFICATIONS_POSITION_TC
		GodotRemote_NOTIFICATIONS_POSITION_TR = GodotRemote.NOTIFICATIONS_POSITION_TR
		GodotRemote_NOTIFICATIONS_POSITION_BL = GodotRemote.NOTIFICATIONS_POSITION_BL
		GodotRemote_NOTIFICATIONS_POSITION_BC = GodotRemote.NOTIFICATIONS_POSITION_BC
		GodotRemote_NOTIFICATIONS_POSITION_BR = GodotRemote.NOTIFICATIONS_POSITION_BR
		
		GodotRemote_SS_Y_ONLY = GodotRemote.SS_Y_ONLY
		GodotRemote_SS_H1V1 = GodotRemote.SS_H1V1
		GodotRemote_SS_H2V1 = GodotRemote.SS_H2V1
		GodotRemote_SS_H2V2 = GodotRemote.SS_H2V2
		
		GodotRemote_LL_NONE = GodotRemote.LL_NONE
		GodotRemote_LL_DEBUG = GodotRemote.LL_DEBUG
		GodotRemote_LL_NORMAL = GodotRemote.LL_NORMAL
		GodotRemote_LL_WARNING = GodotRemote.LL_WARNING
		GodotRemote_LL_ERROR = GodotRemote.LL_ERROR
		
		GodotRemote_IMAGE_COMPRESSION_UNCOMPRESSED = GodotRemote.IMAGE_COMPRESSION_UNCOMPRESSED
		GodotRemote_IMAGE_COMPRESSION_JPG = GodotRemote.IMAGE_COMPRESSION_JPG
		GodotRemote_IMAGE_COMPRESSION_PNG = GodotRemote.IMAGE_COMPRESSION_PNG
		
		GRDevice_STATUS_STARTING = GRDevice.STATUS_STARTING
		GRDevice_STATUS_STOPPING = GRDevice.STATUS_STOPPING
		GRDevice_STATUS_WORKING = GRDevice.STATUS_WORKING
		GRDevice_STATUS_STOPPED = GRDevice.STATUS_STOPPED
		
		GRDevice_InputTypeNone = GRDevice.InputTypeNone
		GRDevice_InputDeviceSensors = GRDevice.InputDeviceSensors
		GRDevice_InputEvent = GRDevice.InputEvent
		GRDevice_InputEventAction = GRDevice.InputEventAction
		GRDevice_InputEventGesture = GRDevice.InputEventGesture
		GRDevice_InputEventJoypadButton = GRDevice.InputEventJoypadButton
		GRDevice_InputEventJoypadMotion = GRDevice.InputEventJoypadMotion
		GRDevice_InputEventKey = GRDevice.InputEventKey
		GRDevice_InputEventMagnifyGesture = GRDevice.InputEventMagnifyGesture
		GRDevice_InputEventMIDI = GRDevice.InputEventMIDI
		GRDevice_InputEventMouse = GRDevice.InputEventMouse
		GRDevice_InputEventMouseButton = GRDevice.InputEventMouseButton
		GRDevice_InputEventMouseMotion = GRDevice.InputEventMouseMotion
		GRDevice_InputEventPanGesture = GRDevice.InputEventPanGesture
		GRDevice_InputEventScreenDrag = GRDevice.InputEventScreenDrag
		GRDevice_InputEventScreenTouch = GRDevice.InputEventScreenTouch
		GRDevice_InputEventWithModifiers = GRDevice.InputEventWithModifiers
		GRDevice_InputEventMAX = GRDevice.InputEventMAX
		
		GRClient_CONNECTION_ADB = GRClient.CONNECTION_ADB
		GRClient_CONNECTION_WiFi = GRClient.CONNECTION_WiFi
		
		GRClient_STRETCH_KEEP_ASPECT = GRClient.STRETCH_KEEP_ASPECT
		GRClient_STRETCH_FILL = GRClient.STRETCH_FILL
		
		GRClient_STREAM_NO_SIGNAL = GRClient.STREAM_NO_SIGNAL
		GRClient_STREAM_ACTIVE = GRClient.STREAM_ACTIVE
		GRClient_STREAM_NO_IMAGE = GRClient.STREAM_NO_IMAGE
		
	else:
		
		GodotRemote_DEVICE_AUTO = GodotRemote.DEVICE_AUTO
		GodotRemote_DEVICE_SERVER = GodotRemote.DEVICE_SERVER
		GodotRemote_DEVICE_CLIENT = GodotRemote.DEVICE_CLIENT
		
		GodotRemote_USE_INTERNAL_SERVER_SETTINGS = GodotRemote.USE_INTERNAL_SERVER_SETTINGS
		GodotRemote_SERVER_PARAM_VIDEO_STREAM_ENABLED = GodotRemote.SERVER_PARAM_VIDEO_STREAM_ENABLED
		GodotRemote_SERVER_PARAM_COMPRESSION_TYPE = GodotRemote.SERVER_PARAM_COMPRESSION_TYPE
		GodotRemote_SERVER_PARAM_JPG_QUALITY = GodotRemote.SERVER_PARAM_JPG_QUALITY
		GodotRemote_SERVER_PARAM_SKIP_FRAMES = GodotRemote.SERVER_PARAM_SKIP_FRAMES
		GodotRemote_SERVER_PARAM_RENDER_SCALE = GodotRemote.SERVER_PARAM_RENDER_SCALE
		
		GodotRemote_NOTIFICATION_ICON_NONE = GodotRemote.NOTIFICATION_ICON_NONE
		GodotRemote_NOTIFICATION_ICON_ERROR = GodotRemote.NOTIFICATION_ICON_ERROR
		GodotRemote_NOTIFICATION_ICON_WARNING = GodotRemote.NOTIFICATION_ICON_WARNING
		GodotRemote_NOTIFICATION_ICON_SUCCESS = GodotRemote.NOTIFICATION_ICON_SUCCESS
		GodotRemote_NOTIFICATION_ICON_FAIL = GodotRemote.NOTIFICATION_ICON_FAIL
		
		GodotRemote_NOTIFICATIONS_POSITION_TL = GodotRemote.NOTIFICATIONS_POSITION_TL
		GodotRemote_NOTIFICATIONS_POSITION_TC = GodotRemote.NOTIFICATIONS_POSITION_TC
		GodotRemote_NOTIFICATIONS_POSITION_TR = GodotRemote.NOTIFICATIONS_POSITION_TR
		GodotRemote_NOTIFICATIONS_POSITION_BL = GodotRemote.NOTIFICATIONS_POSITION_BL
		GodotRemote_NOTIFICATIONS_POSITION_BC = GodotRemote.NOTIFICATIONS_POSITION_BC
		GodotRemote_NOTIFICATIONS_POSITION_BR = GodotRemote.NOTIFICATIONS_POSITION_BR
		
		GodotRemote_SS_Y_ONLY = GodotRemote.SS_Y_ONLY
		GodotRemote_SS_H1V1 = GodotRemote.SS_H1V1
		GodotRemote_SS_H2V1 = GodotRemote.SS_H2V1
		GodotRemote_SS_H2V2 = GodotRemote.SS_H2V2
		
		GodotRemote_LL_NONE = GodotRemote.LL_NONE
		GodotRemote_LL_DEBUG = GodotRemote.LL_DEBUG
		GodotRemote_LL_NORMAL = GodotRemote.LL_NORMAL
		GodotRemote_LL_WARNING = GodotRemote.LL_WARNING
		GodotRemote_LL_ERROR = GodotRemote.LL_ERROR
		
		GodotRemote_IMAGE_COMPRESSION_UNCOMPRESSED = GodotRemote.IMAGE_COMPRESSION_UNCOMPRESSED
		GodotRemote_IMAGE_COMPRESSION_JPG = GodotRemote.IMAGE_COMPRESSION_JPG
		GodotRemote_IMAGE_COMPRESSION_PNG = GodotRemote.IMAGE_COMPRESSION_PNG
		
		GRDevice_STATUS_STARTING = GodotRemote.GRDevice_STATUS_STARTING
		GRDevice_STATUS_STOPPING = GodotRemote.GRDevice_STATUS_STOPPING
		GRDevice_STATUS_WORKING = GodotRemote.GRDevice_STATUS_WORKING
		GRDevice_STATUS_STOPPED = GodotRemote.GRDevice_STATUS_STOPPED
		
		GRDevice_InputTypeNone = GodotRemote.GRDevice_InputTypeNone
		GRDevice_InputDeviceSensors = GodotRemote.GRDevice_InputDeviceSensors
		GRDevice_InputEvent = GodotRemote.GRDevice_InputEvent
		GRDevice_InputEventAction = GodotRemote.GRDevice_InputEventAction
		GRDevice_InputEventGesture = GodotRemote.GRDevice_InputEventGesture
		GRDevice_InputEventJoypadButton = GodotRemote.GRDevice_InputEventJoypadButton
		GRDevice_InputEventJoypadMotion = GodotRemote.GRDevice_InputEventJoypadMotion
		GRDevice_InputEventKey = GodotRemote.GRDevice_InputEventKey
		GRDevice_InputEventMagnifyGesture = GodotRemote.GRDevice_InputEventMagnifyGesture
		GRDevice_InputEventMIDI = GodotRemote.GRDevice_InputEventMIDI
		GRDevice_InputEventMouse = GodotRemote.GRDevice_InputEventMouse
		GRDevice_InputEventMouseButton = GodotRemote.GRDevice_InputEventMouseButton
		GRDevice_InputEventMouseMotion = GodotRemote.GRDevice_InputEventMouseMotion
		GRDevice_InputEventPanGesture = GodotRemote.GRDevice_InputEventPanGesture
		GRDevice_InputEventScreenDrag = GodotRemote.GRDevice_InputEventScreenDrag
		GRDevice_InputEventScreenTouch = GodotRemote.GRDevice_InputEventScreenTouch
		GRDevice_InputEventWithModifiers = GodotRemote.GRDevice_InputEventWithModifiers
		GRDevice_InputEventMAX = GodotRemote.GRDevice_InputEventMAX
		
		GRClient_CONNECTION_ADB = GodotRemote.GRClient_CONNECTION_ADB
		GRClient_CONNECTION_WiFi = GodotRemote.GRClient_CONNECTION_WiFi
		
		GRClient_STRETCH_KEEP_ASPECT = GodotRemote.GRClient_STRETCH_KEEP_ASPECT
		GRClient_STRETCH_FILL = GodotRemote.GRClient_STRETCH_FILL
		
		GRClient_STREAM_NO_SIGNAL = GodotRemote.GRClient_STREAM_NO_SIGNAL
		GRClient_STREAM_ACTIVE = GodotRemote.GRClient_STREAM_ACTIVE
		GRClient_STREAM_NO_IMAGE = GodotRemote.GRClient_STREAM_NO_IMAGE

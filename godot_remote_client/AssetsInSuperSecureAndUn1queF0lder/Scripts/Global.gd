extends Node

enum RateState{
	NotNow = 0,
	No = 1,
	Yes = 2,
}

enum StatInfoState{
	Hidden = 0,
	Simple = 1,
	Detailed = 2,
}

signal show_stats_changed(state)

const CLIENT_VERSION := 0
const SAVE_FILE := "user://settings.json"

var IsMobile : bool = false
var Billings : Node = null

var VersionChanged : bool = false
var PreviousVersion : String = ""
var AppRuns : int = 0
var TotalAppRuns : int = 0
var TouchesToOpenSettings : int = 5 setget set_touches_to_open_settings
var UserRateState : int = RateState.NotNow setget set_user_rate_state

var device_id : String = "" setget set_device_id
var connection_type : int = 0 setget set_con_type
var ip : String = "127.0.0.1" setget set_ip
var port : int = 52341 setget set_port
var stretch_mode : int = 0 setget set_stretch_mode
var target_send_fps : int = 60 setget set_target_send_fps
var texture_filtering : bool = true setget set_texture_filtering
var password : String = "" setget set_password
var sync_viewport_orientation : bool = true setget set_sync_viewport_orientation
var sync_viewport_aspect_ratio : bool = true setget set_sync_viewport_aspect_ratio
var keepscreenon : bool = false setget set_keep_screen_on
var capture_input_when_custom_scene : bool = false setget set_capture_input_when_custom_scene

var show_stats : int = StatInfoState.Hidden setget set_show_stats

var override_server_settings : bool = false setget set_override_settings
var sync_server_settings : bool = false setget set_sync_server_settings
var server_video_stream : bool = true setget set_server_video_stream
var server_compression_type : int = 1 setget set_server_compression_type
var server_jpg_quality : int = 80 setget set_server_jpg_quality
var server_render_scale : float = 0.3 setget set_server_render_scale
var server_skip_fps : int = 0 setget set_server_skip_fps

func get_random_hash(length : int = 6) -> String:
	return str(randi() * randi()).md5_text().substr(0,length)

func _enter_tree():
	IsMobile = OS.has_feature("mobile")
	#IsMobile = true
	
	var d = Directory.new()
	d.open("res://") # godot in some versions print error if derectory not opened
	var f = "res://An0therUn1queN@meOfF0lderForSecur1tyPurp0ses/AndroidBilling.gd"
	var fc = f+"c"
	if OS.has_feature("Android") and OS.has_feature("billing") and (d.file_exists(f) or d.file_exists(fc)):
		var b = load(f)
		if b:
			print("Android Billings Loaded")
			Billings = b.new()
			add_child(Billings)

func _ready():
	randomize()
	device_id = get_random_hash()
	
	if OS.has_feature("standalone") and not OS.has_feature("mobile"):
		OS.window_size = Vector2(1280, 720)
	
	_load_settings()
	
	GodotRemote.create_remote_device(C.GodotRemote_DEVICE_CLIENT)
	
	var d = GodotRemote.get_device()
	d.capture_when_hover = false
	
	_set_all_values()
	_setup_notifications_style()
	_add_runs()
	#GodotRemote.set_log_level(GodotRemote.LL_Debug)
	
	yield(get_tree(), "idle_frame")
	if Billings:
		Billings._init_billings()

func _add_runs():
	AppRuns += 1
	TotalAppRuns += 1
	_save_settings()

func _setup_notifications_style():
	yield(get_tree(), "idle_frame")
	var s = GodotRemote.notifications_style
	s.panel_style = load("res://AssetsInSuperSecureAndUn1queF0lder/Styles/NotificationPanelStyle.tres")
	s.close_button_theme = load("res://AssetsInSuperSecureAndUn1queF0lder/Styles/MainTheme.tres")
	s.title_font = load("res://AssetsInSuperSecureAndUn1queF0lder/Styles/NotificationPanelTitleStyle.tres")
	s.text_font = load("res://AssetsInSuperSecureAndUn1queF0lder/Styles/NotificationPanelTextStyle.tres")
	
	s.close_button_icon = load("res://AssetsInSuperSecureAndUn1queF0lder/Textures/Notifications/Close_icon.png")
	s.set_notification_icon(C.GRNotifications_NOTIFICATION_ICON_SUCCESS, load("res://AssetsInSuperSecureAndUn1queF0lder/Textures/Notifications/Connected_icon.png"))
	s.set_notification_icon(C.GRNotifications_NOTIFICATION_ICON_FAIL, load("res://AssetsInSuperSecureAndUn1queF0lder/Textures/Notifications/Disconnected_icon.png"))
	s.set_notification_icon(C.GRNotifications_NOTIFICATION_ICON_ERROR, load("res://AssetsInSuperSecureAndUn1queF0lder/Textures/Notifications/Error_icon.png"))
	s.set_notification_icon(C.GRNotifications_NOTIFICATION_ICON_WARNING, load("res://AssetsInSuperSecureAndUn1queF0lder/Textures/Notifications/Warning_icon.png"))
	
	GodotRemote.notifications_style = s

func _notification(what):
	match what:
		NOTIFICATION_WM_QUIT_REQUEST:
			_save_settings()

func _set_all_values():
	var dev = GodotRemote.get_device()
	var i_w : int = dev.get_status()
	if i_w == C.GRDevice_STATUS_WORKING:
		dev.stop()
	
	dev.device_id = device_id
	dev.set_address_port(ip, port)
	dev.connection_type = connection_type
	dev.stretch_mode = stretch_mode
	dev.target_send_fps = target_send_fps
	dev.texture_filtering = texture_filtering
	dev.password = password
	dev.server_settings_syncing = sync_server_settings
	dev.viewport_orientation_syncing = sync_viewport_orientation
	dev.viewport_aspect_ratio_syncing = sync_viewport_aspect_ratio
	OS.keep_screen_on = keepscreenon
	
	if override_server_settings:
		dev.set_server_setting(C.GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED, server_video_stream)
		dev.set_server_setting(C.GRDevice_SERVER_PARAM_COMPRESSION_TYPE, server_compression_type)
		dev.set_server_setting(C.GRDevice_SERVER_PARAM_JPG_QUALITY, server_jpg_quality)
		dev.set_server_setting(C.GRDevice_SERVER_PARAM_RENDER_SCALE, server_render_scale)
		dev.set_server_setting(C.GRDevice_SERVER_PARAM_SKIP_FRAMES, server_skip_fps)
	
	if i_w:
		dev.start()

func _save_settings():
	var d = Dictionary()
	
	d["m_version"] = get_version()
	d["app_runs"] = AppRuns
	d["total_app_runs"] = TotalAppRuns
	d["touches_to_open_settings"] = TouchesToOpenSettings
	d["user_rate_state"] = UserRateState
	d["device_id"] = device_id
	d["con_type"] = connection_type
	d["ip"] = ip
	d["port"] = port
	d["stretch"] = stretch_mode
	d["stats"] = show_stats
	d["target_fps"] = target_send_fps
	d["t_filter"] = texture_filtering
	d["password"] = password
	d["v_orient"] = sync_viewport_orientation
	d["v_aspect"] = sync_viewport_aspect_ratio
	d["keepscreenon"] = keepscreenon
	d["capture_input_when_custom_scene"] = capture_input_when_custom_scene
	
	d["ov_s_settings"] = override_server_settings
	d["sync_s_settings"] = sync_server_settings
	d["s_video_stream"] = server_video_stream
	d["s_jpg_quality"] = server_jpg_quality
	d["s_render_scale"] = server_render_scale
	d["s_skip_fps"] = server_skip_fps
	d["s_compression_type"] = server_compression_type
	
	var dir = Directory.new()
	
	dir.open("user://")
	if dir.file_exists(SAVE_FILE):
		var err = dir.remove(SAVE_FILE)
		if err != OK:
			return
	
	var f = File.new()
	
	var err = f.open(SAVE_FILE, File.WRITE)
	if err == OK:
		f.store_string(to_json(d))
		f.close()

func _load_settings():
	var f = File.new()
	
	if f.file_exists(SAVE_FILE):
		var err = f.open(SAVE_FILE, File.READ)
		
		if err == OK:
			var txt = f.get_as_text()
			f.close()
			var d = parse_json(txt)
			
			PreviousVersion = _safe_get_from_dict(d, "m_version", get_version()) 
			VersionChanged = PreviousVersion != get_version()
			if VersionChanged:
				AppRuns = 0
			else:
				AppRuns = _safe_get_from_dict(d, "app_runs", AppRuns)
			
			TotalAppRuns = _safe_get_from_dict(d, "total_app_runs", TotalAppRuns)
			TouchesToOpenSettings = _safe_get_from_dict(d, "touches_to_open_settings", TouchesToOpenSettings)
			UserRateState = _safe_get_from_dict(d, "user_rate_state", UserRateState)
			
			device_id = _safe_get_from_dict(d, "device_id", device_id)
			connection_type = _safe_get_from_dict(d, "con_type", connection_type)
			ip = _safe_get_from_dict(d, "ip", ip)
			port = _safe_get_from_dict(d, "port", port)
			stretch_mode = _safe_get_from_dict(d, "stretch", stretch_mode)
			show_stats = _safe_get_from_dict(d, "stats", show_stats)
			target_send_fps = _safe_get_from_dict(d, "target_fps", target_send_fps)
			texture_filtering = _safe_get_from_dict(d, "t_filter", texture_filtering)
			password = _safe_get_from_dict(d, "password", password)
			sync_viewport_orientation = _safe_get_from_dict(d, "v_orient", sync_viewport_orientation)
			sync_viewport_aspect_ratio = _safe_get_from_dict(d, "v_aspect", sync_viewport_aspect_ratio)
			keepscreenon = _safe_get_from_dict(d, "keepscreenon", keepscreenon)
			capture_input_when_custom_scene = _safe_get_from_dict(d, "capture_input_when_custom_scene", capture_input_when_custom_scene)
			
			override_server_settings = _safe_get_from_dict(d, "ov_s_settings", override_server_settings)
			sync_server_settings = _safe_get_from_dict(d, "sync_s_settings", sync_server_settings)
			server_video_stream = _safe_get_from_dict(d, "s_video_stream", server_video_stream)
			server_jpg_quality = _safe_get_from_dict(d, "s_jpg_quality", server_jpg_quality)
			server_render_scale = _safe_get_from_dict(d, "s_render_scale", server_render_scale)
			server_skip_fps = _safe_get_from_dict(d, "s_skip_fps", server_skip_fps)
			server_compression_type = _safe_get_from_dict(d, "s_compression_type", server_compression_type)

func _safe_get_from_dict(dict:Dictionary, val, def):
	if dict.has(val):
		return dict[val]
	return def

func get_version() -> String:
	return "%s.%d" % [GodotRemote.get_version(), CLIENT_VERSION]

func set_touches_to_open_settings(val : int):
	TouchesToOpenSettings = val
	_save_settings()

func set_user_rate_state(val : int):
	UserRateState = val
	_save_settings()

func set_device_id(val : String):
	device_id = val
	_save_settings()

func set_con_type(val : int):
	connection_type = val
	_save_settings()

func set_ip(val : String):
	ip = val
	_save_settings()

func set_port(val : int):
	port = val
	_save_settings()

func set_stretch_mode(val : int):
	stretch_mode = val
	_save_settings()

func set_show_stats(val : int):
	show_stats = val
	_save_settings()
	emit_signal("show_stats_changed", val)

func set_target_send_fps(val : int):
	target_send_fps = val
	_save_settings()

func set_texture_filtering(val : bool):
	texture_filtering = val
	_save_settings()

func set_password(val : String):
	password = val
	_save_settings()

func set_sync_viewport_orientation(val : bool):
	sync_viewport_orientation = val
	_save_settings()

func set_sync_viewport_aspect_ratio(val : bool):
	sync_viewport_aspect_ratio = val
	_save_settings()

func set_keep_screen_on(val : bool):
	keepscreenon = val
	_save_settings()

func set_capture_input_when_custom_scene(val : bool):
	capture_input_when_custom_scene = val
	_save_settings()

func set_override_settings(val : bool):
	override_server_settings = val
	_save_settings()

func set_sync_server_settings(val : bool):
	sync_server_settings = val
	_save_settings()

func set_server_video_stream(val : bool):
	server_video_stream = val
	_save_settings()

func set_server_compression_type(val : int):
	server_compression_type = val
	_save_settings()

func set_server_jpg_quality(val : int):
	server_jpg_quality = val
	_save_settings()

func set_server_render_scale(val : float):
	server_render_scale = val
	_save_settings()

func set_server_skip_fps(val : int):
	server_skip_fps = val
	_save_settings()

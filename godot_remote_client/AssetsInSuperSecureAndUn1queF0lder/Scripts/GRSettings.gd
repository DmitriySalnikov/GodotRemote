extends Control

signal stretch_mode_changed

var button_red_theme = preload("res://AssetsInSuperSecureAndUn1queF0lder/Styles/ButtonRed.tres")
var button_green_theme = preload("res://AssetsInSuperSecureAndUn1queF0lder/Styles/ButtonGreen.tres")

var game_open_counter := 0

onready var timer = $Timer
onready var start_stop = $V/H/StartStop
onready var version = $V/H/Version
onready var empty_top = $V/H/empty
onready var donations = $V/H/Donations

onready var scroll = $V/Scroll
onready var grid = $V/Scroll/H/Grid
onready var wifi = $V/Scroll/H/Grid/WiFi
onready var adb = $V/Scroll/H/Grid/ADB

onready var device_id = $V/Scroll/H/Grid/DeviceID/ID
onready var con_type_menu = $V/Scroll/H/Grid/ConnectionType/Type
onready var adb_port_line = $V/Scroll/H/Grid/ADB/Port

onready var wifi_port_line = $V/Scroll/H/Grid/WiFi/Address/Port
onready var wifi_ip_line = $V/Scroll/H/Grid/WiFi/Address/IP

onready var auto_line = $V/Scroll/H/Grid/Auto
onready var auto_status_icon = $V/Scroll/H/Grid/Auto/H/UDP_ListenerStatus
onready var auto_prev_addr = $V/Scroll/H/Grid/Auto/LastConnected
onready var auto_item_scroll = $V/Scroll/H/Grid/Auto/AvailableAddresses

onready var fps = $V/Scroll/H/Grid/OutFps/FPS
onready var password = $V/Scroll/H/Grid/PassRow/Pass
onready var filtering = $V/Scroll/H/Grid/Filtering
onready var stretch_mode = $V/Scroll/H/Grid/StretchMode/Type
onready var stats = $V/Scroll/H/Grid/ShowStats/Type
onready var sync_orient = $V/Scroll/H/Grid/SyncOrientation
onready var sync_aspect = $V/Scroll/H/Grid/SyncAspect
onready var decoder_threads = $V/Scroll/H/Grid/DecoderThreads/Type
onready var keepscreen = $V/Scroll/H/Grid/KeepScreen
onready var captureinput = $V/Scroll/H/Grid/CaptureInput
onready var touches_to_open = $V/Scroll/H/Grid/TouchesToOpenSettings/Type

onready var enabled_server_settings = $V/Scroll/H/Grid/OverrideServerSetting
onready var sync_server_settings = $V/Scroll/H/Grid/SyncServerSettings
onready var video_stream = $V/Scroll/H/Grid/VideoStream
onready var stream_quality = $V/Scroll/H/Grid/Quality/Quality
onready var quality_hint = $V/Scroll/H/Grid/Quality/HBox/QualityHint
onready var render_scale = $V/Scroll/H/Grid/RenderScale/Scale
onready var skip_frames = $V/Scroll/H/Grid/SkipFrames/SKIP
onready var scale_hint = $V/Scroll/H/Grid/RenderScale/HBox/ScaleHint
onready var target_server_fps = $V/Scroll/H/Grid/TargetFramerate/fps
onready var compression = $V/Scroll/H/Grid/CompressionType/State2
onready var encoder_threads = $V/Scroll/H/Grid/EncoderThreadsNumber/threads

# Names of LineEdits in custom touch input menu
onready var line_edits_to_touch_input = [
	[device_id, "Device ID:"],
	[adb_port_line.get_line_edit(), "Server port:"],
	[wifi_ip_line, "Server address:"],
	[wifi_port_line.get_line_edit(), "Server port:"],
	[fps.get_line_edit(), "Output data FPS:"],
	[password, "Server password:"],
	[target_server_fps.get_line_edit(), "Server target FPS:"],
	[encoder_threads.get_line_edit(), "Server encoder threads:"],
	]

var updated_by_code := false

func _ready():
	scroll.get_v_scrollbar().rect_min_size.x = 24
	scroll.get_h_scrollbar().rect_min_size.y = 24
	donations.visible = false
	quality_hint.visible = false
	_update_scale_hint_text(render_scale.value)
	#empty_top.visible = true
	
	version.text = "GR version: " + GodotRemote.get_version()
	var d = GodotRemote.get_device()
	auto_item_scroll.connect("selected_address", self, "_on_auto_list_address_changed")
	d.connect("auto_connection_listener_status_changed", self, "_on_auto_connection_status_changed")
	
	d.connect("connection_state_changed", self, "_connection_changed")
	d.connect("status_changed", self, "_status_changed")
	d.connect("server_settings_received", self, "_server_settings_received")
	d.connect("server_quality_hint_setting_received", self, "_update_quality_hint_text")
	update_values()
	_on_auto_list_address_changed()
	_set_buttons_disabled(false)
	_update_start_stop()
	_init_mobile_touch_input()
	_resize_for_mobile()
	_init_point()

func _resize_for_mobile():
	if G.IsMobile:
		var nodes = get_tree().get_nodes_in_group("nodes_that_should_be_higher")
		for n in nodes:
			if n is Control:
				n.rect_min_size = Vector2(n.rect_min_size.x, 64 if n.rect_min_size.y < 64 else n.rect_min_size.y)

func _init_mobile_touch_input():
	for p in line_edits_to_touch_input:
		TIM.set_title(p[0], p[1])

func _init_point():
	if G.Billings:
		yield(G.Billings, "billings_ready")
		donations.visible = G.Billings != null
		#empty_top.visible = G.Billings == null
		G.Billings.connect("points_updated", self, "_points_update")
		_points_update(G.Billings.get_purchased_points())

func _points_update(points):
	donations.text = "Your Points: " + str(points)

func _on_GRSettings_resized():
	var cols = 2 if (rect_size.x / rect_size.y > 1.37) else 1
	if cols != grid.columns:
		grid.columns = cols

func _server_settings_visibility(val : bool):
	var pos = enabled_server_settings.get_position_in_parent() + 1
	var childr = grid.get_children()
	
	for i in range(pos, childr.size()):
		var o = childr[i]
		if o is Control:
			o.visible = val

func _on_GRSettings_visibility_changed():
	_update_start_stop()
	if visible:
		update_values()
		grab_focus()
		
		yield(get_tree(), "idle_frame")
		GodotRemote.get_device().capture_input = false
	else:
		# yield needs to wait next frame and not instant close app
		yield(get_tree(), "idle_frame")
		GodotRemote.get_device().capture_input = true

func update_values():
	var d = GodotRemote.get_device()
	
	device_id.text = d.device_id
	con_type_menu.selected = con_type_menu.get_item_index(d.connection_type)
	adb_port_line.value = d.port
	wifi_port_line.value = d.port
	wifi_ip_line.text = d.get_address()
	stretch_mode.selected = d.stretch_mode
	fps.value = d.target_send_fps
	stats.selected = G.show_stats
	filtering.pressed = G.texture_filtering
	password.text = G.password
	sync_orient.pressed = G.sync_viewport_orientation
	sync_aspect.pressed = G.sync_viewport_aspect_ratio
	keepscreen.pressed = G.keepscreenon
	captureinput.pressed = G.capture_input_when_custom_scene
	touches_to_open.selected = G.TouchesToOpenSettings - 3
	decoder_threads.selected = G.decoder_threads_number - 1
	
	enabled_server_settings.pressed = G.override_server_settings
	sync_server_settings.pressed = G.sync_server_settings
	_server_settings_visibility(G.override_server_settings)
	
	stream_quality.value = G.server_quality
	render_scale.value = G.server_render_scale
	skip_frames.select(skip_frames.get_item_index(G.server_skip_fps))
	compression.select(compression.get_item_index(G.server_compression_type))
	video_stream.pressed = G.server_video_stream
	target_server_fps.value = G.server_target_fps
	encoder_threads.value = G.server_threads_number
	
	_set_all_server_settings()
	_on_con_Type_item_selected(con_type_menu.selected)

func _set_all_server_settings():
	var d = GodotRemote.get_device()
	if G.override_server_settings and d.is_connected_to_host():
		d.set_server_setting(C.GRDevice_SERVER_PARAM_STREAM_QUALITY, stream_quality.value)
		d.set_server_setting(C.GRDevice_SERVER_PARAM_RENDER_SCALE, render_scale.value)
		d.set_server_setting(C.GRDevice_SERVER_PARAM_SKIP_FRAMES, skip_frames.get_item_id(skip_frames.selected))
		d.set_server_setting(C.GRDevice_SERVER_PARAM_COMPRESSION_TYPE, compression.get_item_id(compression.selected))
		d.set_server_setting(C.GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED, video_stream.pressed)
		d.set_server_setting(C.GRDevice_SERVER_PARAM_TARGET_FPS, target_server_fps.value)
		d.set_server_setting(C.GRDevice_SERVER_SETTINGS_THREADS_NUMBER, encoder_threads.value)

func _status_changed(_status : int):
	if timer.is_stopped():
		_set_buttons_disabled(_status == C.GRDevice_STATUS_STARTING or _status == C.GRDevice_STATUS_STOPPING)
	_update_start_stop()

func _on_auto_list_address_changed():
	auto_prev_addr.text = "%s:%d" % [G.auto_ip, G.auto_port]

func _on_auto_connection_status_changed(is_listening):
	if is_listening:
		auto_status_icon.modulate = Color(0.439216, 0.819608, 0.360784)
	else:
		auto_status_icon.modulate = Color(0.819608, 0.360784, 0.360784)

func _connection_changed(connected : bool):
	if connected:
		game_open_counter = 0
		_set_all_server_settings()
	else:
		_game_scene_counter_increase()
		quality_hint.visible = false

func _game_scene_counter_increase():
	if not G.GameShowAfterConnectionErrors:
		return
	
	game_open_counter += 1
	if game_open_counter >= 5:
		game_open_counter = 0
		get_parent().create_game_scene(true)

func _update_quality_hint_text(hint_text):
	quality_hint.visible = true
	quality_hint.text = hint_text

func _update_scale_hint_text(scale : float):
	scale_hint.text = "%.0f %%" % (scale * 100)

func _server_settings_received(_settings : Dictionary):
	updated_by_code = true
	
	var k = _settings.keys()
	for kk in k:
		var v = _settings[kk]
		match kk:
			C.GRDevice_SERVER_PARAM_STREAM_QUALITY: stream_quality.value = v
			C.GRDevice_SERVER_PARAM_RENDER_SCALE: render_scale.value = v 
			C.GRDevice_SERVER_PARAM_SKIP_FRAMES: skip_frames.selected = skip_frames.get_item_index(v)
			C.GRDevice_SERVER_PARAM_COMPRESSION_TYPE: compression.selected = compression.get_item_index(v)
			C.GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED: video_stream.pressed = v
			C.GRDevice_SERVER_PARAM_TARGET_FPS: target_server_fps.value = v
			C.GRDevice_SERVER_SETTINGS_THREADS_NUMBER: encoder_threads.value = v
	
	updated_by_code = false

func _on_LogLevelPopupMenu_id_pressed(id: int) -> void:
	GodotRemote.set_log_level(id)

func _on_button_disable_Timer_timeout():
	var _status = GodotRemote.get_device().get_status()
	_set_buttons_disabled(_status == C.GRDevice_STATUS_STARTING or _status == C.GRDevice_STATUS_STOPPING)

func _update_start_stop():
	match GodotRemote.get_device().get_status():
		C.GRDevice_STATUS_STARTING: 
			start_stop.text = "   Starting Client   "
			start_stop.theme = button_green_theme
		C.GRDevice_STATUS_STOPPING: 
			start_stop.text = "   Stopping Client   "
			start_stop.theme = button_red_theme
		C.GRDevice_STATUS_WORKING: 
			start_stop.text = "     Stop Client     "
			start_stop.theme = button_green_theme
		C.GRDevice_STATUS_STOPPED: 
			start_stop.text = "    Launch Client    "
			start_stop.theme = button_red_theme

func _set_buttons_disabled(state : bool):
	if is_inside_tree():
		for b in get_tree().get_nodes_in_group("buttons_to_disable"):
			if b is Button:
				b.disabled = state

func _disable_buttons_by_timer():
	_set_buttons_disabled(true)
	timer.start()

func _on_Close_pressed():
	if get_parent().has_method("_hide_settings"):
		get_parent()._hide_settings()
	else:
		visible = false

func _on_Donations_pressed():
	get_parent().show_support_window()

func _on_Version_pressed():
	get_parent().popup_welcome_screen()

func _on_StartStop_pressed():
	var d = GodotRemote.get_device()
	
	_disable_buttons_by_timer()
	match d.get_status():
		C.GRDevice_STATUS_STARTING: pass
		C.GRDevice_STATUS_STOPPING: pass
		C.GRDevice_STATUS_WORKING: 
			_set_buttons_disabled(true)
			d.stop()
		C.GRDevice_STATUS_STOPPED: 
			_set_buttons_disabled(true)
			d.start()

func _on_Device_ID_text_changed(new_text : String):
	if !new_text.empty():
		GodotRemote.get_device().device_id = new_text
		G.device_id = new_text
	else:
		G.device_id = G.get_random_hash()
		GodotRemote.get_device().device_id = G.device_id

func _on_wifi_IP_text_entered(_new_text):
	_on_wifi_SetAddress_pressed()

func _on_wifi_SetAddress_pressed():
	_disable_buttons_by_timer()
	var id = con_type_menu.get_item_id(con_type_menu.selected)
	var address = wifi_ip_line.text.replace("http://", "").replace("https://", "").strip_edges()
	if GodotRemote.get_device().set_address_port(address, wifi_port_line.value):
		G.ip = address
		wifi_ip_line.text = address
		G.port = wifi_port_line.value

func _on_adb_SetAddress_pressed():
	_disable_buttons_by_timer()
	var id = con_type_menu.get_item_id(con_type_menu.selected)
	GodotRemote.get_device().port = adb_port_line.value
	G.port = adb_port_line.value

func _on_con_Type_item_selected(index):
	var id = con_type_menu.get_item_id(index)
	G.connection_type = id
	GodotRemote.get_device().connection_type = id
	
	match id:
		0:
			wifi.visible = true
			adb.visible = false
			auto_line.visible = false
		1:
			wifi.visible = false
			adb.visible = true
			auto_line.visible = false
		2:
			wifi.visible = false
			adb.visible = false
			auto_line.visible = true
			

func _on_stretch_Type_item_selected(index):
	GodotRemote.get_device().stretch_mode = index
	G.stretch_mode = index
	emit_signal("stretch_mode_changed")

func _on_stats_State_selected_id(id : int):
	G.show_stats = id

func _on_FPS_value_changed(value):
	GodotRemote.get_device().target_send_fps = value
	G.target_send_fps = value

func _on_texture_Filtering_toggled(button_pressed):
	G.texture_filtering = button_pressed
	GodotRemote.get_device().texture_filtering = button_pressed

func _on_Password_text_changed(new_text):
	G.password = new_text
	GodotRemote.get_device().password = new_text

func _on_SyncOrientation_toggled(button_pressed):
	G.sync_viewport_orientation = button_pressed
	GodotRemote.get_device().viewport_orientation_syncing = button_pressed

func _on_SyncAspect_toggled(button_pressed):
	G.sync_viewport_aspect_ratio = button_pressed
	GodotRemote.get_device().viewport_aspect_ratio_syncing = button_pressed

func _on_number_of_decoder_threads_item_selected(index: int) -> void:
	G.decoder_threads_number =  index + 1
	GodotRemote.get_device().set_decoder_threads_count(index + 1)

func _on_keep_screen_CheckButton_toggled(button_pressed):
	G.keepscreenon = button_pressed
	if !GodotRemote.get_device().is_stream_active():
		OS.keep_screen_on = button_pressed

func _on_CaptureInput_toggled(button_pressed: bool) -> void:
	G.capture_input_when_custom_scene = button_pressed
	if GodotRemote.get_device().get_custom_input_scene():
		GodotRemote.get_device().capture_pointer = button_pressed

func _on_touches_to_open_item_selected(index: int) -> void:
	G.TouchesToOpenSettings = index + 3

func _on_override_settings_State_toggled(button_pressed):
	G.override_server_settings = button_pressed
	_server_settings_visibility(G.override_server_settings)
	if button_pressed:
		_set_all_server_settings()
	else:
		GodotRemote.get_device().disable_overriding_server_settings()

func _on_SyncServerSettings_toggled(button_pressed):
	G.sync_server_settings = button_pressed
	GodotRemote.get_device().server_settings_syncing = button_pressed

func _on_server_Quality_value_changed(value):
	if not updated_by_code:
		if G.override_server_settings:
			GodotRemote.get_device().set_server_setting(C.GRDevice_SERVER_PARAM_STREAM_QUALITY, value)
	G.server_quality = value

func _on_server_render_Scale_value_changed(value):
	if not updated_by_code:
		if G.override_server_settings:
			GodotRemote.get_device().set_server_setting(C.GRDevice_SERVER_PARAM_RENDER_SCALE, value)
	G.server_render_scale = value
	_update_scale_hint_text(value)

func _on_server_skip_frames_value_changed(value):
	var id = skip_frames.get_item_id(value)
	if not updated_by_code:
		if G.override_server_settings:
			GodotRemote.get_device().set_server_setting(C.GRDevice_SERVER_PARAM_SKIP_FRAMES, id)
	G.server_skip_fps = id

func _on_compression_type_item_selected(index):
	var id = compression.get_item_id(index)
	if not updated_by_code:
		if G.override_server_settings:
			GodotRemote.get_device().set_server_setting(C.GRDevice_SERVER_PARAM_COMPRESSION_TYPE, id)
	G.server_compression_type = id

func _on_video_stream_Enabled_toggled(button_pressed):
	if not updated_by_code:
		if G.override_server_settings:
			GodotRemote.get_device().set_server_setting(C.GRDevice_SERVER_PARAM_VIDEO_STREAM_ENABLED, button_pressed)
	G.server_video_stream = button_pressed

func _on_server_target_fps_value_changed(value: float) -> void:
	if not updated_by_code:
		if G.override_server_settings:
			GodotRemote.get_device().set_server_setting(C.GRDevice_SERVER_PARAM_TARGET_FPS, value)
	G.server_target_fps = int(value)

func _on_server_encoder_threads_value_changed(value: float) -> void:
	if not updated_by_code:
		if G.override_server_settings:
			GodotRemote.get_device().set_server_setting(C.GRDevice_SERVER_SETTINGS_THREADS_NUMBER, value)
	G.server_threads_number = int(value)

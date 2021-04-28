extends Control

onready var settings = $GRSettings
onready var stats = $Stats
onready var bg_touch_hint = $BackgroundTouchHint
onready var bg_touch_hint_tex = $BackgroundTouchHint/Panel/TextureRect
onready var Welcome_screen = $Welcome
onready var Changelog_screen = $Changelog
onready var no_signal_hint = $OpenSettingsWithoutSignal
onready var no_signal_hint_text = $OpenSettingsWithoutSignal/SettingsHint

var touches : Dictionary = {}
var mouse_mode : = Input.MOUSE_MODE_VISIBLE
var support : Control = null
var orig_hint_text : String = ""
var prev_stream_state
var game_scene : Node = null

func _enter_tree() -> void:
	G.GodotRemoteRootNode = self

func _ready():
	var d = Directory.new()
	d.open("res://") # godot in some versions print error if derectory not opened
	var f = "res://An0therUn1queN@meOfF0lderForSecur1tyPurp0ses/Support.tscn"
	if d.file_exists(f):
		support = load(f).instance()
		call_deferred("add_child", support)
	
	get_tree().set_quit_on_go_back(false)
	
	TIM.InputPopup = $CustomPopupTextInput
	
	GodotRemote.get_device().set_control_to_show_in($Stream)
	GodotRemote.get_device().connect("custom_input_scene_added", self, "_custom_input_scene_added")
	GodotRemote.get_device().connect("custom_input_scene_removed", self, "_custom_input_scene_removed")
	GodotRemote.get_device().connect("stream_state_changed", self, "_stream_state_changed")
	GodotRemote.get_device().connect("mouse_mode_changed", self, "_mouse_mode_changed")
	GodotRemote.get_device().connect("stream_aspect_ratio_changed", self, "_stream_aspect_ratio_changed")
	GodotRemote.get_device().connect("connection_state_changed", self, "_connections_state_changed")
	GodotRemote.connect("device_removed", self, "_device_removed")
	settings.connect("stretch_mode_changed", self, "_settings_stretch_mode_changed")
	
	stats.visible = false
	bg_touch_hint.visible = false
	_hide_settings()
	Welcome_screen.hide()
	
	GodotRemote.get_device().start()
	
	orig_hint_text = no_signal_hint_text.text
	
	G.connect("touches_to_open_settings_changed", self, "_touches_to_open_settings_changed")
	connect("item_rect_changed", self, "viewport_size_changed")
	_resize_for_mobile()
	_touches_to_open_settings_changed(G.TouchesToOpenSettings)
	if !G.FirstRunAgreementAccepted:
		popup_welcome_screen()
		G.a_progression_event(G.A_ProgressStatus.Start, "WelcomeScreen")

func _resize_for_mobile():
	#if G.IsMobile:
		var height = 82
		var nodes = get_tree().get_nodes_in_group("nodes_that_should_be_higher")
		for n in nodes:
			if n is Control:
				n.rect_min_size = Vector2(n.rect_min_size.x, height if n.rect_min_size.y < height else n.rect_min_size.y)
		
		nodes = get_tree().get_nodes_in_group("menus_that_should_be_higher")
		var tex = ImageTexture.new()
		var img = Image.new()
		img.create(1, height, false, Image.FORMAT_RGBA8)
		tex.create_from_image(img)
		
		for n in nodes:
			if n is OptionButton:
				#n.get_popup().add_constant_override("vseparation", height / 3)
				for i in range(n.get_item_count()):
					if not n.get_item_icon(i) or not n.get_item_icon(i).get_data() or n.get_item_icon(i).get_data().is_empty():
						n.set_item_icon(i, tex)

func _touches_to_open_settings_changed(count : int) -> void:
	no_signal_hint_text.text = orig_hint_text % count

func _settings_stretch_mode_changed() -> void:
	_stream_aspect_ratio_changed(GodotRemote.get_device().get_stream_aspect_ratio())

func popup_welcome_screen() -> void:
	Welcome_screen.show()

func viewport_size_changed() -> void:
#	if Welcome_screen.visible:
#		Welcome_screen.rect_size = get_viewport_rect().size
	_stream_aspect_ratio_changed(GodotRemote.get_device().get_stream_aspect_ratio())

func _mouse_mode_changed(_mode):
	mouse_mode = _mode
	
	if not settings.visible:
		Input.set_mouse_mode(mouse_mode)

func _stream_aspect_ratio_changed(_ratio):
	var safe_rect = GodotRemote.get_2d_safe_area(bg_touch_hint)
	
	if _ratio == 0 or GodotRemote.get_device().stretch_mode == C.GRClient_STRETCH_FILL:
		bg_touch_hint.rect_size = safe_rect.size
		bg_touch_hint.rect_position = safe_rect.position
	else:
		var s = Vector2(safe_rect.size.x, safe_rect.size.y / _ratio)
		if (_ratio >= (safe_rect.size.x / safe_rect.size.y)):
			s.x = safe_rect.size.x
			s.y = s.x / _ratio
			bg_touch_hint.rect_size = s
			bg_touch_hint.rect_position = Vector2(safe_rect.position.x, safe_rect.position.y + (safe_rect.size.y - bg_touch_hint.rect_size.y) / 2)
		else:
			s.y = safe_rect.size.y
			s.x = s.y * _ratio
			var a2 = s.x / s.y
			if s.x > safe_rect.size.x:
				s.x = safe_rect.size.x
				s.y = s.x * a2
			bg_touch_hint.rect_size = s
			bg_touch_hint.rect_position = Vector2((safe_rect.size.x - bg_touch_hint.rect_size.x) / 2 + safe_rect.position.x, safe_rect.position.y)
		
		if (bg_touch_hint_tex.rect_size.x < bg_touch_hint_tex.texture.get_width()) or (bg_touch_hint_tex.rect_size.y < bg_touch_hint_tex.texture.get_height()):
			bg_touch_hint_tex.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		else:
			bg_touch_hint_tex.stretch_mode = TextureRect.STRETCH_KEEP_CENTERED

func _custom_input_scene_added():
	bg_touch_hint_tex.visible = false
	GodotRemote.get_device().capture_pointer = G.capture_input_when_custom_scene

func _custom_input_scene_removed():
	bg_touch_hint_tex.visible = true
	GodotRemote.get_device().capture_pointer = true

func _connections_state_changed(state : bool):
	if state:
		if !G.FirstConnectionSuccessful:
			G.FirstConnectionSuccessful = true
			G.a_design_event("ConnectedFirstTime")

func _stream_state_changed(_is_connected):
	prev_stream_state = _is_connected
	match _is_connected:
		C.GRClient_STREAM_NO_SIGNAL:
			OS.keep_screen_on = G.keepscreenon
			stats.visible = false
			bg_touch_hint.visible = false
			no_signal_hint.visible = true && !settings.visible
			_stream_aspect_ratio_changed(0)
			
		C.GRClient_STREAM_ACTIVE:
			OS.keep_screen_on = true
			stats.visible = not settings.visible
			bg_touch_hint.visible = false
			no_signal_hint.visible = false
			
		C.GRClient_STREAM_NO_IMAGE:
			OS.keep_screen_on = true
			stats.visible = not settings.visible
			bg_touch_hint.visible = true
			bg_touch_hint_tex.visible = GodotRemote.get_device().get_custom_input_scene() == null
			no_signal_hint.visible = false
			_stream_aspect_ratio_changed(GodotRemote.get_device().get_stream_aspect_ratio())

func _device_removed():
	stats.visible = false
	bg_touch_hint.visible = true

func _toggle_settings():
	settings.update_values()
	if settings.visible:
		_hide_settings()
	else:
		_show_settings()

func _show_settings():
	settings.visible = true
	stats.visible = false
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
	no_signal_hint.visible = false

func _hide_settings():
	settings.visible = false
	stats.visible = GodotRemote.get_device().is_stream_active()
	Input.set_mouse_mode(mouse_mode)
	
	match prev_stream_state:
		C.GRClient_STREAM_NO_SIGNAL:
			no_signal_hint.visible = true
		C.GRClient_STREAM_ACTIVE:
			no_signal_hint.visible = false
		C.GRClient_STREAM_NO_IMAGE:
			no_signal_hint.visible = false

func _count_pressed_touches() -> int:
	var res = 0
	for k in touches:
		res += int(touches[k])
	return res

func _release_sceen_touches(count : int):
	for x in range(count):
		var ev = InputEventScreenTouch.new()
		ev.pressed = false
		ev.index = x
		Input.parse_input_event(ev)

func _notification(what):
	if game_scene:
		return
	
	match what:
		NOTIFICATION_WM_GO_BACK_REQUEST: 
			if TIM.IsVisible:
				TIM.hide()
			elif Changelog_screen.visible:
				Changelog_screen.hide()
			elif support.visible:
				support.hide()
			elif Welcome_screen.visible:
				if G.FirstRunAgreementAccepted:
					Welcome_screen.hide()
			elif settings.visible:
				_hide_settings()
			else:
				get_tree().quit(0)

func _input(e):
	if game_scene:
		return
	
	if e is InputEventKey:
		if e.pressed:
			match e.scancode:
				KEY_ESCAPE: 
					if TIM.IsVisible:
						TIM.hide()
					elif Changelog_screen.visible:
						Changelog_screen.hide()
					elif Welcome_screen.visible:
						Welcome_screen.hide()
						
						# auto accept on PC
						if !G.FirstRunAgreementAccepted:
							G.FirstRunAgreementAccepted = true
					else:
						_toggle_settings()
	
	if e is InputEventScreenTouch:
		touches[e.index] = e.pressed
		if e.pressed:
			var pressed_count = _count_pressed_touches()
			if pressed_count >= G.TouchesToOpenSettings:
				_show_settings()
				_release_sceen_touches(pressed_count)

func show_changelogs():
	$Changelog.show_logs()

func show_support_window():
	if support:
		support.visible = true

func _on_open_settings_pressed() -> void:
	_show_settings()

func _on_no_this_is_a_game_pressed() -> void:
	create_game_scene()

func create_game_scene(as_dino = false):
	game_scene = load("res://AssetsInSuperSecureAndUn1queF0lder/Game/THIS_IS_A_GAME.tscn").instance()
	game_scene.connect("tree_exiting", self, "_game_scene_exiting", [as_dino])
	game_scene.set_is_loading_after_error(as_dino)
	add_child(game_scene)
	
	if as_dino:
		G.a_design_event("Game:AsDino")
	else:
		G.a_design_event("Game:FromWelcome")

func _game_scene_exiting(as_dino : bool):
	game_scene = null
	
	if not as_dino:
		popup_welcome_screen()
		if !G.FirstRunAgreementAccepted:
			G.a_progression_event(G.A_ProgressStatus.Start, "WelcomeScreen")

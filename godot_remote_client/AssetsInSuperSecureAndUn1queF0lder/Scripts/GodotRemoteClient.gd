extends Control

var touches : Dictionary = {}
var mouse_mode : = Input.MOUSE_MODE_VISIBLE
var support : Control = null
var orig_welcome_text : String = ""

func _ready():
	var d = Directory.new()
	d.open("res://") # godot in some versions print error if derectory not opened
	var f = "res://An0therUn1queN@meOfF0lderForSecur1tyPurp0ses/Support.tscn"
	if d.file_exists(f):
		support = load(f).instance()
		call_deferred("add_child", support)
	
	TIM.InputPopup = $CustomPopupTextInput
	
	GodotRemote.get_device().set_control_to_show_in($Stream)
	GodotRemote.get_device().connect("custom_input_scene_added", self, "_custom_input_scene_added")
	GodotRemote.get_device().connect("custom_input_scene_removed", self, "_custom_input_scene_removed")
	GodotRemote.get_device().connect("stream_state_changed", self, "_stream_state_changed")
	GodotRemote.get_device().connect("mouse_mode_changed", self, "_mouse_mode_changed")
	GodotRemote.connect("device_removed", self, "_device_removed")
	
	$Stats.visible = false
	$BackgroundTouchHint.visible = false
	_hide_settings()
	
	GodotRemote.get_device().start()
	
	connect("item_rect_changed", self, "viewport_size_changed")
	orig_welcome_text = $FirstLaunchHint/VBox/Label2.text
	if G.TotalAppRuns == 1:
		popup_welcome_screen()

func popup_welcome_screen():
	$FirstLaunchHint/VBox/Label2.text = orig_welcome_text % G.TouchesToOpenSettings
	$FirstLaunchHint.popup_centered(get_viewport_rect().size)

func viewport_size_changed() -> void:
	if $FirstLaunchHint.visible:
		$FirstLaunchHint.rect_size = get_viewport_rect().size

func _mouse_mode_changed(_mode):
	mouse_mode = _mode
	
	if not $GRSettings.visible:
		Input.set_mouse_mode(mouse_mode)

func _custom_input_scene_added():
	$BackgroundTouchHint/Panel/TextureRect.visible = false
	GodotRemote.get_device().capture_pointer = G.capture_input_when_custom_scene

func _custom_input_scene_removed():
	$BackgroundTouchHint/Panel/TextureRect.visible = true
	GodotRemote.get_device().capture_pointer = true

func _stream_state_changed(_is_connected):
	match _is_connected:
		C.GRClient_STREAM_NO_SIGNAL:
			OS.keep_screen_on = G.keepscreenon
			$Stats.visible = false
			$BackgroundTouchHint.visible = false
			
		C.GRClient_STREAM_ACTIVE:
			OS.keep_screen_on = true
			$Stats.visible = not $GRSettings.visible
			$BackgroundTouchHint.visible = false
			
		C.GRClient_STREAM_NO_IMAGE:
			$Stats.visible = not $GRSettings.visible
			$BackgroundTouchHint.visible = true
			$BackgroundTouchHint/Panel/TextureRect.visible = GodotRemote.get_device().get_custom_input_scene() == null

func _device_removed():
	$Stats.visible = false
	$BackgroundTouchHint.visible = true

func _toggle_settings():
	$GRSettings.update_values()
	if $GRSettings.visible:
		_hide_settings()
	else:
		_show_settings()

func _show_settings():
	$GRSettings.visible = true
	$Stats.visible = false
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func _hide_settings():
	$GRSettings.visible = false
	$Stats.visible = GodotRemote.get_device().is_stream_active()
	Input.set_mouse_mode(mouse_mode)

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

func _input(e):
	if e is InputEventKey:
		if e.pressed:
			match e.scancode:
				KEY_F2: GodotRemote.set_log_level(C.GodotRemote_LL_NONE)
				KEY_F3: GodotRemote.set_log_level(C.GodotRemote_LL_NORMAL)
				KEY_F4: GodotRemote.set_log_level(C.GodotRemote_LL_DEBUG)
				KEY_ESCAPE: 
					if TIM.IsVisible:
						TIM.hide()
					else:
						_toggle_settings()
	
	if e is InputEventScreenTouch:
		touches[e.index] = e.pressed
		if e.pressed:
			var pressed_count = _count_pressed_touches()
			if pressed_count >= G.TouchesToOpenSettings:
				_show_settings()
				_release_sceen_touches(pressed_count)

func show_support_window():
	if support:
		support.visible = true

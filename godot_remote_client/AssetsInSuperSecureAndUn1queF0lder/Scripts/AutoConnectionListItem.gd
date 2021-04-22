extends Button

signal server_selected(ips, port, project_name, icon)

onready var tween : Tween = $Tween
onready var tweenColor : Tween = $TweenColor
onready var col_rect : ColorRect = $ColorRect

var server_ips : PoolStringArray
var server_server_uid : int
var server_port : int
var server_project_name : String

func _ready() -> void:
	modulate.a = 0
	disabled = true
	col_rect.visible = true
	col_rect.modulate = Color.transparent

func appear():
	var dur = 0.3
	tween.stop_all()
	tween.interpolate_property(self, "modulate", modulate, Color(1, 1, 1, 1), dur, Tween.TRANS_SINE, Tween.EASE_IN_OUT)
	tween.interpolate_callback(self, dur, "set_disabled", false)
	tween.start()

func _pressed() -> void:
	emit_signal("server_selected", server_ips, server_port, server_project_name)

func setup_params(server_uid : int, version : String, project_name : String, port : int, addresses : PoolStringArray, icon_img : ImageTexture, preview_img : Image):
	server_ips = addresses
	server_port = port
	server_project_name = project_name
	server_server_uid = server_uid
	
	$H/Icon.texture = icon_img
	$H/P/Preview.texture = G.create_tex_filtered(preview_img)
	
	$H/V/ProjectName.text = project_name
	$H/V/Port.text = str(port)
	$H/V/H/Version.text = version
	$H/V/H/Addresses.text = addresses.join(", ")

func delayed_destroy():
	remove_meta("server_uid")
	set_meta("delayed_destroy", true)
	disabled = true
	var dur = 0.3
	tween.stop_all()
	#tween.interpolate_property(self, "rect_size", rect_size, Vector2(0, rect_size.y), dur, Tween.TRANS_SINE, Tween.EASE_IN_OUT)
	tween.interpolate_property(self, "modulate", modulate, Color(1, 1, 1, 0), dur, Tween.TRANS_SINE, Tween.EASE_IN_OUT)
	tween.interpolate_callback(self, dur, "queue_free")
	tween.start()

var prev_color : Color = Color.transparent
func blink_with_color(col : Color):
	if prev_color == col:
		if tweenColor.is_active():
			return
	else:
		prev_color = col
	
	var times = 3
	var duration = 1.25
	var dur = duration / float(times)
	var col_t := Color(1,1,1,0)
	var col_c := Color(1,1,1,1)
	col_rect.color = col
	
	tweenColor.reset_all()
	tweenColor.stop_all()
	for i in range(times):
		tweenColor.interpolate_property(col_rect, "modulate", col_t, col_c, dur / 2.0, Tween.TRANS_CUBIC, Tween.EASE_IN_OUT, dur * i)
		tweenColor.interpolate_property(col_rect, "modulate", col_c, col_t, dur / 2.0, Tween.TRANS_CUBIC, Tween.EASE_IN_OUT, dur * i + dur / 2.0)
	tweenColor.start()

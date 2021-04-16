extends Button

signal server_selected(ips, port, project_name)

var server_ips : PoolStringArray
var server_server_uid : int
var server_port : int
var server_project_name : String

func _ready() -> void:
	modulate.a = 0
	disabled = true

func appear():
	var dur = 0.3
	$Tween.stop_all()
	$Tween.interpolate_property(self, "modulate", modulate, Color(1, 1, 1, 1), dur, Tween.TRANS_SINE, Tween.EASE_IN_OUT)
	$Tween.interpolate_callback(self, dur, "set_disabled", false)
	$Tween.start()

func _create_tex(img : Image):
	if img and not img.is_empty():
		var tex = ImageTexture.new()
		tex.create_from_image(img, Texture.FLAG_FILTER)
		return tex
	else:
		return null

func _pressed() -> void:
	emit_signal("server_selected", server_ips, server_port, server_project_name)

func setup_params(server_uid : int, version : String, project_name : String, port : int, addresses : PoolStringArray, icon_img : Image, preview_img : Image):
	server_ips = addresses
	server_port = port
	server_project_name = project_name
	server_server_uid = server_uid
	
	$H/Icon.texture = _create_tex(icon_img)
	$H/Preview.texture = _create_tex(preview_img)
	
	$H/V/ProjectName.text = project_name
	$H/V/Port.text = str(port)
	$H/V/H/Version.text = version
	$H/V/H/Addresses.text = addresses.join(", ")

func delayed_destroy():
	remove_meta("server_uid")
	set_meta("delayed_destroy", true)
	disabled = true
	var dur = 0.3
	$Tween.stop_all()
	#$Tween.interpolate_property(self, "rect_size", rect_size, Vector2(0, rect_size.y), dur, Tween.TRANS_SINE, Tween.EASE_IN_OUT)
	$Tween.interpolate_property(self, "modulate", modulate, Color(1, 1, 1, 0), dur, Tween.TRANS_SINE, Tween.EASE_IN_OUT)
	$Tween.interpolate_callback(self, dur, "queue_free")
	$Tween.start()

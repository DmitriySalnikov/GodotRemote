extends Button

func _ready() -> void:
	modulate.a = 0
	disabled = true

func appear():
	var dur = 0.3
	$Tween.stop_all()
	$Tween.interpolate_property(self, "modulate", modulate, Color(1, 1, 1, 1), dur, Tween.TRANS_SINE, Tween.EASE_IN_OUT)
	$Tween.interpolate_callback(self, dur, "set_disabled", false)
	$Tween.start()

func setup_params(version : String, project_name : String, port : int, addresses : PoolStringArray, img : Image):
	if img and not img.is_empty():
		var tex = ImageTexture.new()
		tex.create_from_image(img)
		$H/Icon.texture = tex
	else:
		$H/Icon.texture = null
	
	$H/V/ProjectName.text = project_name
	$H/V/Port.text = str(port)
	$H/V2/Version.text = version
	$H/V/Addresses.text = addresses.join(", ")

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

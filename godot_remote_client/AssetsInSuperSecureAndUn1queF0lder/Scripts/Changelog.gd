extends WindowDialog

var changelog : Dictionary = {
"1.0.2" : 
"""Nothing has changed in the functionality.
API:
Most enums have been renamed and moved.
Exposed all classes in GDScript, but did not expose their methods.
""",
}

func _ready() -> void:
	if G.VersionChanged:
		var text = "Current version: %s\nPrevious version: %s\n\n" % [GodotRemote.get_version(), G.PreviousVersion]
		
		var prev = _get_version_sum(G.PreviousVersion.split("."))
		var curr = _get_version_sum(GodotRemote.get_version().split("."))
		
		if curr < prev:
			return
		
		for k in changelog.keys():
			var v = _get_version_sum(k.split("."))
			if v > prev and v <= curr:
				text += "[%s]\n%s\n" % [k, changelog[k]]
		
		$HBoxContainer/Control/ListOfChanges.text = text
		$HBoxContainer/HBoxContainer/Button2.visible = _check_need_update_server(G.PreviousVersion, GodotRemote.get_version())
		call_deferred("popup_centered")

func _get_version_sum(v : PoolStringArray) -> int:
	var major = int(v[0]) << 16
	var minor = int(v[1]) << 8
	var patch = int(v[2])
	
	return major + minor + patch

func _check_need_update_server(prev : String, current : String) -> bool:
	var p = prev.split(".")
	var c = current.split(".")
	return int(p[0]) != int(c[0]) || int(p[1]) != int(c[1])

func _on_Button_pressed() -> void:
	hide()

func _on_Button2_pressed() -> void:
	OS.shell_open("https://github.com/DmitriySalnikov/GodotRemote#download")
	hide()

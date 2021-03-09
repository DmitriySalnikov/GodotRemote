extends PopupPanel

var changelog : Dictionary = {
"1.0.2.0" : 
"""Client:
Added detailed ping and fps stats.
Added the ability to change the number of touches to open the settings.
Changed the 'Welcome' screen to be more clear. Now it can be reopened by clicking on 'Godot Remote' version in the settings.
Module:
Fixed multithreading issues.
Improved fps and ping counters.
Most enums have been renamed and moved.
Exposed all classes in GDScript, but did not expose their methods.
Custom input scenes now adding '.md5' files from '.import' folder.
""",
}

func _ready() -> void:
	if G.VersionChanged:
		var text = "Current version: %s\nPrevious version: %s\n\n" % [G.get_version(), G.PreviousVersion]
		
		var prev = _get_version_sum(G.PreviousVersion.split("."))
		var curr = _get_version_sum(G.get_version().split("."))
		
		if curr < prev:
			return
		
		var found_logs = false
		for k in changelog.keys():
			var v = _get_version_sum(k.split("."))
			if v > prev and v <= curr:
				text += "[%s]\n%s\n" % [k, changelog[k]]
				found_logs = true
		
		if not found_logs:
			text += "No changes were found between these versions."
		
		$HBoxContainer/Control/ListOfChanges.text = text
		$HBoxContainer/HBoxContainer/Button2.visible = _check_need_update_server(G.PreviousVersion, G.get_version())
		call_deferred("popup_centered_ratio", 1)
		get_parent().connect("item_rect_changed", self, "viewport_size_changed")

func viewport_size_changed() -> void:
	if visible:
		rect_size = get_viewport_rect().size

func _get_version_sum(v : PoolStringArray) -> int:
	var major = int(v[0]) << 32
	var minor = int(v[1]) << 24
	var patch = int(v[2]) << 16
	# 16 bits for mobile versions will be enough
	var mobile = 0
	if(v.size() > 3):
		mobile = int(v[3])
	
	return major + minor + patch + mobile

func _check_need_update_server(prev : String, current : String) -> bool:
	var p = prev.split(".")
	var c = current.split(".")
	return int(p[0]) != int(c[0]) || int(p[1]) != int(c[1])

func _on_Button_pressed() -> void:
	hide()

func _on_Button2_pressed() -> void:
	OS.shell_open("https://github.com/DmitriySalnikov/GodotRemote#download")
	hide()

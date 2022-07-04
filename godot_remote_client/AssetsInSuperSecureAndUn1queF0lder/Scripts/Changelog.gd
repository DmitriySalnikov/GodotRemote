extends Panel

const UPD_EDITOR = "force_update_editor"

# 1.0.2.3 means:
# 1 - major version of module
# 0 - minor version of module
# 2 - patch version of module
# 3 - version of this client

# also 'UPD_EDITOR : true' after "text" key can be used to force show download button

var changelog : Dictionary = {
"1.0.4.0" : 
{ "text" : """Module:
Updated OpenH264 to version 2.2.0
"""},
"1.0.3.4" : 
{ "text" : """Added debugging symbols to track native crashes
"""},
"1.0.3.3" : 
{ "text" : """Godot Engine updated to version 3.4.4
""", UPD_EDITOR : true},
"1.0.3.2" : 
{ "text" : """Godot Engine updated to version 3.4.2
""", UPD_EDITOR : true},
"1.0.3.1" : 
{ "text" : """Godot Engine updated to version 3.4.1
""", UPD_EDITOR : true},
"1.0.3.0" : 
{ "text" : """Client:
Now in the Google Play version, some points can be purchased forever.
Module:
Added the ability to disable sending mouse events (by default, this app sends emulated mouse events).
"""},
"1.0.2.2" : 
{ "text" : """Godot Engine updated to version 3.3.2
""", UPD_EDITOR : true},
"1.0.2.1" : 
{ "text" : """Godot Engine updated to version 3.3.1
""", UPD_EDITOR : true},
"1.0.2.0" : 
{ "text" : """Client:
The UI has become more adapted for mobile.
UI now respects screen safe area.
Added auto connection mode UI.
Added more stats.
Added the ability to change the number of touches to open the settings.
Changed the 'Welcome' screen to be more clear. It can be reopened by clicking on 'Godot Remote' version in the settings.
Added changelog window.
Added the ability to open settings when the broadcast is not active by clicking button.
Changed the text of the 'Connect' buttons.
The Start/Stop client button now changes color to visually indicate the client status.
Added built-in game :-)
Module:
Fixed multithreading issues.
Added auto connection mode with live preview and project icons.
Improved fps and ping counters. Added traffic counter and delay counter.
Improved notification logic.
Most enums have been renamed and moved.
Significantly improved JPG compression/decompression performance.
Added H.264 codec. Currently only in software mode.
Removed Uncompressed and PNG codecs.
Custom input scenes now adding '.md5' files from '.import' folder.
Added many optimizations to avoid memory allocation and improve performance.
Added 'tracy' profiler.
""", UPD_EDITOR : true },
"1.0.1.0" : 
{ "text" : """Client:
Added the ability to send user packages in custom input scenes
Added "Rate This App" window
Module:
The codebase has also been updated to match the usual Godot API and GDNative at the same time.
""", },
"1.0.0.0" : 
{ "text" : """First Release.
""", },
}

func _ready() -> void:
	hide()
	if G.VersionChanged:
		var prev = _get_version_sum(G.PreviousVersion.split("."))
		var curr = _get_version_sum(G.get_version().split("."))
		
		if curr < prev:
			return
		
		show_logs(prev)
#	get_parent().connect("item_rect_changed", self, "viewport_size_changed")

func show_logs(prev : int = 0):
	var curr = _get_version_sum(G.get_version().split("."))
	
	var text = ""
	if prev == 0:
		text += "Current version: %s\n\n" % [G.get_version()]
	else:
		text += "Current version: %s\nPrevious version: %s\n\n" % [G.get_version(), G.PreviousVersion]
	
	var force_update_editor : bool = false
	
	var found_logs = false
	for k in changelog.keys():
		var v = _get_version_sum(k.split("."))
		if v > prev and v <= curr:
			text += "[%s]\n%s\n" % [k, changelog[k]["text"]]
			found_logs = true
			
			if changelog[k].has(UPD_EDITOR):
				if changelog[k][UPD_EDITOR] == true:
					force_update_editor = true
	
	if not found_logs:
		text += "No changes were found between these versions."
	
	$HBoxContainer/Control/ListOfChanges.text = text
	$HBoxContainer/HBoxContainer/Button2.visible = prev == 0 or force_update_editor or _check_need_update_server(G.PreviousVersion, G.get_version())
	show()

#func viewport_size_changed() -> void:
#	if visible:
#		rect_size = get_viewport_rect().size

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
	OS.shell_open("https://dmitriysalnikov.itch.io/godot-remote")
	hide()

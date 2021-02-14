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
		
		for k in changelog.keys():
			var v = _get_version_sum(k.split("."))
			if v > prev and v <= curr:
				text += "[%s]\n%s\n" % [k, changelog[k]]
		
		$HBoxContainer/Control/ListOfChanges.text = text
		call_deferred("popup_centered")

func _get_version_sum(v : PoolStringArray) -> int:
	var major = int(v[0]) << 16
	var minor = int(v[1]) << 8
	var patch = int(v[2])
	
	return major + minor + patch

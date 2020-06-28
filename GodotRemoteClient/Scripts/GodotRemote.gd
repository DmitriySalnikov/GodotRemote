extends Control


func _ready():
	GodotRemote.start_remote_device(GodotRemote.DEVICE_Standalone)
	GodotRemote.get_device().set_control_to_show_in($Stream)

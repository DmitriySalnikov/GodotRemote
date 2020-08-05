extends Control

# If you want to disable capturing touch events or mouse pointer
# you can use this method
func _enter_tree():
	GodotRemote.get_device().set_capture_pointer(false)

# And after removing this scene you need to restore value
func _exit_tree():
	GodotRemote.get_device().set_capture_pointer(true)

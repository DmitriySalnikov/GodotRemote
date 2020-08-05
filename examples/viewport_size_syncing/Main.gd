extends Control

var is_vertical = false
var screen_aspect = OS.window_size.x / OS.window_size.y

func _ready():
	# Waiting for one frame until the device is created
	yield(get_tree(), "idle_frame")
	# Connect to server signals
	GodotRemote.get_device().connect("client_viewport_orientation_changed", self, "_screen_rotated")
	GodotRemote.get_device().connect("client_viewport_aspect_ratio_changed", self, "_screen_aspect_changed")

# Simple functions to resize window
func _screen_rotated(_is_vertical):
	is_vertical = _is_vertical
	if _is_vertical:
		OS.window_size = Vector2(600, 600 / screen_aspect)
	else:
		OS.window_size = Vector2(600 * screen_aspect, 600)

func _screen_aspect_changed(_aspect):
	screen_aspect = _aspect
	_screen_rotated(is_vertical)

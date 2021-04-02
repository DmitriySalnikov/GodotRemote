extends Node

enum UI_LAYOUTS{
	HIDDEN,
	PlAY_MENU,
	TAP_TO_START,
}

onready var player = $World/Player
onready var game_menu = $Control/WindowDialog
onready var tap_to_start = $Control/Label

onready var buttons_to_disable = [
	$Control/WindowDialog/PanelContainer/VBoxContainer/HBoxContainer/Play,
	$Control/WindowDialog/PanelContainer/VBoxContainer/HBoxContainer/Exit,
]

func _enter_tree() -> void:
	ProjectSettings.set_setting("display/window/handheld/orientation", OS.SCREEN_ORIENTATION_LANDSCAPE)
	$World/Floor.position = get_viewport().size * Vector2(0.5, 1)

func _exit_tree() -> void:
	ProjectSettings.set_setting("display/window/handheld/orientation", OS.SCREEN_ORIENTATION_SENSOR)

func _ready() -> void:
	player.connect("dead", self, "_switch_layout", [UI_LAYOUTS.PlAY_MENU])
	player.connect("started", self, "_switch_layout", [UI_LAYOUTS.HIDDEN])
	_switch_layout(UI_LAYOUTS.PlAY_MENU)

func _set_disabled_buttons(state):
	for b in buttons_to_disable:
		b.disabled = state

func _switch_layout(layout):
	match layout:
		UI_LAYOUTS.HIDDEN:
			game_menu.hide()
			tap_to_start.hide()
		UI_LAYOUTS.PlAY_MENU:
			game_menu.show()
			_set_disabled_buttons(false)
			tap_to_start.hide()
		UI_LAYOUTS.TAP_TO_START:
			game_menu.hide()
			tap_to_start.show()

func _on_Play_pressed() -> void:
	_set_disabled_buttons(true)
	
	yield(get_tree(), "idle_frame")
	_switch_layout(UI_LAYOUTS.TAP_TO_START)
	player.get_ready()

func _on_Exit_pressed() -> void:
	pass # Replace with function body.

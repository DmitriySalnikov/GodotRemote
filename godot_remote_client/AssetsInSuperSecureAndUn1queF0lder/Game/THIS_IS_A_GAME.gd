extends Node

enum UI_LAYOUTS{
	HIDDEN,
	PlAY_MENU,
	TAP_TO_START,
	GAMEPLAY_SCORE,
}

onready var player = $World/Player
onready var game_menu = $Control/WindowDialog
onready var tap_to_start = $Control/Label
onready var score = $Control/Score
onready var high_score = $Control/WindowDialog/PanelContainer/VBoxContainer/HighScore
onready var current_score = $Control/WindowDialog/PanelContainer/VBoxContainer/CurrentScore
onready var pipe_handler = $World/PipeHandler
onready var dont_show = $Control/WindowDialog/PanelContainer/VBoxContainer/DontShowOnReconnects

var is_loading_after_error = false

onready var layouts := [
	game_menu,
	tap_to_start,
	score,
]

onready var buttons_to_disable = [
	$Control/WindowDialog/PanelContainer/VBoxContainer/HBoxContainer/Play,
	$Control/WindowDialog/PanelContainer/VBoxContainer/HBoxContainer/Exit,
]

#func _enter_tree() -> void:
#	ProjectSettings.set_setting("display/window/handheld/orientation", OS.SCREEN_ORIENTATION_LANDSCAPE)
#
#func _exit_tree() -> void:
#	ProjectSettings.set_setting("display/window/handheld/orientation", OS.SCREEN_ORIENTATION_SENSOR)

func set_is_loading_after_error(state : bool):
	is_loading_after_error = state

func _ready() -> void:
	_center_floor()
	player.connect("dead", self, "_game_stopped")
	player.connect("started", self, "_game_started")
	player.connect("score_updated", self, "_score_updated")
	_update_scoreboard()
	_switch_layout(UI_LAYOUTS.PlAY_MENU)
	
	dont_show.visible = !G.GameShowAfterConnectionErrors or is_loading_after_error
	dont_show.pressed = !G.GameShowAfterConnectionErrors

func _center_floor():
	$World/Floor.position = $Control.rect_size * Vector2(0.5, 1)

func _score_updated(new_score):
	_update_score(new_score)

func _set_disabled_buttons(state):
	for b in buttons_to_disable:
		b.disabled = state

func _game_stopped():
	pipe_handler.stop()
	_update_scoreboard()
	
	yield(get_tree().create_timer(1), "timeout")
	_switch_layout(UI_LAYOUTS.PlAY_MENU)

func _game_started():
	_switch_layout(UI_LAYOUTS.GAMEPLAY_SCORE)
	pipe_handler.start()
	_update_score(0)

func _update_score(val : int):
	score.text = "Score: %d" % val

func _set_layout_visible(layout : Control):
	for l in layouts:
		l.visible = l == layout

func _update_scoreboard():
	current_score.text = "Score: %d" % player.score
	
	var new_high = false
	if player.score > G.GameHighScore:
		G.GameHighScore = player.score
		new_high = true
	
	high_score.text = ("New " if new_high else "") + "Highscore: %d" % G.GameHighScore

func _switch_layout(layout):
	match layout:
		UI_LAYOUTS.HIDDEN:
			_set_layout_visible(null)
		UI_LAYOUTS.PlAY_MENU:
			_set_layout_visible(game_menu)
			_set_disabled_buttons(false)
		UI_LAYOUTS.TAP_TO_START:
			_set_layout_visible(tap_to_start)
		UI_LAYOUTS.GAMEPLAY_SCORE:
			_set_layout_visible(score)

func _on_Play_pressed() -> void:
	_set_disabled_buttons(true)
	_center_floor()
	
	yield(get_tree(), "idle_frame")
	_switch_layout(UI_LAYOUTS.TAP_TO_START)
	player.get_ready()
	pipe_handler.update_bounds($Control.rect_size)
	pipe_handler.clear_pipes()

func _on_Exit_pressed() -> void:
	queue_free()

func _on_DontShowOnReconnects_toggled(button_pressed: bool) -> void:
	G.GameShowAfterConnectionErrors = !button_pressed

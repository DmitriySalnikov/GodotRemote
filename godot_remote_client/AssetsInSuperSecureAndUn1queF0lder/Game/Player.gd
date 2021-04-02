extends KinematicBody2D

signal dead
signal started
onready var tween = $Tween
onready var sprite = $CollisionShape2D/Sprite
var is_playing = false

func _ready() -> void:
	set_process_input(false)
	reset_game()

func reset_game():
	var vp_s = get_viewport_rect().size
	tween.stop_all()
	position = Vector2(vp_s.x / 5, vp_s.y / 2)
	sprite.position = Vector2.ZERO
	sprite.rotation_degrees = 0
	set_physics_process(false)

func get_ready():
	reset_game()
	set_process_input(true)

func start():
	is_playing = true
	set_physics_process(true)
	emit_signal("started")

func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.pressed and event.button_index == BUTTON_LEFT:
			if is_playing:
				print("JUMP")
			else:
				start()

func _physics_process(delta: float) -> void:
	var col = move_and_collide(Vector2.DOWN * 256 * delta, true)
	
	if col:
		set_physics_process(false)
		set_process_input(false)
		fall_tween()
		is_playing = false
		emit_signal("dead")

func fall_tween():
	tween.interpolate_property(sprite, "global_position", global_position, Vector2(position.x, get_viewport_rect().size.y - 24),
	0.6, Tween.TRANS_CUBIC, Tween.EASE_IN)
	tween.interpolate_property(sprite, "global_rotation_degrees", global_rotation_degrees, 90,
	0.25, Tween.TRANS_CUBIC, Tween.EASE_IN)
	tween.start()

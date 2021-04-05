extends KinematicBody2D

signal dead
signal started
signal score_updated(new_score)

export(int, 0, 128) var acceleration = 5
export(int, -1024, 1024) var max_velocity = 1024
export(int, 0, 1024) var jump_velocity = 512
onready var tween = $Tween
onready var sprite = $CollisionShape2D/Sprite
var is_playing = false
var score = 0
var velocity : float = 0
var viewport_size : Vector2

func _ready() -> void:
	set_process_input(false)
	reset_game()

func update_bounds(vp_size : Vector2):
	viewport_size = vp_size

func increment_score():
	score += 1
	emit_signal("score_updated", score)

func reset_game():
	var vp_s = get_viewport_rect().size
	tween.stop_all()
	score = 0
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
	set_process_input(false)
	emit_signal("started")

func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.pressed and event.button_index == BUTTON_LEFT:
			start()

func _physics_process(delta: float) -> void:
	velocity = clamp(velocity + acceleration, -max_velocity, max_velocity)
	if Input.is_mouse_button_pressed(BUTTON_LEFT):
		velocity = -jump_velocity
	
	var force_dead = false
	var rel_vec = Vector2.DOWN * velocity * delta
	if position.y + rel_vec.y < 0:
		position.y = 0
		rel_vec.y = 0
		velocity = 0
	elif position.y > viewport_size.y:
		force_dead = true
	
	sprite.rotation_degrees = (90 * pow((velocity + max_velocity) / float(max_velocity * 2), 2)) - 45
	
	var col = move_and_collide(rel_vec, true)
	
	if col or force_dead:
		set_physics_process(false)
		fall_tween()
		is_playing = false
		emit_signal("dead")

func fall_tween():
	var duration = 0.8
	var bounce_time_percent = 0.4
	var duration_without_bounce = duration - duration * bounce_time_percent
	var target_pos = get_viewport_rect().size.y - 24
	var bounce = false
	if target_pos - global_position.y > 128:
		target_pos -= 128
		bounce = true
	
	tween.interpolate_property(sprite, "global_position", global_position, Vector2(position.x, target_pos), duration_without_bounce if bounce else duration, Tween.TRANS_CUBIC, Tween.EASE_IN)
	if bounce:
		tween.interpolate_property(sprite, "global_position", Vector2(position.x, target_pos), Vector2(position.x, target_pos + 128), duration * bounce_time_percent, Tween.TRANS_BOUNCE, Tween.EASE_OUT, duration_without_bounce)
	tween.interpolate_property(sprite, "global_rotation_degrees", global_rotation_degrees, 90, 0.25, Tween.TRANS_CUBIC, Tween.EASE_IN)
	tween.start()

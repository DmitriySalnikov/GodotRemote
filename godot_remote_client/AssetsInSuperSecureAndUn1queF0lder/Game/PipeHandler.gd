extends Node2D

export var pipe_scene : PackedScene
export(int, 1, 2048) var max_dist_delta := 256
export(int, 1, 1024) var pipe_interval := 256 * 1.5
export(int, 0, 2048) var pipes_speed := 200
var spawn_x : float = 0
var viewport_size : Vector2
var last_pipe : Node2D = null
var moving_pipes : Array = []

func _ready() -> void:
	set_physics_process(false)

func update_bounds(vp_size : Vector2):
	spawn_x = vp_size.x + 128
	viewport_size = vp_size

func start():
	set_physics_process(true)

func stop():
	set_physics_process(false)

func _physics_process(delta: float) -> void:
	if last_pipe == null:
		var p : Node2D = pipe_scene.instance()
		p.position = Vector2(spawn_x, rand_range(p.half_height, viewport_size.y - p.half_height))
		p.connect("tree_exiting", self, "_pipe_destroying", [p])
		moving_pipes.append(p)
		last_pipe = p
		add_child(p)
	else:
		if (spawn_x - last_pipe.position.x) > pipe_interval:
			var p : Node2D = pipe_scene.instance()
			var pos = last_pipe.position.y
			p.position = Vector2(spawn_x, clamp(rand_range(p.half_height, viewport_size.y - p.half_height), pos - max_dist_delta, pos + max_dist_delta))
			p.connect("tree_exiting", self, "_pipe_destroying", [p])
			moving_pipes.append(p)
			last_pipe = p
			add_child(p)
	
	for p in moving_pipes:
		p.position.x -= pipes_speed * delta
		
	

var is_manual_destroying = false

func _pipe_destroying(p : Node2D):
	if is_manual_destroying:
		return
	
	if last_pipe == p:
		last_pipe = null
	
	var pos = moving_pipes.find(p)
	if pos != -1:
		moving_pipes.remove(pos)

func clear_pipes():
	is_manual_destroying = true
	for p in moving_pipes:
		p.queue_free()
	last_pipe = null
	
	is_manual_destroying = false

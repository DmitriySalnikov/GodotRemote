tool
extends StaticBody2D

export(int, 0, 512) var half_height = 192 setget _set_half_height
export(int, -512, 512) var offset = 64 setget _set_offset

func _ready() -> void:
	if not Engine.editor_hint:
		var cs = get_node_or_null("CollisionShape2D")
		if cs:
			var s1 = cs.shape
			if s1 is RectangleShape2D:
				if (cs.global_position.y + s1.extents.y) < 0:
					cs.queue_free()

func _set_offset(val : int):
	offset = val
	_set_half_height(half_height)

func _set_half_height(val : int):
	half_height = val
	var cs1 = get_node_or_null("CollisionShape2D")
	if cs1:
		var s1 = cs1.shape
		if s1 is RectangleShape2D:
			cs1.position = Vector2(offset, -s1.extents.y - half_height)
	
	var s2 = $CollisionShape2D2.shape
	if s2 is RectangleShape2D:
		$CollisionShape2D2.position = Vector2(offset, s2.extents.y + half_height)
	
	var s3 = $Area2D/CollisionShape2D.shape
	if s3 is RectangleShape2D:
		s3.extents.y = half_height
	
	$Area2D/CollisionShape2D.position = Vector2(offset + 32, 0)

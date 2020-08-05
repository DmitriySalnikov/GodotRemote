extends Node

signal visibility_changed(_visible)

var InputPopup : CustomPopupTextInput setget _set_input_popup

var IsVisible : bool = false

var _lines_titles : Dictionary = {}

func _enter_tree():
	if not Engine.editor_hint:
		get_tree().connect("node_added", self, "_node_added_to_scene")
		get_tree().connect("node_removed", self, "_node_removed_from_scene")

func hide():
	InputPopup.force_close = true

func set_title(line : LineEdit, title : String):
	if _lines_titles.has(line):
		_lines_titles[line] = title

func _node_added_to_scene(node : Node):
	if G.IsMobile:
		if node is LineEdit:
			if not node.is_in_group("IgnoreTouchInput"):
				_register_line_edit(node, "")

func _node_removed_from_scene(node : Node):
	if G.IsMobile:
		if node is LineEdit:
			if _lines_titles.has(node):
				_lines_titles.erase(node)

func _set_input_popup(val):
	InputPopup = val
	InputPopup.connect("visibility_changed", self, "_input_visibility_changed")

func _input_visibility_changed():
	emit_signal("visibility_changed", InputPopup.visible)
	IsVisible = InputPopup.visible

func _register_line_edit(line : LineEdit, title : String):
	if G.IsMobile:
		line.focus_mode = Control.FOCUS_NONE
	else:
		line.focus_mode = Control.FOCUS_CLICK
	#line.editable = false
	line.connect("gui_input", self, "_on_touch_line_edit_gui_input", [line])
	
	_lines_titles[line] = title

func _on_touch_line_edit_gui_input(event : InputEvent, line : LineEdit):
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT:
			if not event.pressed and Rect2(Vector2(), line.rect_size).intersects(Rect2(event.position, Vector2())):
				_show_touch_input(line)

func _show_touch_input(line : LineEdit):
	InputPopup.popup_text_edit(_lines_titles[line], line)

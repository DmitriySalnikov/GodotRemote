tool
class_name CustomPopupTextInput
extends Control

onready var label := $Box/Label
onready var line := $Box/LineEdit

var Title := "" setget _set_title
var IsSecret := false setget _set_is_secret
var Text := "" setget _set_text
var Placeholder := "" setget _set_placeholder
var MaxLength := 128 setget _set_max_length
var LineFont : Font setget _set_line_font

var LineEditToReturn : LineEdit

var is_ready := false
var is_shown := false
var force_close := false
var _is_mobile := false
var line_style : StyleBoxFlat

func _ready():
	is_ready = true
	_is_mobile = OS.has_feature("mobile") # G.IsMobile not valid for this cause PC dont have virtual keyboard like Android/iOS
	
	set_process(false)
	if not Engine.editor_hint:
		visible = false
		line_style = line.get_stylebox("normal")

func _process(_delta):
	if _is_mobile:
		var h := OS.get_virtual_keyboard_height()
		if h > 0 and not force_close:
			var vp_s := get_tree().root.get_visible_rect().size
			rect_size = vp_s - Vector2(0, h) * (vp_s / OS.window_size)
			
			if not is_shown:
				is_shown = true
				visible = true
		else:
			if is_shown or force_close:
				is_shown = false
				visible = false
				
				if LineEditToReturn:
					LineEditToReturn.text = line.text
					LineEditToReturn.emit_signal("text_changed", line.text)
					if LineEditToReturn.get_parent() is SpinBox:
						LineEditToReturn.emit_signal("text_entered", line.text)
				
				LineEditToReturn = null
				set_process(false)
	else:
		if force_close:
			is_shown = false
			visible = false
			
			if LineEditToReturn:
				LineEditToReturn.text = line.text
				LineEditToReturn.emit_signal("text_changed", line.text)
				if LineEditToReturn.get_parent() is SpinBox:
					LineEditToReturn.emit_signal("text_entered", line.text)
			
			LineEditToReturn = null
			set_process(false)

func popup_text_edit(title : String, line_to_return : LineEdit):
	LineEditToReturn = line_to_return
	self.Title = title
	self.IsSecret = line_to_return.secret
	self.Text = line_to_return.text
	self.Placeholder = line_to_return.placeholder_text
	self.LineFont = line_to_return.get_font("font")
	self.MaxLength = line_to_return.max_length
	
	line.caret_position = line.text.length()
	
	var f = get_font("font").get_height() 
	line.rect_min_size = Vector2(0, f * 2.5)
	line_style.content_margin_left = f * 0.75
	line_style.content_margin_right = -f * 0.75
	
	force_close = false
	is_shown = false
	
	if not _is_mobile:
		visible = true
	
	line.grab_focus()
	set_process(true)

func _set_is_secret(val : bool):
	if is_ready:
		IsSecret = val
		line.secret = val

func _set_title(val : String):
	if is_ready:
		Title = val
		label.text = val

func _set_text(val : String):
	if is_ready:
		Text = val
		line.text = val

func _set_placeholder(val : String):
	if is_ready:
		Placeholder = val
		line.placeholder_text = val

func _set_max_length(val : int):
	if is_ready:
		MaxLength = val
		line.max_length = val

func _set_line_font(val : Font):
	if is_ready:
		LineFont = val
		line.add_font_override("font", val)

func _on_LineEdit_text_changed(new_text):
	if LineEditToReturn:
		LineEditToReturn.text = new_text

func _on_LineEdit_text_entered(_new_text):
	force_close = true

func _on_LineEdit_focus_exited():
	force_close = true

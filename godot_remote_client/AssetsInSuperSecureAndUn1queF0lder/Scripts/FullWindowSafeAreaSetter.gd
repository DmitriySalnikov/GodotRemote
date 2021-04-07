extends Control

export(Array, Resource) var Objects : Array = []

func _ready() -> void:
	connect("resized", self, "_on_SafeArea_resized")
	
	for o in Objects:
		if o is FullWindowSafeAreaObject:
			var ControlToHandle : Control = get_node_or_null(o.ControlToHandle)
			if ControlToHandle:
				if o.IsRootObjectOfPanel:
					var parent = ControlToHandle.get_parent()
					parent.connect("visibility_changed", self, "_popup_panel_vis_changed", [ControlToHandle, o])
				else:
					ControlToHandle.connect("visibility_changed", self, "_popup_vis_changed", [ControlToHandle, o])
	
	_on_SafeArea_resized()

func _on_SafeArea_resized() -> void:
	yield(get_tree(), "idle_frame")
	for o in Objects:
		if o is FullWindowSafeAreaObject:
			var ControlToHandle : Control = get_node_or_null(o.ControlToHandle)
			if ControlToHandle:
				var res = G.get_margin_rect(rect_size, ControlToHandle, o.Left, o.Top, o.Right, o.Bottom)
				
				if not o.IsRootObjectOfPanel:
					call_deferred("_update_normal_control_margins", ControlToHandle, res, o)
				elif ControlToHandle.visible:
					call_deferred("_update_panel_style_control_margins", ControlToHandle, res, o)

func _popup_panel_vis_changed(ControlToHandle : Control, params : FullWindowSafeAreaObject):
	_update_panel_style_control_margins(ControlToHandle, G.get_margin_rect(rect_size, ControlToHandle, params.Left, params.Top, params.Right, params.Bottom), params)

func _popup_vis_changed(ControlToHandle : Control, params : FullWindowSafeAreaObject):
	_update_normal_control_margins(ControlToHandle, G.get_margin_rect(rect_size, ControlToHandle, params.Left, params.Top, params.Right, params.Bottom), params)

func _update_normal_control_margins(ControlToHandle : Control, margins : Rect2, params : FullWindowSafeAreaObject):
	ControlToHandle.set_anchor_and_margin(MARGIN_LEFT, 0, margins.position.x)
	ControlToHandle.set_anchor_and_margin(MARGIN_TOP, 0, margins.position.y)
	ControlToHandle.set_anchor_and_margin(MARGIN_RIGHT, 1, -margins.size.x)
	ControlToHandle.set_anchor_and_margin(MARGIN_BOTTOM, 1, -margins.size.y)
	if params.ChangeParentSizeToFullwindow:
		ControlToHandle.get_parent().rect_size = rect_size
		ControlToHandle.get_parent().rect_position = Vector2()

func _update_panel_style_control_margins(ControlToHandle : Control, margins : Rect2, params : FullWindowSafeAreaObject):
	var parent : Control = ControlToHandle.get_parent()
	var orig_box = parent.get_stylebox("panel") as StyleBoxFlat
	
	if orig_box:
		var box : StyleBoxFlat = orig_box.duplicate(true)
		box.set_default_margin(MARGIN_LEFT, margins.position.x)
		box.set_default_margin(MARGIN_TOP, margins.position.y)
		box.set_default_margin(MARGIN_RIGHT, margins.size.x)
		box.set_default_margin(MARGIN_BOTTOM, margins.size.y)
		parent.add_stylebox_override("panel", box)
		
		# FORCE MANUAL UPDATE RECT!!!!!!!!!!!!!!!!
		# https://github.com/godotengine/godot/issues/8670
		if params.ChangeParentSizeToFullwindow:
			parent.rect_size = rect_size
			parent.rect_position = Vector2()
		ControlToHandle._set_position(margins.position)
		ControlToHandle._set_size(rect_size - margins.size - margins.position)
	else:
		printerr("StylyBox was null for " + str(parent))

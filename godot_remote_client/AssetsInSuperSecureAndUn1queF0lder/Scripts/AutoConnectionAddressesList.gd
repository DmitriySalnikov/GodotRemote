extends ScrollContainer

signal selected_address

onready var list = $H/List
onready var h = $H
onready var nothing = $H/Nothing
onready var vsep = $H/VSeparator
var list_item : PackedScene = preload("res://AssetsInSuperSecureAndUn1queF0lder/Scenes/AutoConnectionListItem.tscn")

func _ready() -> void:
	get_v_scrollbar().rect_min_size.x = 24
	get_v_scrollbar().connect("visibility_changed", self, "_v_scroll_vis_changed")
	GodotRemote.get_device().connect("auto_connection_list_changed", self, "_on_auto_connection_list_changed")
	_on_auto_connection_list_changed([])
	_v_scroll_vis_changed()

# consume all wheel input here
func _gui_input(ev: InputEvent) -> void:
	if ev is InputEventMouseButton:
		if get_v_scrollbar().visible:
			if ev.button_index == BUTTON_WHEEL_DOWN or ev.button_index == BUTTON_WHEEL_LEFT or\
				ev.button_index == BUTTON_WHEEL_RIGHT or ev.button_index == BUTTON_WHEEL_UP:
				accept_event()

func _v_scroll_vis_changed():
	vsep.visible = get_v_scrollbar().visible

func _on_auto_connection_list_changed(new_list):
	var children = list.get_children()
	for c in children:
		list.remove_child(c)
		c.queue_free()
	
	for dict in new_list:
		var version : String = _dict_safe_get(dict, "version", "")
		var ip : String = _dict_safe_get(dict, "ip", "")
		var project_name : String = _dict_safe_get(dict, "project_name", "")
		var port : int = _dict_safe_get(dict, "port", 0)
		var addresses : Array =  _dict_safe_get(dict, "addresses", [])
		var img : Image = _dict_safe_get(dict, "icon", null)
		
		var tmp_item = list_item.instance()
		tmp_item.setup_params(version, project_name, port, addresses, img)
		tmp_item.connect("pressed", self, "_on_address_pressed", [ip, port])
		list.add_child(tmp_item)
	
	if list.get_child_count() > 0:
		list.visible = true
		nothing.visible = false
	else:
		list.visible = false
		nothing.visible = true
	
	yield(get_tree(), "idle_frame")
	_update_rect_size()

func _on_address_pressed(ip : String, port : int):
	var d = GodotRemote.get_device()
	if d.current_auto_connect_address != ip or d.current_auto_connect_port != port:
		d.current_auto_connect_address = ip
		d.current_auto_connect_port = port
		G.auto_ip = ip
		G.auto_port = port
		emit_signal("selected_address")

func _dict_safe_get(dict : Dictionary, key : String, def):
	if dict.has(key):
		return dict[key]
	else:
		return def

func _update_rect_size():
	var style = get_stylebox("bg")
	var sum_height = h.rect_size.y + style.content_margin_bottom + style.content_margin_top + 4*3
	if sum_height < 238:
		rect_min_size.y = sum_height
	else:
		rect_min_size.y = 238
	minimum_size_changed()

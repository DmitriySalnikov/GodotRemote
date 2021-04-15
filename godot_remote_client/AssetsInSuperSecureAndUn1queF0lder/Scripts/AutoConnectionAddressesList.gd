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
	connect("visibility_changed", self, "_update_rect_size")
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
	var current_children_map : Dictionary = {}
	
	# found all already added correct items
	var children = list.get_children()
	for c in children:
		if c.has_meta("server_uid"):
			current_children_map[c.get_meta("server_uid")] = c
		else:
			if not c.has_meta("delayed_destroy"):
				list.remove_child(c)
				c.queue_free()
	
	#print(current_children_map)
	
	for dict in new_list:
		var version : String = _dict_safe_get(dict, "version", "")
		var project_name : String = _dict_safe_get(dict, "project_name", "")
		var port : int = _dict_safe_get(dict, "port", 0)
		var server_uid : int = _dict_safe_get(dict, "server_uid", 0)
		var addresses : PoolStringArray =  _dict_safe_get(dict, "addresses", [])
		var icon_img : Image = _dict_safe_get(dict, "icon", null)
		var preview_img : Image = _dict_safe_get(dict, "preview", null)
		
		var tmp_item = null
		if current_children_map.has(server_uid):
			tmp_item = current_children_map[server_uid]
			current_children_map.erase(server_uid)
		else:
			tmp_item = list_item.instance()
			tmp_item.set_meta("server_uid", server_uid)
			tmp_item.connect("pressed", self, "_on_address_pressed", [addresses, port, project_name])
			tmp_item.connect("tree_exiting", self, "_update_rect_size")
			list.add_child(tmp_item)
			
			tmp_item.appear()
		
		tmp_item.setup_params(version, project_name, port, addresses, icon_img, preview_img)
	
	# remove all unused items
	for uid in current_children_map:
		current_children_map[uid].delayed_destroy()
	
	_update_rect_size()

func _on_address_pressed(ips : PoolStringArray, port : int, project_name : String):
	if ips.size() == 0:
		return
	var d = GodotRemote.get_device()
	
	var same = d.current_auto_connect_addresses.size() == ips.size()
	if same:
		var tmp = d.current_auto_connect_addresses
		for i in range(ips.size()):
			if tmp[i] != ips[i]:
				same = false
				break
	
	if d.current_auto_connect_project_name != project_name or !same or d.current_auto_connect_port != port:
		d.current_auto_connect_addresses = ips
		d.current_auto_connect_port = port
		d.current_auto_connect_project_name = project_name
		G.auto_addresses = ips
		G.auto_port = port
		G.auto_project_name = project_name
		emit_signal("selected_address")

func _dict_safe_get(dict : Dictionary, key : String, def):
	if dict.has(key):
		return dict[key]
	else:
		return def

func _update_rect_size():
	yield(get_tree(), "idle_frame")
	
	# show tip if nothing added
	if list.get_child_count() > 0:
		list.visible = true
		nothing.visible = false
	else:
		list.visible = false
		nothing.visible = true
	
	# dynamic block size:
	
	#yield(get_tree(), "idle_frame")
	
	#var style = get_stylebox("bg")
	#var sum_height = h.rect_size.y + style.content_margin_bottom + style.content_margin_top + 4*3
	#if sum_height < 238:
	#	rect_min_size.y = sum_height
	#else:
	#	rect_min_size.y = 238
	
	#minimum_size_changed()

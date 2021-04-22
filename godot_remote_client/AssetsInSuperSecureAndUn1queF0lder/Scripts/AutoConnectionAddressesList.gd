extends ScrollContainer

signal selected_address

onready var list = $H/List
onready var h = $H
onready var nothing = $H/Nothing
onready var vsep = $H/VSeparator
var list_item : PackedScene = preload("res://AssetsInSuperSecureAndUn1queF0lder/Scenes/AutoConnectionListItem.tscn")
var server_uid_to_button_map : Dictionary = {}
var icons_cache : Dictionary = {}

func _ready() -> void:
	get_v_scrollbar().rect_min_size.x = 24
	get_v_scrollbar().connect("visibility_changed", self, "_v_scroll_vis_changed")
	connect("visibility_changed", self, "_update_rect_size")
	GodotRemote.get_device().connect("auto_connection_list_changed", self, "_on_auto_connection_list_changed")
	
	GodotRemote.get_device().connect("auto_connection_server_connected", self, "_on_auto_connection_server_connected")
	GodotRemote.get_device().connect("auto_connection_server_error", self, "_on_auto_connection_server_error")
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
		var icon_img : ImageTexture = _dict_safe_get(dict, "icon", null)
		var preview_img : Image = _dict_safe_get(dict, "preview", null)
		
		var tmp_item : Node = null
		if current_children_map.has(server_uid):
			tmp_item = current_children_map[server_uid]
			current_children_map.erase(server_uid)
		else:
			tmp_item = list_item.instance()
			tmp_item.set_meta("server_uid", server_uid)
			tmp_item.connect("tree_exiting", self, "_update_rect_size")
			tmp_item.connect("tree_exited", self, "_remove_button_from_map", [server_uid, project_name, port])
			tmp_item.connect("server_selected", self, "_on_address_pressed")
			list.add_child(tmp_item)
			
			icons_cache["%s:%d" % [project_name, port]] = icon_img
			server_uid_to_button_map[server_uid] = tmp_item
			tmp_item.appear()
		
		tmp_item.setup_params(server_uid, version, project_name, port, addresses, icon_img, preview_img)
	
	# remove all unused items
	for uid in current_children_map:
		current_children_map[uid].delayed_destroy()
	
	_update_rect_size()

func _remove_button_from_map(uid : int, project_name : String, port : int):
	if server_uid_to_button_map.has(uid):
		server_uid_to_button_map.erase(uid)
	
	var icon_name = "%s:%d" % [project_name, port]
	if icons_cache.has(icon_name):
		icons_cache.erase(icon_name)

func _on_auto_connection_server_connected(uid : int):
	if server_uid_to_button_map.has(uid):
		server_uid_to_button_map[uid].blink_with_color(Color(0.128686, 0.300781, 0.122192))

func _on_auto_connection_server_error(uid : int):
	if server_uid_to_button_map.has(uid):
		server_uid_to_button_map[uid].blink_with_color(Color(0.34902, 0.133333, 0.133333))

func _on_address_pressed(ips : PoolStringArray, port : int, project_name : String):
	if ips.size() == 0:
		return
	var d = GodotRemote.get_device()
	
	if d.set_current_auto_connect_server(project_name, ips, port, true, 0, false):
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
	#if sum_height < 294:
	#	rect_min_size.y = sum_height
	#else:
	#	rect_min_size.y = 294
	
	#minimum_size_changed()

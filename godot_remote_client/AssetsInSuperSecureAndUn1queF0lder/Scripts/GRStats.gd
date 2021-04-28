extends PanelContainer

onready var grid = $V/Grid

onready var fps = $V/Grid/FPS_avg
onready var fps_min = $V/Grid/FPS_min
onready var fps_max = $V/Grid/FPS_max

onready var ping = $V/Grid/Ping_avg
onready var ping_min = $V/Grid/Ping_min
onready var ping_max = $V/Grid/Ping_max

onready var delay = $V/Grid/Delay_avg
onready var delay_min = $V/Grid/Delay_min
onready var delay_max = $V/Grid/Delay_max

onready var t_send_total = $V/HS/T_Send_total
onready var t_send = $V/Grid/T_Send_avg
onready var t_send_min = $V/Grid/T_Send_min
onready var t_send_max = $V/Grid/T_Send_max

onready var t_recv_total = $V/HR/T_Recv_total
onready var t_recv = $V/Grid/T_Recv_avg
onready var t_recv_min = $V/Grid/T_Recv_min
onready var t_recv_max = $V/Grid/T_Recv_max

var stat_state : int = 0
var is_detailed_available : bool = false
var is_traffic_available : bool = false

func _ready():
	G.connect("show_stats_changed", self, "show_stats_changed")
	show_stats_changed(G.show_stats)
	var dev : Node = GodotRemote.get_device()
	
	is_detailed_available = dev.has_method("get_min_fps") and dev.has_method("get_min_ping") and dev.has_method("get_min_delay")
	if not is_detailed_available:
		print("Detailed stats not available for this cleint")
	is_traffic_available = dev.has_method("get_min_send_mbyte") and dev.has_method("get_min_recv_mbyte") and dev.has_method("get_total_sended_mbytes")
	if not is_traffic_available:
		print("Traffic stats not available for this cleint")

func _process(_delta):
	if get_parent() is Control and get_parent().visible and visible and GodotRemote.get_device():
		var dev = GodotRemote.get_device()
		if stat_state > G.StatInfoState.Hidden:
			fps.text = "%.1f" % dev.get_avg_fps()
			ping.text = "%.1f" % dev.get_avg_ping()
		
		if is_detailed_available and (stat_state == G.StatInfoState.Detailed or stat_state == G.StatInfoState.All):
			fps_min.text = "%.1f" % dev.get_min_fps()
			fps_max.text = "%.1f" % dev.get_max_fps()
			
			ping_min.text = "%.1f" % dev.get_min_ping()
			ping_max.text = "%.1f" % dev.get_max_ping()
			
			delay.text = "%.1f" % dev.get_avg_delay()
			delay_min.text = "%.1f" % dev.get_min_delay()
			delay_max.text = "%.1f" % dev.get_max_delay()
		
		if is_traffic_available and (stat_state == G.StatInfoState.Traffic or stat_state == G.StatInfoState.All):
			t_send_total.text = "%.3f" % dev.get_total_sended_mbytes()
			t_send.text = "%.3f" % dev.get_avg_send_mbyte()
			t_send_min.text = "%.3f" % dev.get_min_send_mbyte()
			t_send_max.text = "%.3f" % dev.get_max_send_mbyte()
			
			t_recv_total.text = "%.3f" % dev.get_total_received_mbytes()
			t_recv.text = "%.3f" % dev.get_avg_recv_mbyte()
			t_recv_min.text = "%.3f" % dev.get_min_recv_mbyte()
			t_recv_max.text = "%.3f" % dev.get_max_recv_mbyte()
		
		rect_size = grid.rect_size

func change_visibility_of_groups(groups : Array, state : bool):
	var nodes : Array = []
	for s in groups:
		nodes.append_array(get_tree().get_nodes_in_group(s))
	
	for l in nodes:
		l.visible = state

func show_stats_changed(state):
	stat_state = state
	match state:
		G.StatInfoState.Hidden:
			visible = false
		G.StatInfoState.All:
			visible = true
			change_visibility_of_groups(["simple", "traffic", "detailed"], true)
			grid.columns = 4
		G.StatInfoState.Simple:
			visible = true
			change_visibility_of_groups(["simple"], true)
			change_visibility_of_groups(["traffic", "detailed"], false)
			grid.columns = 2
		G.StatInfoState.Detailed:
			visible = true
			change_visibility_of_groups(["traffic"], false)
			change_visibility_of_groups(["detailed", "simple"], true)
			grid.columns = 4
		G.StatInfoState.Traffic:
			visible = true
			change_visibility_of_groups(["traffic"], true)
			change_visibility_of_groups(["detailed", "simple"], false)
			grid.columns = 4

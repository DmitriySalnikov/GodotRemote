extends PanelContainer

onready var grid = $Grid
onready var fps = $Grid/FPS
onready var fps_min = $Grid/FPS_min
onready var fps_max = $Grid/FPS_max
onready var ping = $Grid/Ping
onready var ping_min = $Grid/Ping_min
onready var ping_max = $Grid/Ping_max
var stat_state : int = 0

onready var detailed_labels = [
	ping_max, ping_min,
	fps_max, fps_min,
]

func _ready():
	G.connect("show_stats_changed", self, "show_stats_changed")
	show_stats_changed(G.show_stats)

func _process(_delta):
	if GodotRemote.get_device():
		if stat_state > G.StatInfoState.Hidden:
			fps.text = "FPS: %.1f" % GodotRemote.get_device().get_avg_fps()
			ping.text = "PING: %.1f" % GodotRemote.get_device().get_avg_ping()
		
		if stat_state > G.StatInfoState.Simple:
			fps_min.text = "%.1f" % GodotRemote.get_device().get_min_fps()
			fps_max.text = "%.1f" % GodotRemote.get_device().get_max_fps()
			
			ping_min.text = "%.1f" % GodotRemote.get_device().get_min_ping()
			ping_max.text = "%.1f" % GodotRemote.get_device().get_max_ping()
		
		rect_size = grid.rect_size

func change_detailed_label_visibility(state : bool):
	for l in detailed_labels:
		l.visible = state

func show_stats_changed(state):
	stat_state = state
	match state:
		G.StatInfoState.Hidden:
			visible = false
		G.StatInfoState.Simple:
			visible = true
			change_detailed_label_visibility(false)
			grid.columns = 1
		G.StatInfoState.Detailed:
			visible = true
			change_detailed_label_visibility(true)
			grid.columns = 3

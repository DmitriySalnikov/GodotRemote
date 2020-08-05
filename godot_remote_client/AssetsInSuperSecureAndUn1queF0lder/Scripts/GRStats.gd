extends PanelContainer

onready var fps = $HBoxContainer/FPS
onready var ping = $HBoxContainer/Ping

func _ready():
	G.connect("show_stats_changed", self, "show_stats_changed")
	visible = G.show_stats

func _process(_delta):
	if visible and GodotRemote.get_device():
		fps.text = "FPS: %.1f" % GodotRemote.get_device().get_avg_fps()
		ping.text = "PING: %.1f" % GodotRemote.get_device().get_avg_ping()

func show_stats_changed(state):
	visible = state

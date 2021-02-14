extends WindowDialog

var has_billings = false
var shown = false

func _enter_tree():
	if G.Billings:
		has_billings = true
		G.Billings.connect("billings_ready", self, "_billings_ready")

func _is_ready_to_show() -> bool:
	return (G.AppRuns != 0 and G.AppRuns % 5 == 0) and !G.VersionChanged

func _ready():
	if not has_billings and not shown:
		shown = true
		if _is_ready_to_show():
			call_deferred("popup_centered")

func _billings_ready():
	if G.Billings.get_purchased_points() == 0 and not shown:
		shown = true
		if _is_ready_to_show():
			call_deferred("popup_centered")

func _on_Button2_pressed():
	hide()

func _on_Button_pressed():
	hide()
	if has_billings:
		get_parent().show_support_window()
	else:
		OS.shell_open("https://github.com/DmitriySalnikov/GodotRemote#godot-remote")

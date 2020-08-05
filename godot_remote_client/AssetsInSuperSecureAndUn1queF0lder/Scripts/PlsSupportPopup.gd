extends WindowDialog

var has_billings = false

func _enter_tree():
	if G.Billings:
		has_billings = true
		G.Billings.connect("billings_ready", self, "_billings_ready")

func _ready():
	if not has_billings:
		if G.AppRuns == 4:
			popup_centered()

func _billings_ready():
	if G.Billings.get_purchased_points() == 0:
		if G.AppRuns != 0 && G.AppRuns % 4 == 0:
			popup_centered()

func _on_Button2_pressed():
	hide()

func _on_Button_pressed():
	hide()
	if has_billings:
		get_parent().show_support_window()
	else:
		OS.shell_open("https://github.com/DmitriySalnikov/GodotRemote")

extends WindowDialog

var has_billings = false
var shown = false
var one_of_buttons_pressed = false

func _enter_tree():
	if G.Billings:
		has_billings = true
		G.Billings.connect("billings_ready", self, "_billings_ready")

func _is_ready_to_show() -> bool:
	return (G.AppRuns != 0 and G.AppRuns % 5 == 0) and !G.VersionChanged and G.FirstRunAgreementAccepted

func _ready():
	if not has_billings and not shown:
		shown = true
		if _is_ready_to_show():
			call_deferred("popup_centered")
			get_close_button().call_deferred("hide")

func _billings_ready():
	if G.Billings.get_purchased_points() == 0 and not shown:
		shown = true
		if _is_ready_to_show():
			call_deferred("popup_centered")

func _on_Button2_pressed():
	one_of_buttons_pressed = true
	hide()
	
	G.a_design_event("PlsSupportDeveloper:NotNow")

func _on_Button_pressed():
	one_of_buttons_pressed = true
	hide()
	
	if has_billings:
		get_parent().show_support_window()
	else:
		OS.shell_open("https://github.com/DmitriySalnikov/GodotRemote#godot-remote")
	
	G.a_design_event("PlsSupportDeveloper:Yes")

func _on_PlsSupportPopup_visibility_changed() -> void:
	if !visible:
		if shown and !one_of_buttons_pressed:
			G.a_design_event("PlsSupportDeveloper:ClickOutside")

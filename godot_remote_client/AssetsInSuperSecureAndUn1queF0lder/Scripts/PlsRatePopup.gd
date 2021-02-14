extends WindowDialog

func _is_ready_to_show() -> bool:
	return (G.Billings and ((G.AppRuns == 7 or (G.AppRuns > 9 and G.AppRuns % 9 == 0)) and G.UserRateState == G.RateState.NotNow)) and !G.VersionChanged

func _ready():
	if _is_ready_to_show():
		call_deferred("popup_centered")

func _on_yes_pressed() -> void:
	hide()
	G.UserRateState = G.RateState.Yes
	#if G.Billings:
	OS.shell_open("https://play.google.com/store/apps/details?id=com.dmitriysalnikov.godotremote")

func _on_no_pressed() -> void:
	hide()
	G.UserRateState = G.RateState.No

func _on_later_pressed() -> void:
	hide()
	G.UserRateState = G.RateState.NotNow

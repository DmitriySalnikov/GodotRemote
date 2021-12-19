extends WindowDialog

var is_in_app_review_ready = false
var in_app_review = null

func _is_can_be_shown() -> bool:
	return (G.Billings and ((G.AppRuns == 7 or (G.AppRuns > 9 and G.AppRuns % 9 == 0)) and G.UserRateState != G.RateState.Yes)) and !G.VersionChanged and G.FirstRunAgreementAccepted

func _ready():
	if _is_can_be_shown():
		if Engine.has_singleton("GodotGooglePlayInAppReview"):
			in_app_review = Engine.get_singleton("GodotGooglePlayInAppReview")
			in_app_review.connect("on_request_review_success", self, "_on_request_review_success")
			in_app_review.connect("on_request_review_failed", self, "_on_request_review_failed")
			in_app_review.connect("on_launch_review_flow_success", self, "_on_launch_review_flow_success")
			in_app_review.requestReviewInfo()
		call_deferred("popup_centered")
		get_close_button().call_deferred("hide")

func _on_yes_pressed() -> void:
	hide()
	
	if is_in_app_review_ready:
		in_app_review.launchReviewFlow()
	else:
		OS.shell_open("https://play.google.com/store/apps/details?id=com.dmitriysalnikov.godotremote")
		G.UserRateState = G.RateState.Yes
	
	G.a_design_event("PlsRateThisApp:Yes")

func _on_no_pressed() -> void:
	hide()
	G.UserRateState = G.RateState.No
	
	G.a_design_event("PlsRateThisApp:No")

func _on_later_pressed() -> void:
	hide()
	G.UserRateState = G.RateState.NotNow
	
	G.a_design_event("PlsRateThisApp:NotNow")

func _on_request_review_success():
	is_in_app_review_ready = true

func _on_request_review_failed():
	is_in_app_review_ready = false

func _on_launch_review_flow_success():
	G.UserRateState = G.RateState.Yes

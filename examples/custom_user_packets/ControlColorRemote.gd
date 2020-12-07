extends Control

func _ready() -> void:
	GodotRemote.get_device().connect("user_data_received", self, "user_data_received")

func _on_ColorPickerButton_color_changed(color: Color) -> void:
	GodotRemote.get_device().send_user_data("bg_color", color, false)

func user_data_received(packet_id, user_data):
	print("Received packet: %s, data: %s" % [packet_id, user_data])
	match packet_id:
		"slider_value": $ProgressBar.value = user_data

extends Button

func _ready() -> void:
	pass # Replace with function body.

func setup_params(version : String, ip : String, project_name : String, port : int, addresses : Array, img : Image):
	if img and not img.is_empty():
		var tex = ImageTexture.new()
		tex.create_from_image(img)
		$H/Icon.texture = tex
	else:
		$H/Icon.texture = null
	
	$H/V/ProjectName.text = project_name
	$H/V/Port.text = str(port)
	$H/V2/Version.text = version
	
	var adr_line = ""
	for a in range(addresses.size()):
		adr_line += addresses[a]
		if a != addresses.size() - 1 :
			adr_line += ", "
	$H/V/Addresses.text = adr_line
	

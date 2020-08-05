extends Node


var CustomMappings : = [
	"03000000ac0500002c02000000000000,ipega Bluetooth Gamepad,platform:Windows,a:b0,b:b1,x:b3,y:b4,back:b10,start:b11,leftstick:b13,rightstick:b14,leftshoulder:b8,rightshoulder:b9,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,-rightx:a3~,+rightx:a4,-righty:a5,+righty:a4,lefttrigger:b6,righttrigger:b7,",
]

func _ready():
	for m in CustomMappings:
		Input.add_joy_mapping(m);

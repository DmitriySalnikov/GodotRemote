extends Control


# firstly you need to disable autostart GodotRemote in Project Settings/Debug/Godot Remote/General
# and change the Network/Limits/Connect Timeout Seconds to 1 otherwise app will be closing very long time
func _ready():
	# create client
	GodotRemote.create_remote_device(GodotRemote.DEVICE_CLIENT)
	
	# get device and convert it to client class
	var d : GRClient = GodotRemote.get_device()
	# set control where you want to see stream. it can be whole screen control or custom 'viewport'
	d.set_control_to_show_in(self)
	# set address of server. optional if you want to connect to other projects on one pc or if you use connection over adb 
	d.set_address("127.0.0.1")
	# set password to get acces to the server if it need one
	d.password = "1234"
	# and change other settings if you need it
	
	# start client
	GodotRemote.start_remote_device()

# If you need to support custom input scenes best way to avoid any errors by overriding resources
# from server is just put all assets of this project to folder with unique and long name
# 
# Example:
# *res://
#   -UniqueL0ngNameThatINeverOverrideFromServer
#      -icon.png
#      -default_env.tres
#      -Scene.tscn
#      -Scene.gd

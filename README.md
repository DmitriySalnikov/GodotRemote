# Godot Remote

This is cross platform native module for [Godot Engine](https://github.com/godotengine/godot) v3 for control apps and games over WiFi or ADB.

If you are developing on a non-touch device, this module is the best way to quickly test touch input or test mobile sensors data.

API references can be found here: [API.md](API.md)

Building instructions placed here: [BUILDING.md](BUILDING.md)

| Video Demonstration                                                                                                             | Custom Packets Demo                                                                                                             |
| ------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| [<img src="https://img.youtube.com/vi/LbFcQnS3z3E/0.jpg" alt="Video Demonstration" width="200"/>](https://youtu.be/LbFcQnS3z3E) | [<img src="https://img.youtube.com/vi/RmhppDWZZk8/0.jpg" alt="Custom Packets Demo" width="200"/>](https://youtu.be/RmhppDWZZk8) |

## Support

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/I2I53VZ2D)

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/dmitriysalnikov)

## Changelog

Changelogs can be found inside the mobile app or on the [github releases](https://github.com/DmitriySalnikov/GodotRemote/releases) page

## Download

### Editor with Built-in Module

<iframe frameborder="0" src="https://itch.io/embed/1010487?border_width=2" width="554" height="169"><a href="https://dmitriysalnikov.itch.io/godot-remote">Godot Remote by Dmitriy Salnikov</a></iframe>

### Mobile app

The mobile app can be found on [Google Play](https://play.google.com/store/apps/details?id=com.dmitriysalnikov.godotremote), as well as on [itch.io](https://dmitriysalnikov.itch.io/godot-remote).

## Mobile Client Configuration

### Quick Setup

1. Open the app and accept the agreement that you understand that this is not a game. Sorry for that, but random people from Google Play just download this app and think it's a game, then rate it 1 star. ![Quick Setup 0](Images/Screenshots/quick_setup_0.jpg)
2. Then you'll see the main screen where you need to click on the `Open Settings` ![Quick Setup 1](Images/Screenshots/quick_setup_1.jpg)
3. Now you need to start Godot Editor with `Godot Remote` module and run one or many projects. ![Quick Setup 2](Images/Screenshots/quick_setup_2.jpg)
4. If everything went well, all running projects will be added to the `Available Servers` list in the mobile app. ![Quick Setup 3](Images/Screenshots/quick_setup_3.jpg)
5. Just click on one of them from this list. ![Quick Setup 4](Images/Screenshots/quick_setup_4.jpg)
6. Close the settings menu and you're ready to go! ![Quick Setup 5](Images/Screenshots/quick_setup_5.jpg)

**Important:** The client and server (smartphone and PC with the Godot editor) must be on the same local network, for example, the PC can be connected to the router via a LAN cable, and the smartphone can be connected via Wi-Fi.

**Important:** `Godot Editor with 'Godot Remote' module`  means that you should see the module settings category in your projects settings.
![Settings](Images/Screenshots/settings.png)

Also, starting from version 1.0.2, you can find the `Godot Remote` tool menu in the editor.
![Settings](Images/Screenshots/tool_menu.png)

## Advanced

### Custom Client

If need to support other platforms or you need a specific version of module integrated to the client app, you can build client from source code placed [here](godot_remote_client).

If you don't want to use my client app you can check the [example client project](examples/simple_client) and build your own client.

Or you can donate me some money with request to buy iPhone and adapt a client for it 🙂

### Custom Input Scenes

In custom input scenes you can use everything you want but to send InputEvent's from client to server you must emulate input. Or use the `send_user_data` method and `user_data_received` signal for send and receive custom packets.
Example:

```python
# -- With InputEvent's

func _on_pressed():
    # Create event for pressed state
    var iea_p = InputEventAction.new()
    iea_p.pressed = true
    iea_p.action = "jump"
    # Create event for released state
    var iea_r = InputEventAction.new()
    iea_r.pressed = false
    iea_p.action = "jump"
    # Parse event to send it to the server
    Input.parse_input_event(iea_p)
    Input.parse_input_event(iea_r)

# -- With custom packets

# on first device
func _ready():
    GodotRemote.get_device().connect("user_data_received", self, "_on_user_data_received")

func _on_user_data_received(id, data):
    print("Received packet: %s, data: %s" % [id, data])

# on second device
func _on_button_pressed():
    GodotRemote.get_device().send_user_data("bg_color", color, false)
```

## License

MIT license

# Godot Remote

This is cross platform native module for [Godot Engine](https://github.com/godotengine/godot) v3 for control apps and games over WiFi or ADB.

If you are developing on a non-touch device, this module is the best way to quickly test touch input or test mobile sensors data.

[Video Demonstration](https://youtu.be/LbFcQnS3z3E)

[Custom Packets Demo](https://youtu.be/RmhppDWZZk8)

## Support

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/I2I53VZ2D)

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/dmitriysalnikov)

## Download

Precompiled binaries can be found on [GitHub Releases](https://github.com/DmitriySalnikov/GodotRemote/releases) page

### Mobile app

On releases page you can found precompiled mobile app but also it can be downloaded from [Google Play](https://play.google.com/store/apps/details?id=com.dmitriysalnikov.godotremote)

## Configure Mobile App

To open settings menu you need to touch the screen with 5 fingers at once.

Then you'll see this settings menu:

![Settings](Images/Screenshots/mobile_settings.png)

**Important:** after entering server address you should apply it by pressing `Set Type and Address` or `Set Type and Port`


## Custom client

If need to support other platforms or you need a specific version of module integrated to the client app, you can build client from source code placed [here](godot_remote_client).

If you don't want to use my client app you can check the [example client project](examples/simple_client) and build your own client.

Or you can donate me some money with request to buy iPhone and adapt a client for it 🙂

## Custom Input Scenes

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

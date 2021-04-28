
# API Reference

Here the methods will be declared according to this pattern:

```python
return_type function_name([arg_name1 : type [= defalut value]][, arg_name2 : type [= defalut value]])
```

**Important:** All enums in GDNative version is exposed in GodotRemote class because of limitations.
For example, if you want to use StreamState.STREAM_ACTIVE from GRClient you need to get property GRClient_STREAM_ACTIVE of GodotRemote __object__

```python
# Godot module:
GRClient.STREAM_ACTIVE:

# GDNative
# *GodotRemote is autoload NativeScript
GodotRemote.GRClient_STREAM_ACTIVE
```

## GodotRemote

Main class of module with methods to start/stop client and server. Also this class contains bindings to static class GRNotifications and some other utilities methods.

```python
# --- Properties

# Canvas layer that shows notifications
# type int, default 128
notifications_layer

# Notifications position on screen
# type GRNotifications.NotificationsPosition, default TC
notifications_position

# Is notifications enabled
# type bool, default true
notifications_enabled

# Base duration for showing notifications
# type float, default 3.0
notifications_duration

# Notifcations style
# type GRNotificationStyle
notifications_style

# --- Methods

# Notifications

# Adds or fully update existing notification
# @title: Notification title
# @text: Text of notification
# @notification_icon: Notification icon from enum NotificationIcon
# @update_existing: Updates existing notification
# @duration_multiplier: Multiply base notifications duration
void add_notification(title: String, text: String, notification_icon: GRNotifications.NotificationIcon = 0, update_existing: bool = true, duration_multiplier: float = 1.0)

# Adds new notification or append text to existing notification
# @title: Notification title
# @text: Text of notification
# @icon: Notification icon from enum NotificationIcon
# @add_to_new_line: Adds text to new line or adds to current line
void add_notification_or_append_string(title: String, text: String, icon: GRNotifications.NotificationIcon, add_to_new_line: bool = true, duration_multiplier: float = 1.0)

# Adds notification or update one line of notification text
# @title: Notification title
# @id: Line ID
# @text: Text of notification
# @icon: Notification icon from enum NotificationIcon
# @duration_multiplier: Multiply base notifications duration
void add_notification_or_update_line(title: String, id: String, text: String, icon: GRNotifications.NotificationIcon, duration_multiplier: float = 1.0)

# Clear all notifications
void clear_notifications()

# Get notifications list
# @return list of all visible notifications
Array get_all_notifications()

# Get notification with specified title or null
# @title: Notification title
# @return matched notification
GRNotificationPanel get_notification(title: String)

# Get all notifications with specified title
# @title: Notification title
# @return list of visible notifications
Array get_notifications_with_title(title: String)

# Remove notifications with specified title
# @title: Notifications title
# @is_all_entries: Delete all notifications with @title if true
void remove_notification(title: String, is_all_entries: bool = true)

# Remove exact notification by reference
# @notification: Notification reference
void remove_notification_exact(notification: Node)

# Binding for 'connect' method of GRNotifications
int notifications_connect(signal: String, inst: Object, method: String, binds: Array, flags: int)

# Client/Server

# Create device: client or server
# @device_type: Type of device
# @return true if device created successful
bool create_remote_device(device_type: GodotRemote.DeviceType = 0)

# Start device
# @return true if device valid
bool start_remote_device()

# Create and start device
# @device_type: Type of device
void create_and_start_device(device_type: GodotRemote.DeviceType = 0)

# Remove and delete currently working device
# @return true if succeed
bool remove_remote_device()

# Get device
# @return client, server or null
GRDevice get_device()

# Utility functions

# Not exposed to GDScript fuctions from Input class
# And currently not available in GDNative
void set_accelerometer(value: Vector3)
void set_gravity(value: Vector3)
void set_gyroscope(value: Vector3)
void set_magnetometer(value: Vector3)

# Set GodotRemote log level
# @level: Level of logging
void set_log_level(level: LogLevel)

# Debug methods for printing the log
void print_str(txt: String)
void print_warning_str(txt: String, func: String, file: String, line: int)
void print_error_str(txt: String, func: String, file: String, line: int)

# Get GodotRemote module version
# @return module version in format "MAJOR.MINOR.BUILD"
String get_version()

# Get safe area for specified CanvasItem. (e.g. area without any cutouts on Android or iOS)
# @canvas_item: canvas item needed to get viewport
# @return correctly scaled safe area rect
Rect2 get_2d_safe_area(canvas_item: CanvasItem)

# @return true if module compiled as GDNative library
bool is_gdnative()

# --- Signals

# Device added
device_added()

# Device removed
device_removed()

# --- Enumerations

DeviceType:
    DEVICE_AUTO = 0
    DEVICE_SERVER = 1
    DEVICE_CLIENT = 2

LogLevel:
    LL_NONE = 4
    LL_DEBUG = 0
    LL_NORMAL = 1
    LL_WARNING = 2
    LL_ERROR = 3
```

## GRNotifications

Container for all notifications

```python

# --- Signals

# Called when a single notification is added
notification_added(notification: GRNotificationPanel)

# Called when a single notification is removed
notification_removed(title: String, is_cleared: bool)

# Called when all notifications are cleared
notifications_cleared()

# Called when notifications are enabled or disabled
notifications_toggled(is_enabled: bool)

# --- Enumerations

NotificationIcon:
    ICON_NONE = 0
    ICON_ERROR = 1
    ICON_WARNING = 2
    ICON_SUCCESS = 3
    ICON_FAIL = 4

NotificationsPosition:
    TOP_LEFT = 0
    TOP_CENTER = 1
    TOP_RIGHT = 2
    BOTTOM_LEFT = 3
    BOTTOM_CENTER = 4
    BOTTOM_RIGHT = 5
```

## GRNotificationPanel

Notification instance

```python
# --- Methods

# @return notification icon id
GRNotifications.NotificationIcon get_icon_id()

# @return notification text
String get_text()

# @return notification title
String get_title()

# Update notification text
# @text: new text to show
void update_text(text: String)

```

## GRNotificationPanelUpdatable

Extended version of GRNotificationPanel (but API not fully exposed)

```python
# --- Methods

# Clear all lines
void clear_lines(text: String)

# Remove one line from notification
# @id: line id
void remove_updatable_line(id: String)

```

## GRNotificationStyle

Helper class to store parameters of notifications style

```python
# --- Properties

# Style of background notifications panel
# type StyleBox
panel_style

# Theme for notification close button
# type Theme
close_button_theme

# Close button icon texture
# type Texture
close_button_icon

# Notification title font
# type Font
title_font

# Notification text font
# type Font
text_font

# --- Methods

# Get notification icon from this style
# @notification_icon: Notfication icon id
# @return icon texture of null
Texture get_notification_icon(notification_icon: GRNotifications.NotificationIcon)

# Set notification icon in this style
# @notification_icon: Notfication icon id
# @icon_texture: Icon texture
void set_notification_icon(notification_icon: GRNotifications.NotificationIcon, icon_texture: Texture)

```

## GRDevice

Base class for client and server

```python
# --- Properties

# Connection port
# type int, default 22766
port

# Port for auto connection mode listener
# type int, default 22765
auto_connection_port

# --- Methods

# Send user data to remote device
# @packet_id: any data to identify your packet
# @user_data: any data to send to remote device
# @full_objects: flag for full serialization of objects, possibly with their executable code. For more info check Godot's PacketPeer.put_var() and PacketPeer.get_var()
void send_user_data(packet_id: Variant, user_data: Variant, full_objects: bool = false)

# Get device status
WorkingStatus get_status()

# Start device
void start()

# Stop device
void stop()

# Restart device
void restart()

# @return the total number of MB received for the current connection
float get_total_received_mbytes()

# @return the total number of MB sended for the current connection
float get_total_sended_mbytes()

# Methods for getting counters values
# - FPS

# @return average FPS
float get_avg_fps()

# @return minimum FPS
float get_min_fps()

# @return maximum FPS
float get_max_fps()

# - Ping

# @return average ping
float get_avg_ping()

# @return minimum ping
float get_min_ping()

# @return maximum ping
float get_max_ping()

# - Received MBytes in previous few seconds

# @return average received MBytes
float get_avg_recv_mbyte()

# @return minimum received MBytes
float get_min_recv_mbyte()

# @return maximum received MBytes
float get_max_recv_mbyte()

# - Sended MBytes in previous few seconds

# @return average sended MBytes
float get_avg_send_mbyte()

# @return minimum sended MBytes
float get_min_send_mbyte()

# @return maximum sended MBytes
float get_max_send_mbyte()

# --- Signals

# Device status changed
status_changed(status: GRDevice.WorkingStatus)

# User data received from a remote device
user_data_received(packet_id: Variant, user_data: Variant)

# --- Enumerations

ImageCompressionType:
    COMPRESSION_JPG = 1
    COMPRESSION_H264 = 3

TypesOfServerSettings:
    SERVER_SETTINGS_USE_INTERNAL = 0
    SERVER_SETTINGS_VIDEO_STREAM_ENABLED = 1
    SERVER_SETTINGS_COMPRESSION_TYPE = 2
    SERVER_SETTINGS_STREAM_QUALITY = 3
    SERVER_SETTINGS_SKIP_FRAMES = 4
    SERVER_SETTINGS_RENDER_SCALE = 5
    SERVER_SETTINGS_TARGET_FPS = 6
    SERVER_SETTINGS_THREADS_NUMBER = 7

WorkingStatus:
    STATUS_STARTING = 3
    STATUS_STOPPING = 2
    STATUS_WORKING = 1
    STATUS_STOPPED = 0
```

## GRServer

```python
# --- Properties

# Server password
# type String, default ""
password

# Path to the custom input scene.
# type String, default ""
custom_input_scene

# Is custom input scene compressed
## Doesn't work in GDNative
# type bool, default true
custom_input_scene_compressed

# Compression type of custom input scene
## Doesn't work in GDNative
# type File.CompressionMode, default FastLZ
custom_input_scene_compression_type

# --- Methods

# Set whether the stream is enabled
bool set_video_stream_enabled(value : bool)

# @return whether the stream is enabled
bool is_video_stream_enabled()

# Set how many frames to skip
bool set_skip_frames(frames : int)

# @return the number of skipping frames
int get_skip_frames()

# @return the number of encoder threads
int get_encoder_threads_count()

# Set number of encoder threads
bool set_encoder_threads_count(count : int)

# Set JPG quality
bool set_jpg_quality(quality : int)

# @return JPG quality
int get_jpg_quality()

# Set the scale of the stream
bool set_render_scale(scale : float)

# @return stream scale
float get_render_scale()

# Force update custom input scene on client
void force_update_custom_input_scene()

# Get resize viewport node
# @return resize viewport or null
GRSViewport get_gr_viewport()


# --- Signals

# On client connected
client_connected(device_id: String)

# On client disconnected
client_disconnected(device_id: String)

# On orientation of client's screen or viewport changed
client_viewport_orientation_changed(is_vertical: bool)

# On client's screen or viewport aspect ratio changed
client_viewport_aspect_ratio_changed(stream_aspect: float)

```

## GRClient

```python
# --- Properties

# Capture input only when containing control has focus
# type bool, default false
capture_on_focus

# Capture input only when stream image hovered
# type bool, default true
capture_when_hover

# Capture mouse pointer and touch events
# type bool, default true
capture_pointer

# Capture input
# type bool, default true
capture_input

# Type of connection
# type GRClient.ConnectionType, default CONNECTION_WiFi
connection_type

# Frequency of sending data to the server
# type int, default 60
target_send_fps

# Stretch mode of stream image
# type GRClient.StretchMode, default STRETCH_KEEP_ASPECT
stretch_mode

# Use texture filtering of stream image
# type bool, default true
texture_filtering

# Password
# type String, default ""
password

# ID of device
# type String, default 6 random digits and characters
device_id

# Sync viewport orientation with server
# type bool, default true
viewport_orientation_syncing

# Sync viewport aspect ratio with server
# type bool, default true
viewport_aspect_ratio_syncing

# Receive updated server settings
# type bool, default false
server_settings_syncing

# Set whether to process the preview images
# type bool, default false
auto_connection_preview_processing

# --- Methods

# Break current connection blocking
void break_connection()

# Break current connection not blocking
void break_connection_async()

# Restore settings on server
void disable_overriding_server_settings()

# Get the current visible custom input scene
# @return: Custom input scene
Node get_custom_input_scene()

# Get server address
# @return server address
String get_address()

# Is connected to server
# @return true if connected to server
bool is_connected_to_host()

# Is stream active
# @return true if stream active
bool is_stream_active()

# Set server address to connect
# @ip: IP of server
# @return true if address is valid
bool set_address(ip: String)

# Set both server address and port
# @ip: IP of server
# @port: Port of server
# @return true if address is valid
bool set_address_port(ip: String, port: int)

# Set the control to show stream in
# @control_node: Control where stream will be shown
# @position_in_node: Position of stream in parent
void set_control_to_show_in(control_node: Control, position_in_node: int = 0)

# Set custom material for no signal screen
# @material: Custom material
void set_custom_no_signal_material(material: Material)

# Set custom horizontal texture for no signal screen
# @texture: Custom texture
void set_custom_no_signal_texture(texture: Texture)

# Set custom vertical texture for no signal screen
# @texture: Custom texture
void set_custom_no_signal_vertical_texture(texture: Texture)

# Override setting on server
# @setting: Which setting need to change
# @value: Value of setting
void set_server_setting(setting: GRdevice.TypesOfServerSettings, value: Variant)

# Set auto connection mode server
# @project_name: project name to connect
# @addresses: prioritized addresses to connect
# @port: prioritized port to connect
# @connect_to_exact_server: is need to connect to exact server with specifed data
# @exact_time_limit: time limit in seconds to exact connection. 0 means infinite wait time
# @force_update: force update values and break current connection
bool set_current_auto_connect_server(project_name: String, addresses: PoolStringArray, port: int, connect_to_exact_server: bool = true, exact_time_limit: float = 0, force_update: bool = false)

# @return auto connection prioritized list of addresses
PoolStringArray get_current_auto_connect_addresses()

# @return auto connection prioritized port
int get_current_auto_connect_port()

# @return auto connection project name
String get_current_auto_connect_project_name()

# @return stream aspect ratio
float get_stream_aspect_ratio()

# @return the number of decoder threads
int get_decoder_threads_count()

# Set number of decoder threads
bool set_decoder_threads_count(count : int)

# - Delay

# @return average delay
float get_avg_delay()

# @return minimum delay
float get_min_delay()

# @return maximum delay
float get_max_delay()

# --- Signals

# On list of available servers changed
# @available_connections: Array of Dictionaries
# Each dictionary contains such entries:
#   version: String
#   project_name: String
#   port: int
#   server_uid: int
#   addresses: PoolStringArray
#   preview: Image
#   icon: ImageTexture
auto_connection_list_changed(available_connections: Array)

# On auto connection listener status changed
auto_connection_listener_status_changed(is_listening: bool)

# On successful connection to the server
# @uid: unique server id (changed on every server start)
auto_connection_server_connected(uid: int)

# On auto connection to the server error occur
# @uid: unique server id (changed on every server start)
auto_connection_server_error(uid: int)

# On custom input scene added and becomes visible
custom_input_scene_added()

# On custom input scene removed
custom_input_scene_removed()

# On connection state changed
connection_state_changed(is_connected: bool)

# On stream state changed
stream_state_changed(state: GRClient.StreamState)

# On server stream aspect ratio changed (available without active stream too)
stream_aspect_ratio_changed(ratio: float)

# On mouse mode changed on server
mouse_mode_changed(mouse_mode: Input.MouseMode)

# On received server settings from server
# @settings: Dictionary where key is GRDevice.TypesOfServerSettings and value is Variant
server_settings_received(settings: Dictionary)

# On received hint string for quality option (e.g "100%", "25434 kbit/s", etc)
server_quality_hint_setting_received(hint: String)

# --- Enumerations

ConnectionType:
    CONNECTION_ADB = 1
    CONNECTION_WiFi = 0
    CONNECTION_AUTO = 2

StreamState:
    STREAM_NO_SIGNAL = 0
    STREAM_ACTIVE = 1
    STREAM_NO_IMAGE = 2

StretchMode:
    STRETCH_KEEP_ASPECT = 0
    STRETCH_FILL = 1
```

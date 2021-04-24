# Building Godot Remote

## General

1. Clone repository to any folder with `git clone --recursive https://github.com/DmitriySalnikov/GodotRemote.git` (you need `git` installed) or just download this repository but not all parameters will work correctly
1. [Configure environment](https://docs.godotengine.org/en/3.3/development/compiling/index.html) to build editor or gdnative library

## Compiling the Module

### As a module

1. copy `godot_remote` folder to the `modules/` directory or make [symlink](https://en.wikipedia.org/wiki/Symbolic_link) or use `custom_modules=[path to godot_remote folder]` argument
2. compile the engine according to the instructions in the Godot documentation with an additional argument `module_godot_remote_enabled=yes` (e.g. `scons p=windows tools=yes module_godot_remote_enabled=yes -j[place here count of your CPU threads]`)
3. run `bin/godot[based on config]`

If everything compiles successfully, you'll find a new category in project settings `Debug/Godot Remote` where you can configure the server.

![Settings](Images/Screenshots/settings.png)

### As a GDNative library

1. (Optional)
   1. Generate `api.json` for GDNative api. `bin/godot --gdnative-generate-json-api api.json`
   2. Copy `api.json` to the root directory of this repository (or just use existing one from `godot-cpp/godot-headers` folder)
2. Compile godot-cpp (e.g. in `godot-cpp` directory run `scons generate_bindings=true platform=windows target=release bits=64 -j8 custom_api_file="../api.json"` (remove `custom_api_file` argument if using default one))
3. Compile module for your platform (Available platforms: windows, osx, linux, ios, android. Tested and verified platforms: windows, linux)
   1. For android: Run in root directory `[path to your android ndk root dir]/ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk  APP_PLATFORM=android-21` (currently broken)
   2. For all other platforms: `scons platform=[your platform] target=release -j8`
4. Use produced library in `bin/`

GDNative has limitations so here `GodotRemote` is not a singleton and you need to create autoload NativeScript for `GodotRemote` class. Also there is no any settings in `Debug/Godot Remote`.

Enum constants in this version changed too (see [API.md](API.md) )

**Currently, the GDNative version does not support the assignment of sensor data, so the editor will not support accelerometer, gyroscope, etc.
Also, this version may fail at any time.**

If GDNative becomes more stable, I will add the necessary code to easily integrate this module into any project, but now it just works.. sometimes.

## Additional Parameters

Also module has additional compilation parameters

| Parameter                            | Default Value | Description                                                                              |
| ------------------------------------ | ------------- | ---------------------------------------------------------------------------------------- |
| godot_remote_disable_server          | no            | Disable server.                                                                          |
| godot_remote_disable_client          | no            | Disable client.                                                                          |
| godot_remote_no_default_resources    | no            | Remove default resources. (notification icons, background "no signal", etc)              |
| godot_remote_auto_connection_enabled | yes           | Enable auto connection mode using UDP broadcasting.                                      |
| godot_remote_libjpeg_turbo_enabled   | yes           | Enable libjpeg-turbo integration. (much faster than default jpge.cpp)                    |
| godot_remote_h264_enabled            | yes           | Enable H264 codec integration. (default codec is OpenH264)                               |
| godot_remote_use_sse2                | yes           | Use SSE2 to convert YUV to RGB for the H264 codec. Only on PC and without libjpeg-turbo. |
| godot_remote_tracy_enabled           | no            | Enable tracy profiler integration.                                                       |

### Usage example

```scons platform=windows target=release -j8 godot_remote_h264_enabled=no godot_remote_libjpeg_turbo_enabled=no```

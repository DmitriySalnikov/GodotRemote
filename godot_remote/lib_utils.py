from shutil import copyfile
import os
import pathlib
current_dir = '#modules/godot_remote/'

def prepare_turbo_jpeg(env):
    base_dir = '#modules/godot_remote/libjpeg-turbo/lib/'
    dir = base_dir
    lib = ''
    need_to_use_hack = False

    # Windows libs
    if env['platform'] == 'windows':
        env.Append(LIBS=['turbojpeg-static'])
        if env['bits'] == '32':
            dir += 'win/x86/'
        elif env['bits'] == '64':
            dir += 'win/x64/'
        lib = dir + 'turbojpeg-static.lib'
        need_to_use_hack = True
    
    # Linux libs
    elif env['platform'] == 'x11':
        env.Append(LIBS=['libturbojpeg'])
        if env['bits'] == '32':
            dir += 'linux/x86/'
        elif env['bits'] == '64':
            dir += 'linux/x64/'
        lib = dir + 'libturbojpeg.a'
        need_to_use_hack = True
    
    # Android libs
    elif env['platform'] == 'android':
        env.Append(LIBS=['libturbojpeg'])
        if env['android_arch'] == 'arm64v8':
            dir += 'android/arm64-v8a/'
        elif env['android_arch'] == 'armv7':
            dir += 'android/armeabi-v7a/'
        elif env['android_arch'] == 'x86':
            dir += 'android/x86/'
        elif env['android_arch'] == 'x86_64':
            dir += 'android/x86_64/'

        env.Append(LIBPATH=[dir])
        lib = dir + 'libturbojpeg.a'

    if need_to_use_hack:
        tmp_dir = base_dir + 'temp_folder_for_lib_copies/'
        new_dir = dir.replace(base_dir, tmp_dir).replace(current_dir, '')
        new_file_name = (os.path.splitext(lib)[0] + env['LIBSUFFIX']).replace(base_dir, tmp_dir)
        env.Append(LIBPATH=[pathlib.Path(new_file_name).parent])
        new_file_name = new_file_name.replace(current_dir, '')

        if not os.path.exists(new_dir):
            os.makedirs(new_dir)
        copyfile(lib.replace(current_dir, ''), new_file_name)

# orinal code from godot 3.x https://github.com/godotengine/godot/commit/6d022f813f2ee00dbde98946e596183ad67c0411
def prepare_h264(env):
    lib_version = "openh264-2.1.1"

    if env["platform"] == "android":
        lib_vars = {"armv7":["-android-arm.so", "armeabi-v7a"], "arm64v8":["-android-arm64.so", "arm64-v8a"], "x86":["-android-x86.so", "x86"], "x86_64":["-android-x64.so", "x86_64"]}
        lib_arch_dir = ""
        lib_openh264 = ""
        if env["android_arch"] in lib_vars.keys():
            lib_openh264 = "lib" + lib_version + lib_vars[env["android_arch"]][0]
            lib_arch_dir = lib_vars[env["android_arch"]][1]
        else:
            print("WARN: Architecture not suitable for embedding into APK; keeping .so at \\bin")

        if lib_arch_dir != "":
            if env["target"] == "release":
                lib_type_dir = "release"
            else:  # release_debug, debug
                lib_type_dir = "debug"

            new_dir = "../../platform/android/java/lib/libs/" + lib_type_dir + "/" + lib_arch_dir
            old_lib = "openh264/" + lib_openh264
            if not os.path.exists(new_dir):
                os.makedirs(new_dir)
            copyfile(old_lib, new_dir + "/libopenh264.so")
    elif env["platform"] in ["windows", "x11", "linuxbsd", "osx"]:
        lib_openh264 = ""
        if env["platform"] == "windows":
            lib_openh264 = lib_version + ("-win64.dll" if env["bits"] == "64" else "-win32.dll")
        elif env["platform"] in ["x11", "linuxbsd"]:
            lib_openh264 = "lib" + lib_version + ("-linux64.6.so" if env["bits"] == "64" else "-linux32.6.so")

        new_dir = "../../bin/" 
        old_lib = "openh264/" + lib_openh264
        if not os.path.exists(new_dir):
            os.makedirs(new_dir)
        copyfile(old_lib, new_dir + lib_openh264)
        lib_openh264 = ""

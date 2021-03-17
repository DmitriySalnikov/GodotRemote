from shutil import copyfile
import os
import pathlib

def prepare_lib(env):
    current_dir = '#modules/godot_remote/'
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
        new_dir = dir.replace(base_dir, base_dir + 'temp_folder_for_lib_copies/').replace(current_dir, '')
        new_file_name = (os.path.splitext(lib)[0] + env['LIBSUFFIX']).replace(base_dir, base_dir + 'temp_folder_for_lib_copies/')
        env.Append(LIBPATH=[pathlib.Path(new_file_name).parent])
        new_file_name = new_file_name.replace(current_dir, '')

        if not os.path.exists(new_dir):
            os.makedirs(new_dir)
        copyfile(lib.replace(current_dir, ''), new_file_name)
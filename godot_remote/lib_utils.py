#!/usr/bin/env python3

from shutil import copyfile
import os
import json

# 'headers_dir' option in SConstruct must be correct: 'godot-cpp/godot-headers'
# 'binding_generator' should be commented out or replaced by 'pass'

# in GDNative this method must be placed instead of Help generation line
def setup_options(env, opts, gen_help, is_gdnative = False):
    from SCons.Variables import BoolVariable, EnumVariable

    opts.Add(BoolVariable("godot_remote_tracy_enabled", "Godot Remote. Enable tracy profiler.", False))
    opts.Add(BoolVariable("godot_remote_libjpeg_turbo_enabled", "Godot Remote. Enable libjpeg-turbo.", True))
    opts.Add(BoolVariable("godot_remote_h264_enabled", "Godot Remote. Enable OpenH264 codec.", True))
    opts.Add(BoolVariable("godot_remote_no_default_resources", "Godot Remote. Remove default resources.", False))
    opts.Add(BoolVariable("godot_remote_disable_server", "Godot Remote. Remove server.", False))
    opts.Add(BoolVariable("godot_remote_disable_client", "Godot Remote. Remove client.", False))
    opts.Add(BoolVariable("godot_remote_use_sse2", "Godot Remote. Use SSE2 to convert YUV to RGB for the H264 codec. Only on PC and without libjpeg-turbo.", True))
    opts.Add(BoolVariable("godot_remote_auto_connection_enabled", "Godot Remote. Enable auto connection mode using UDP broadcasting.", True))
    #opts.Add(BoolVariable("godot_remote_livepp", "Godot Remote. Live++ support... Windows only", False))
    opts.Update(env)

    gen_help(env)

    if env['platform'] not in ['windows', 'x11', 'linuxbsd', 'osx']:
        env['godot_remote_use_sse2'] = False
    
    if not is_gdnative:
        setup_default_cpp_defines(env)

def setup_default_cpp_defines(env):
    if env['godot_remote_no_default_resources']:
        env.Append(CPPDEFINES=['NO_GODOTREMOTE_DEFAULT_RESOURCES'])
    if env['godot_remote_disable_server']:
        env.Append(CPPDEFINES=['NO_GODOTREMOTE_SERVER'])
    if env['godot_remote_disable_client']:
        env.Append(CPPDEFINES=['NO_GODOTREMOTE_CLIENT'])
    if env['godot_remote_libjpeg_turbo_enabled']:
        env.Append(CPPDEFINES=['GODOT_REMOTE_LIBJPEG_TURBO_ENABLED'])
    if env['godot_remote_use_sse2']:
        env.Append(CPPDEFINES=['GODOT_REMOTE_USE_SSE2'])
    if env['godot_remote_auto_connection_enabled']:
        env.Append(CPPDEFINES=['GODOT_REMOTE_AUTO_CONNECTION_ENABLED'])

######
# gdnative
######

# src_files variable must be declared near gdnative_specific_params
def gdnative_get_sources(src):
    return ["godot_remote/" + file for file in src]

# this can replace everything after 'sources = []' by Default(gdnative_get_library_object(env))
def gdnative_get_library_object(env):
    setup_default_cpp_defines(env)
    env.Append(CPPDEFINES=['GDNATIVE_LIBRARY'])

    #######################################################
    # platform specific

    if env['platform'] == 'linux' or env['platform'] == 'freebsd':
        env['SHLIBSUFFIX'] = '.so'

    elif env['platform'] == 'osx':
        env['SHLIBSUFFIX'] = '.dylib'

    elif env['platform'] == 'ios':
        env['SHLIBSUFFIX'] = '.dylib'

    elif env['platform'] == 'windows':
        env['SHLIBSUFFIX'] = '.dll'

    elif env['platform'] == 'android':
        env['SHLIBSUFFIX'] = '.so'
        
        if env['target'] == 'debug':
            env.Append(CCFLAGS=['-Og'])
        elif env['target'] == 'release':
            env.Append(CCFLAGS=['-O3'])
    
    #######################################################
    
    arch_suffix = env['bits']
    if env['platform'] == 'android':
        arch_suffix = env['android_arch']
    if env['platform'] == 'ios':
        arch_suffix = env['ios_arch']

    env.Append(CPPPATH=[
        'godot-cpp/godot-headers',
        'godot_remote',
        'godot-cpp/include',
        'godot-cpp/include/gen',
        'godot-cpp/include/core',
    ])

    src = []
    with open('godot_remote/default_sources.json') as f:
        src = json.load(f)

    #######################################################
    # additional data for specific options
    src.append('GRToolMenuPlugin.cpp')

    # Tracy
    if env['godot_remote_tracy_enabled']:
        src.append('tracy/TracyClient.cpp')
        env.Append(CPPDEFINES=['GODOTREMOTE_TRACY_ENABLED', 'TRACY_ENABLE', 'TRACY_ON_DEMAND', 'TRACY_DELAYED_INIT', 'TRACY_MANUAL_LIFETIME'])
        #module_env.Append(CPPDEFINES=['TRACY_CALLSTACK'])

    # libturbo-jpeg
    if env['godot_remote_libjpeg_turbo_enabled']:
        # needed for windows precompiled lib
        if env['platform'] == 'windows':
            env.Append(LINKFLAGS=['/NODEFAULTLIB:LIBCMT'])
        
        # an absolutely stupid system that doesn't allow me to link libraries 
        # inside this module without 'LIBSUFFIX' required this hack
        prepare_turbo_jpeg(env, True)
    else:
        src.append('jpge.cpp')

    # OpenH264
    if env['godot_remote_h264_enabled']:
        env.Append(CPPDEFINES=['GODOTREMOTE_H264_ENABLED'])
        #prepare_h264(env)

    #######################################################

    env.Append(LIBPATH=['#godot-cpp/bin/'])
    env.Append(LIBS=[
            'libgodot-cpp.{}.{}.{}{}'.format( # godot-cpp lib
            env['platform'],
            env['target'],
            arch_suffix,
            env['LIBSUFFIX']),
    ])

    return env.SharedLibrary(
        target='#bin/' + 'godot_remote.{}.{}.{}{}'.format(
        env['platform'],
        env['target'],
        arch_suffix,
        env['SHLIBSUFFIX']
        #env['LIBSUFFIX']
        ), source=gdnative_get_sources(src)
    )

######
# only for regular module
######

# original code from godot 3.x https://github.com/godotengine/godot/commit/6d022f813f2ee00dbde98946e596183ad67c0411
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

######
# for both types
######

# 'CXXFLAGS' 'CCFLAGS' 'LINKFLAGS' and others
def remove_flag_from_env(env, flags_group, flags):
    for f in flags:
        if f in env[flags_group]:
            env[flags_group].remove(f)

def prepare_turbo_jpeg(env, is_gdnative = False):
    current_dir = '#modules/godot_remote/'
    base_dir = '#modules/godot_remote/libjpeg-turbo/lib/' if not is_gdnative else '#godot_remote/libjpeg-turbo/lib/'
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
    elif env['platform'] in ['x11', 'linux', 'freebsd']:
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

        lib = dir + 'libturbojpeg.a'

    if need_to_use_hack and not is_gdnative:
        if env['platform'] == 'windows':
            import pathlib
        tmp_dir = base_dir + 'temp_folder_for_lib_copies/'
        new_dir = dir.replace(base_dir, tmp_dir).replace(current_dir, '')
        new_file_name = (os.path.splitext(lib)[0] + env['LIBSUFFIX']).replace(base_dir, tmp_dir)
        env.Append(LIBPATH=[new_file_name[:new_file_name.rfind('/')]])
        new_file_name = new_file_name.replace(current_dir, '')

        if not os.path.exists(new_dir):
            os.makedirs(new_dir)
        copyfile(lib.replace(current_dir, ''), new_file_name)
    else:
        env.Append(LIBPATH=[dir])

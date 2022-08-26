#!/usr/bin/env python3

from godot_remote import lib_utils
env = SConscript("godot-cpp/SConstruct")
opts = Variables([], ARGUMENTS)
lib_utils.setup_options(env, opts, lambda e: Help(opts.GenerateHelpText(e)), True)
Default(lib_utils.gdnative_get_library_object(env))

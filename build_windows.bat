cd godot-cpp
git apply --ignore-space-change --ignore-whitespace ../patches/godot_cpp_trim_unused_classes.patch
cd ..
scons platform=windows bits=64 target=release godot_remote_custom_init_for_trimmed_godot_cpp=yes
scons platform=windows bits=64 target=debug godot_remote_custom_init_for_trimmed_godot_cpp=yes
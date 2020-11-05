cd godot-cpp
scons generate_bindings=true platform=windows bits=64 -j8
scons platform=android target=debug android_arch=arm64v8 -j8

scons platform=android target=release android_arch=arm64v8 -j8
scons platform=android target=release android_arch=armv7 -j8
scons platform=android target=release android_arch=x86 -j8
scons platform=android target=release android_arch=x86_64 -j8
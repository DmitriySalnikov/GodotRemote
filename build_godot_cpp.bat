cd godot-cpp
set api=custom_api_file="../api.json"

scons generate_bindings=true platform=windows target=debug bits=64 -j8 %api%
scons platform=windows target=release bits=64 -j8 %api%

scons platform=android target=debug android_arch=arm64v8 -j8 %api%

scons platform=android target=release android_arch=arm64v8 -j8 %api%
scons platform=android target=release android_arch=armv7 -j8 %api%
scons platform=android target=release android_arch=x86 -j8 %api%
scons platform=android target=release android_arch=x86_64 -j8 %api%
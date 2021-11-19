cd godot-cpp
set cpu=%NUMBER_OF_PROCESSORS%
::set api=custom_api_file="../api.json"
set api= 

scons generate_bindings=yes platform=windows target=release bits=64 -j%cpu% %api%
scons platform=windows target=release bits=64 -j%cpu% %api%
scons platform=windows target=debug bits=64 -j%cpu% %api%

scons platform=android target=debug android_arch=arm64v8 -j%cpu% %api%

scons platform=android target=release android_arch=arm64v8 -j%cpu% %api%
scons platform=android target=release android_arch=armv7 -j%cpu% %api%
scons platform=android target=release android_arch=x86 -j%cpu% %api%
scons platform=android target=release android_arch=x86_64 -j%cpu% %api%
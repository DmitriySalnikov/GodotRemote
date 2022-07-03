cd godot-cpp
set cpu=%NUMBER_OF_PROCESSORS%
::set api=custom_api_file="../api.json"
set api= 

git apply ../godot_cpp_trim_unused_classes.patch

title win x64 release
scons platform=windows target=release bits=64 -j%cpu% %api% generate_bindings=yes
if errorlevel 1 ( echo Failed to generate godot-cpp source code. Code: %errorlevel% && exit /b %errorlevel% )

title win x64 debug
scons platform=windows target=debug bits=64 -j%cpu% %api%
if errorlevel 1 ( echo Failed to compile debug Windows godot-cpp for x64. Code: %errorlevel% && exit /b %errorlevel% )

title win x86
scons platform=windows target=release bits=32 -j%cpu% %api%
if errorlevel 1 ( echo Failed to compile Windows godot-cpp for x64. Code: %errorlevel% && exit /b %errorlevel% )


title android arm64v8 debug
scons platform=android target=debug android_arch=arm64v8 -j%cpu% %api%
if errorlevel 1 ( echo Failed to compile debug Android godot-cpp for arm64v8. Code: %errorlevel% && exit /b %errorlevel% )


title android arm64v8
scons platform=android target=release android_arch=arm64v8 -j%cpu% %api%
if errorlevel 1 ( echo Failed to compile Android godot-cpp for arm64v8. Code: %errorlevel% && exit /b %errorlevel% )

title android armv7
scons platform=android target=release android_arch=armv7 -j%cpu% %api%
if errorlevel 1 ( echo Failed to compile Android godot-cpp for armv7. Code: %errorlevel% && exit /b %errorlevel% )

title android x86
scons platform=android target=release android_arch=x86 -j%cpu% %api%
if errorlevel 1 ( echo Failed to compile Android godot-cpp for x86. Code: %errorlevel% && exit /b %errorlevel% )

title android x86_64
scons platform=android target=release android_arch=x86_64 -j%cpu% %api%
if errorlevel 1 ( echo Failed to compile Android godot-cpp for x86_64. Code: %errorlevel% && exit /b %errorlevel% )


cd ..
where /Q wsl
IF ERRORLEVEL 1 (
	@echo wsl not found
) ELSE (
	del /Q "godot-cpp\.sconsign.dblite"
	wsl -d Ubuntu-18.04 bash build_godot_cpp_linux.sh
	if errorlevel 1 ( echo Failed to compile Linux godot-cpp for x64. Code: %errorlevel% && exit /b %errorlevel% )
)
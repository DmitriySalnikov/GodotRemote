# Android.mk
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := godot-cpp
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := godot-cpp/bin/libgodot-cpp.android.release.x86.a
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := godot-cpp/bin/libgodot-cpp.android.release.x86_64.a
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := godot-cpp/bin/libgodot-cpp.android.release.armv7.a
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := godot-cpp/bin/libgodot-cpp.android.release.arm64v8.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := godot_remote.android.release.$(TARGET_ARCH_ABI)
LOCAL_CPPFLAGS := -std=c++14
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_LDLIBS := -llog 

LOCAL_SRC_FILES := \
godot_remote/GodotRemote.cpp \
godot_remote/GRClient.cpp \
godot_remote/GRDevice.cpp \
godot_remote/GRInputData.cpp \
godot_remote/GRNotifications.cpp \
godot_remote/GRPacket.cpp \
godot_remote/GRResources.cpp \
godot_remote/GRServer.cpp \
godot_remote/GRUtils.cpp \
godot_remote/jpge.cpp \
godot_remote/register_types.cpp

LOCAL_C_INCLUDES := \
godot-cpp/godot-headers \
godot-cpp/include/ \
godot-cpp/include/core \
godot-cpp/include/gen \

LOCAL_STATIC_LIBRARIES := godot-cpp

include $(BUILD_SHARED_LIBRARY)
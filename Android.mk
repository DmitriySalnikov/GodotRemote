# Android.mk

# TODO outdated
# Need to add openh264, libjpeg-turbo and tracy libs integrations

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
LOCAL_CFLAGS := -DGODOT_REMOTE_AUTO_CONNECTION_ENABLED

LOCAL_SRC_FILES := \
godot_remote/GodotRemote.cpp \
godot_remote/GRAVGCounter.cpp \
godot_remote/GRClient.cpp \
godot_remote/GRDevice.cpp \
godot_remote/GRInputData.cpp \
godot_remote/GRNotifications.cpp \
godot_remote/GRObjectPool.cpp \
godot_remote/GRPacket.cpp \
godot_remote/GRProfiler.cpp \
godot_remote/GRProfilerViewportMiniPreview.cpp \
godot_remote/GRResources.cpp \
godot_remote/GRServer.cpp \
godot_remote/GRStreamDecoders.cpp \
godot_remote/GRStreamDecoderH264.cpp \
godot_remote/GRStreamDecoderImageSequence.cpp \
godot_remote/GRStreamEncoders.cpp \
godot_remote/GRStreamEncoderH264.cpp \
godot_remote/GRStreamEncoderImageSequence.cpp \
godot_remote/GRUtils.cpp \
godot_remote/GRUtilsH264Codec.cpp \
godot_remote/GRUtilsJPGCodec.cpp \
godot_remote/GRViewportCaptureRect.cpp \
godot_remote/UDPSocket.cpp \
godot_remote/iterable_queue.cpp \
godot_remote/register_types.cpp 

LOCAL_C_INCLUDES := \
godot-cpp/godot-headers \
godot-cpp/include/ \
godot-cpp/include/core \
godot-cpp/include/gen \

LOCAL_STATIC_LIBRARIES := godot-cpp

include $(BUILD_SHARED_LIBRARY)
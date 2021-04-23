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
GodotRemote.cpp \
GRAVGCounter.cpp \
GRClient.cpp \
GRDevice.cpp \
GRInputData.cpp \
GRNotifications.cpp \
GRObjectPool.cpp \
GRPacket.cpp \
GRProfiler.cpp \
GRProfilerViewportMiniPreview.cpp \
GRResources.cpp \
GRServer.cpp \
GRStreamDecoders.cpp \
GRStreamDecoderH264.cpp \
GRStreamDecoderImageSequence.cpp \
GRStreamEncoders.cpp \
GRStreamEncoderH264.cpp \
GRStreamEncoderImageSequence.cpp \
GRUtils.cpp \
GRUtilsH264Codec.cpp \
GRUtilsJPGCodec.cpp \
GRViewportCaptureRect.cpp \
UDPSocket.cpp \
iterable_queue.cpp \
register_types.cpp 

LOCAL_C_INCLUDES := \
godot-cpp/godot-headers \
godot-cpp/include/ \
godot-cpp/include/core \
godot-cpp/include/gen \

LOCAL_STATIC_LIBRARIES := godot-cpp

include $(BUILD_SHARED_LIBRARY)
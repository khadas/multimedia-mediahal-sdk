ifeq (,$(wildcard media_hal))
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:=videodec_test
LOCAL_SRC_FILES := VideoDecTest.cpp
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_SHARED_LIBRARIES := libutils libbase libbinder libcutils libui \
		libgui \
		liblog \
		libstagefright \
		libstagefright_foundation \
		libhidlbase \
		android.hardware.graphics.allocator@2.0 \
		android.hardware.graphics.bufferqueue@1.0 \
		android.hardware.graphics.mapper@2.0 \
		libstagefright_bufferqueue_helper \
                libamgralloc_ext@2

LOCAL_STATIC_LIBRARIES := libgtest
LOCAL_C_INCLUDES:= system/core/libion/include \
		external/libchrome \
		$(LOCAL_PATH)/../../include \

include $(BUILD_EXECUTABLE)
endif

ifeq (,$(wildcard media_hal))
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := AmTsPlayerExample
LOCAL_SRC_FILES := AmTsPlayerExample.cpp \

LOCAL_SHARED_LIBRARIES := libmediahal_tsplayer.system

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include \

include $(BUILD_EXECUTABLE)
endif

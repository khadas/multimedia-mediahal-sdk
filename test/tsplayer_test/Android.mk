ifeq (,$(wildcard media_hal))
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:=AmTsPlayerTest
LOCAL_SRC_FILES := AmTsPlayerTest.cpp \
                   AmTsPlayerSession.cpp \


LOCAL_SHARED_LIBRARIES := libmediahal_tsplayer.system

LOCAL_STATIC_LIBRARIES := libgtest
LOCAL_C_INCLUDES:= system/core/libion/include \
		external/libchrome \
		$(LOCAL_PATH)/../../include \
                $(LOCAL_PATH)/include \

include $(BUILD_EXECUTABLE)
endif

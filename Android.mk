# THIS FILE IS AUTOGENERATED, DO NOT EDIT
# Generated from Android.bp.in, run ./release_tool.sh to regenerate
ifeq (,$(wildcard media_hal))
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libmediahal_videodec
LOCAL_MULTILIB := 32
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/
LOCAL_SRC_FILES_arm := lib/vendor/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libmediahal_videodec.system
LOCAL_MULTILIB := 32
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT)/lib/
LOCAL_SRC_FILES_arm := lib/system/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libmediahal_tsplayer
LOCAL_MULTILIB := 32
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/
LOCAL_SRC_FILES_arm := lib/vendor/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libmediahal_tsplayer.system
LOCAL_MULTILIB := 32
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT)/lib/
LOCAL_SRC_FILES_arm := lib/system/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

endif


LOCAL_PATH:= $(call my-dir)

LOCAL_CFLAGS += -Wall -Werror

#--------------------------------------------
#          Build default_volume_control LIB
#--------------------------------------------
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    defaultPluginControls.cpp

LOCAL_CFLAGS += -O2 -fvisibility=hidden

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libdl \
    libexpat \
    libar-pal

LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/session/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/stream/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/resource_manager/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/device/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/utils/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/context_manager/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/ar/ar_osal
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/ar/spf/api/apm
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/ar/spf/api/vcpm
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/acdbdata/
LOCAL_C_INCLUDES += $(TOP)/system/media/audio_route/include
LOCAL_C_INCLUDES += $(TOP)/system/media/audio/include
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/agm
LOCAL_C_INCLUDES += $(TOP)/external/expat/lib/expat.h

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/techpack/audio/include
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr


#if android version is R, use qtitinyxxx headers & libs, otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_C_INCLUDES       += $(TOP)/vendor/qcom/opensource/tinyalsa/include
LOCAL_C_INCLUDES       += $(TOP)/vendor/qcom/opensource/tinycompress/include
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa libqti-tinycompress
else
LOCAL_C_INCLUDES       += $(TOP)/vendor/qcom/opensource/tinycompress/include
LOCAL_SHARED_LIBRARIES += libtinyalsa libqti-tinycompress
endif

LOCAL_HEADER_LIBRARIES := \
    libspf-headers \
    libagm_headers \
    libarosal_headers \
    libaudiologutils_headers \
    libacdb_headers \
    liblisten_headers \
    libar-gsl_headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := lib_default_plugin_controls
LOCAL_MODULE_OWNER := qti
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)

#--------------------------------------------
#          Build default_set_param_control LIB
#--------------------------------------------
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    defaultShmemPluginControls.cpp

LOCAL_CFLAGS += -O2 -fvisibility=hidden

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libdl \
    libexpat \
    libar-pal

#if android version is R, use qtitinyxxx headers & libs, otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/session/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/stream/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/resource_manager/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/device/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/utils/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/context_manager/inc
LOCAL_C_INCLUDES += $(TOP)/system/media/audio_route/include
LOCAL_C_INCLUDES += $(TOP)/system/media/audio/include

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libarosal_headers \
    libaudiologutils_headers \
    libacdb_headers \
    liblisten_headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := lib_default_set_param_plugin_controls
LOCAL_MODULE_OWNER := qti
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)

#--------------------------------------------
#          Build audio_volume LIB
#--------------------------------------------


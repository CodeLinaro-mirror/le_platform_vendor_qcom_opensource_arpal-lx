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
    libar-pal \
    libqti-tinyalsa

LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/session/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/stream/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/resource_manager/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/device/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/utils/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/ar/ar_osal
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/ar/gsl
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/ar/spf/api/apm
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/ar/spf/api/vcpm
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/acdbdata/
LOCAL_C_INCLUDES += $(TOP)/system/media/audio_route/include
LOCAL_C_INCLUDES += $(TOP)/system/media/audio/include
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/tinyalsa/include
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/tinycompress/include
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/include/mm-audio/agm
LOCAL_C_INCLUDES += $(TOP)/external/expat/lib/expat.h

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/techpack/audio/include



LOCAL_HEADER_LIBRARIES := \
    libspf-headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := lib_default_plugin_controls
LOCAL_MODULE_OWNER := qti
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)

#--------------------------------------------
#          Build audio_volume LIB
#--------------------------------------------


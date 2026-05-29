LOCAL_PATH:= $(call my-dir)

LOCAL_CFLAGS += -Wall -Werror

#--------------------------------------------
#          Build default_volume_control LIB
#--------------------------------------------
include $(CLEAR_VARS)

LOCAL_CFLAGS   += -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2

LOCAL_SRC_FILES := \
    defaultPluginControls.cpp

LOCAL_CFLAGS += -O2 -fvisibility=hidden

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libdl \
    libexpat \
    libar-pal \
    libsession_ar

LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/session/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/session/SessionAR/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/stream/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/resource_manager/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/device/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/utils/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/context_manager/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/plugins/PluginManager/inc
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

# Use flag based selection to use QTI vs open source tinycompress project
ifeq ($(TARGET_USES_QTI_TINYCOMPRESS),true)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa libqti-tinycompress
else
LOCAL_C_INCLUDES       += $(TOP)/vendor/qcom/opensource/tinycompress/include
LOCAL_SHARED_LIBRARIES += libqti-tinycompress
ifneq (,$(filter gen4_gvm_gy gen5_gvm gen5_gvm_gy auto_gen_prime, $(TARGET_BOARD_PLATFORM)$(TARGET_BOARD_SUFFIX)$(TARGET_BOARD_DERIVATIVE_SUFFIX)))
LOCAL_SHARED_LIBRARIES += libtinyalsav2
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif
endif

LOCAL_HEADER_LIBRARIES := \
    libspf-headers \
    libagm_headers \
    libarosal_headers \
    libaudiologutils_headers \
    libacdb_headers \
    liblisten_headers \
    libarvui_intf_headers \
    libsession_ar_headers \
    libaudiofeaturestats_headers \
    plugin_manager_headers \
    libsession_voice_headers

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
    libar-pal \
    libqti-tinyalsa

LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/session/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/stream/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/resource_manager/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/device/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/utils/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/context_manager/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/qcom/opensource/pal/plugins/PluginManager/inc
LOCAL_C_INCLUDES += $(TOP)/system/media/audio_route/include
LOCAL_C_INCLUDES += $(TOP)/system/media/audio/include

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libarosal_headers \
    libaudiologutils_headers \
    libacdb_headers \
    libaudiofeaturestats_headers \
    libspf-headers \
    libarvui_intf_headers \
    libsession_ar_headers \
    plugin_manager_headers \
    liblisten_headers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := lib_default_set_param_plugin_controls
LOCAL_MODULE_OWNER := qti
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)

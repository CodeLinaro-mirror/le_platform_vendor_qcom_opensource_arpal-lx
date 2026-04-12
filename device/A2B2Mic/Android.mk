LOCAL_PATH := $(call my-dir)

#-------------------------------------------
#            Build DEVICE_A2B2MIC LIB
#-------------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE        := libdev_a2b2mic
LOCAL_MODULE_OWNER  := qti
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS   += -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
LOCAL_CPPFLAGS += -fexceptions -frtti

LOCAL_SRC_FILES := \
    src/A2B2Mic.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc

LOCAL_C_INCLUDES += $(TOP)/system/media/audio_route/include
LOCAL_C_INCLUDES += $(TOP)/system/media/audio/include

LOCAL_HEADER_LIBRARIES := \
    libarpal_headers \
    libspf-headers \
    libagm_headers \
    libacdb_headers \
    liblisten_headers \
    libarosal_headers \
    libaudiofeaturestats_headers \
    libarvui_intf_headers \
    libarmemlog_headers \
    libarpal_internalheaders \
    libsession_ar_headers

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    liblx-osal \
    libar-pal \
    libexpat \
    libutilscallstack \
    libsession_ar

ifeq ($(TARGET_USES_QTI_TINYCOMPRESS),true)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa libqti-tinycompress
else
LOCAL_C_INCLUDES       += $(TOP)/vendor/qcom/opensource/tinycompress/include
LOCAL_SHARED_LIBRARIES += libqti-tinycompress
ifneq (,$(filter gen4_gvm_gy gen5_gvm gen5_gvm_gy, $(TARGET_BOARD_PLATFORM)$(TARGET_BOARD_SUFFIX)$(TARGET_BOARD_DERIVATIVE_SUFFIX)))
LOCAL_SHARED_LIBRARIES += libtinyalsav2
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif
endif

ifeq ($(USE_PAL_STATIC_LINKING_MODULES),true)
    include $(BUILD_STATIC_LIBRARY)
else
    include $(BUILD_SHARED_LIBRARY)
endif

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpalclient
LOCAL_MODULE_OWNER := qti
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := \
    src/pal_client_wrapper.cpp

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libutils \
    liblog \
    libcutils \
    libfmq \
    libhardware \
    libbase \
    vendor.qti.hardware.pal@1.0 \
    android.hidl.allocator@1.0 \
    android.hidl.memory@1.0 \
    libhidlmemory

LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libarpal_headers
LOCAL_HEADER_LIBRARIES := libarpal_headers
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/palcshmtest/inc \

LOCAL_MODULE               := palcshm_test
LOCAL_MODULE_OWNER         := qti
LOCAL_MODULE_TAGS          := optional

LOCAL_CPPFLAGS := -Wno-macro-redefined
LOCAL_CPPFLAGS += -D_GNU_SOURCE
LOCAL_CPPFLAGS += -fexceptions
LOCAL_CPPFLAGS += -std=c++14 -g -O0
LOCAL_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc/

LOCAL_SRC_FILES        := palcshmtest/src/palcshm_test.cpp\

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libutils \
    liblog \
    libcutils \
    libfmq \
    libhardware \
    libbase \
    vendor.qti.hardware.pal@1.0 \
    vendor.qti.hardware.paleventnotifier@1.0 \
    android.hidl.allocator@1.0 \
    android.hidl.memory@1.0 \
    libhidlmemory \
    libjsoncpp

LOCAL_HEADER_LIBRARIES := libarpal_headers \
                          libarosal_headers \
                          libspf-headers
LOCAL_VENDOR_MODULE := true
include $(BUILD_EXECUTABLE)

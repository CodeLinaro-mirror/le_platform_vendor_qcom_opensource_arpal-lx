/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <fmq/EventFlag.h>
#include <fmq/MessageQueue.h>
#include <utils/Thread.h>
#include "PalApi.h"
#include "shmem_client_module_protocol.h"
#include <fstream>
#include <iostream>
#include <string>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <fmq/EventFlag.h>
#include <fmq/MessageQueue.h>
#include <utils/Thread.h>
#include <log/log.h>
#include <vendor/qti/hardware/paleventnotifier/1.0/IPALEventNotifierCallback.h>
#include <vendor/qti/hardware/paleventnotifier/1.0/types.h>
#include <vendor/qti/hardware/pal/1.0/IPALCallback.h>

#define LOG_NDEBUG 0

#define CSHM_TC_LOG_ERR             (0x1) /**< error message, represents code bugs that should be debugged and fixed.*/
#define CSHM_TC_LOG_INFO            (0x2) /**< info message, additional info to support debug */
#define CSHM_TC_LOG_DBG             (0x4) /**< debug message, required at minimum for debug.*/
#define CSHM_TC_LOG_VERBOSE         (0x8)/**< verbose message, useful primarily to help developers debug low-level code */

extern uint32_t cshm_tc_log_lvl;
#define CSHM_TC_FATAL(log_tag, arg,...)                                       \
    if (cshm_tc_log_lvl & CSHM_TC_LOG_ERR) {                              \
        ALOGE("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
        abort();                                                  \
    }

#define CSHM_TC_ERR(log_tag, arg,...)                                          \
    if (cshm_tc_log_lvl & CSHM_TC_LOG_ERR) {                              \
        ALOGE("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
    }
#define CSHM_TC_DBG(log_tag,arg,...)                                           \
    if (cshm_tc_log_lvl & CSHM_TC_LOG_DBG) {                               \
        ALOGD("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__); \
    }
#define CSHM_TC_INFO(log_tag,arg,...)                                         \
    if (cshm_tc_log_lvl & CSHM_TC_LOG_INFO) {                             \
        ALOGI("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
    }
#define CSHM_TC_VERBOSE(log_tag,arg,...)                                      \
    if (cshm_tc_log_lvl & CSHM_TC_LOG_VERBOSE) {                          \
        ALOGD("%s: %d: "  arg, __func__, __LINE__, ##__VA_ARGS__);\
    }



/*PAL interface */
using PalMessageQueueFlagBits = ::vendor::qti::hardware::pal::V1_0::PalMessageQueueFlagBits;
using PalReadWriteDoneResult = ::vendor::qti::hardware::pal::V1_0::PalReadWriteDoneResult;
using PalReadWriteDoneCommand = ::vendor::qti::hardware::pal::V1_0::PalReadWriteDoneCommand;
using PalCallbackBuffer = ::vendor::qti::hardware::pal::V1_0::PalCallbackBuffer;
using PalCShmInfo = ::vendor::qti::hardware::pal::V1_0::PalCShmInfo;
using PalCShmType = ::vendor::qti::hardware::pal::V1_0::PalCShmType;
using PalCShmId = ::vendor::qti::hardware::pal::V1_0::PalCShmId;
using IPALCallback = ::vendor::qti::hardware::pal::V1_0::IPALCallback;
using PalStreamHandle = ::vendor::qti::hardware::pal::V1_0::PalStreamHandle;

/*Pal Notifier Interface */
using IPALEventNotifierCallback = ::vendor::qti::hardware::paleventnotifier::V1_0::IPALEventNotifierCallback;
using PalCallbackConfig_hidl = ::vendor::qti::hardware::paleventnotifier::V1_0::PalCallbackConfig;
using PalDeviceId = ::vendor::qti::hardware::paleventnotifier::V1_0::PalDeviceId;
using PalStreamAttributes = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamAttributes;
using PalStreamType = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamType;
using PalStreamInfo = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamInfo;
using PalStreamFlag = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamFlag;
using PalStreamDirection = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamDirection;
using PalMediaConfig = ::vendor::qti::hardware::paleventnotifier::V1_0::PalMediaConfig;
using PalChannelInfo = ::vendor::qti::hardware::paleventnotifier::V1_0::PalChannelInfo;
using PalAudioFmt = ::vendor::qti::hardware::paleventnotifier::V1_0::PalAudioFmt;

namespace vendor {
namespace qti {
namespace hardware {
namespace paleventnotifier {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::hardware::MessageQueue;
using ::android::hardware::EventFlag;
using ::android::Thread;
using ::android::status_t;

struct PALEventNotifierCallback : public IPALEventNotifierCallback {
    Return<void> onStart(const PalCallbackConfig_hidl& config) override;
	Return<void> onStop(const PalCallbackConfig_hidl& config) override;
    Return<void> onDeviceSwitch(const PalCallbackConfig_hidl& config) override;
    PALEventNotifierCallback(/*pal_audio_event_callback callBack*/)
    {
        ALOGE("PALEventNotifierCallback: called callback constructor");
    }
    ~PALEventNotifierCallback();
    //protected:
   //pal_audio_event_callback cb;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace pal
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

class server_death_notifier : public android::hardware::hidl_death_recipient
{
    public:
        server_death_notifier(){}
        void serviceDied(uint64_t cookie,
         const android::wp<::android::hidl::base::V1_0::IBase>& who) override ;
};

namespace vendor {
namespace qti {
namespace hardware {
namespace pal {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::hardware::MessageQueue;
using ::android::hardware::EventFlag;
using ::android::Thread;
using ::android::status_t;

struct PalCallback : public IPALCallback {
    typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;
    typedef MessageQueue<PalReadWriteDoneCommand, kSynchronizedReadWrite> CommandMQ;

    Return<int32_t> event_callback(uint64_t stream_handle,
                               uint32_t event_id, uint32_t event_data_size,
                               const hidl_vec<uint8_t>& event_data,
                               uint64_t cookie) override;
    Return<void> event_callback_rw_done(uint64_t stream_handle,
                               uint32_t event_id, uint32_t event_data_size,
                               const hidl_vec<PalCallbackBuffer>& event_data,
                               uint64_t cookie) override;
    Return<void> prepare_mq_for_transfer(uint64_t stream_handle,
                                    uint64_t cookie,
                                    prepare_mq_for_transfer_cb _hidl_cb) override;
    Return<void> ssr_event(uint8_t state, uint32_t subsystem, uint64_t cookie) override;

    PalCallback()
    {
		CSHM_TC_DBG(LOG_TAG, "PalCallback Constructor");
    }
    ~PalCallback();

    protected:
       pal_stream_callback cb;
       pal_global_callback gcb = nullptr;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace pal
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
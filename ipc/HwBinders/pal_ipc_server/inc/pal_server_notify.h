/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ANDROID_SYSTEM_paleventnotifier_V1_0_pal_H
#define ANDROID_SYSTEM_paleventnotifier_V1_0_pal_H

#include <vendor/qti/hardware/paleventnotifier/1.0/IPALEventNotifier.h>
#include <vendor/qti/hardware/paleventnotifier/1.0/IPALEventNotifierCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <fmq/EventFlag.h>
#include <fmq/MessageQueue.h>
#include <utils/Thread.h>
#include <utils/RefBase.h>
#include <cutils/list.h>
#include <mutex>
#include "PalApi.h"
#include <log/log.h>

using namespace android;

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
using IPALEventNotifierCallback = ::vendor::qti::hardware::paleventnotifier::V1_0::IPALEventNotifierCallback;
using PalCallbackConfig = ::vendor::qti::hardware::paleventnotifier::V1_0::PalCallbackConfig;
using PalDeviceId = ::vendor::qti::hardware::paleventnotifier::V1_0::PalDeviceId;
using PalStreamAttributes = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamAttributes;
using PalStreamType = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamType;
using PalStreamInfo = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamInfo;
using PalStreamFlag = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamFlag;
using PalStreamDirection = ::vendor::qti::hardware::paleventnotifier::V1_0::PalStreamDirection;
using PalMediaConfig = ::vendor::qti::hardware::paleventnotifier::V1_0::PalMediaConfig;
using PalChannelInfo = ::vendor::qti::hardware::paleventnotifier::V1_0::PalChannelInfo;
using PalAudioFmt = ::vendor::qti::hardware::paleventnotifier::V1_0::PalAudioFmt;

class NotifierClientDeathRecipient;

typedef struct {
    struct listnode list;
    uint32_t pid;
    android::sp<IPALEventNotifierCallback> pal_clbk;
    bool isnotified;
    std::vector<PalDeviceId> deviceID;
    std::vector<PalStreamType> streamType;
} client_info;

struct PALEventNotifier: public IPALEventNotifier {
public:
    PALEventNotifier()
    {
        sInstance = this;
    }

    static PALEventNotifier* getInstance()
    {
        return sInstance;
    }

    Return<int32_t> ipc_pal_notify_register_callback(const sp<IPALEventNotifierCallback>& cb, const hidl_vec<PalDeviceId>& devID, const hidl_vec<PalStreamType>& streamType) override;
    sp<NotifierClientDeathRecipient> mNotifierDeathRecipient;
	std::vector<std::shared_ptr<client_info>> mNotifierClients;
private:
    static PALEventNotifier* sInstance;
};

class NotifierClientDeathRecipient : public android::hardware::hidl_death_recipient {
    public:
        NotifierClientDeathRecipient(const sp<PALEventNotifier> NotifierInstance)
        : mNotifierInstance(NotifierInstance){}
        void serviceDied(uint64_t cookie,
         const android::wp<::android::hidl::base::V1_0::IBase>& who) override ;
    private:
       sp<PALEventNotifier> mNotifierInstance;
       std::mutex mLock;
};
}
}
}
}
}
}
#endif

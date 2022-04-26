/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.

 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/

#ifndef __POWER_STATE_MONITOR_IMPL_H__
#define __POWER_STATE_MONITOR_IMPL_H__

#include <utils/Mutex.h>
#include <utils/StrongPointer.h>
#include <vendor/qti/hardware/powerstateservice/1.0/types.h>
#include <vendor/qti/hardware/powerstateservice/1.0/IPowerStateService.h>
#include <vendor/qti/hardware/powerstateservice/1.0/IPowerStateCallback.h>

#include "PowerStateCommon.h"

using ::android::Mutex;
using ::android::sp;
using ::android::wp;
using ::android::hardware::Return;
using ::android::hardware::hidl_death_recipient;
using ::android::hidl::base::V1_0::IBase;
using ::vendor::qti::hardware::powerstateservice::V1_0::state;
using ::vendor::qti::hardware::powerstateservice::V1_0::IPowerStateService;
using ::vendor::qti::hardware::powerstateservice::V1_0::IPowerStateCallback;

class PowerStateMonitorImpl : public IPowerStateCallback {
public:
    PowerStateMonitorImpl();
    ~PowerStateMonitorImpl();

    static PowerStateMonitorImpl *getInstance();

    /* Overrides */
    Return<bool> notifyDeepSleepEvent(state parameter) override;
    Return<bool> notifyHibernateEvent(state parameter) override;

    void handlePowerStateServiceDeath(uint64_t cookie, const wp<IBase>& who);

    bool attachToPowerStateServiceHal();
    bool dettachFromPowerStateServiceHal();
    int getPowerStateMonitorDsState();

private:
    struct PowerStateServiceDeathRecipient : public hidl_death_recipient {
        public:
            explicit PowerStateServiceDeathRecipient(
                const android::sp<PowerStateMonitorImpl>& handler)
                : mHandler(handler) {
            }
            ~PowerStateServiceDeathRecipient() = default;
            void serviceDied(uint64_t cookie,
                               const wp<IBase>& who) override;
        private:
            sp<PowerStateMonitorImpl> mHandler;
    };
    sp<PowerStateServiceDeathRecipient> mPssDeathRecipient;

    Mutex mPssMutex;
    sp<IPowerStateService> mPssHandler GUARDED_BY(mPssMutex);
    std::string mPssAudioClient;
    int mPsmDsState;
};

#endif //__POWER_STATE_MONITOR_IMPL_H__

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

#define LOG_TAG "PAL: PowerStateMonitorImpl"

#include "PowerStateMonitorImpl.h"
#include <chrono>
#include <thread>
#include <utils/RefBase.h>
#include <cutils/log.h>

#define MAX_PSS_RETRY_COUNT    50
#define PSS_AUDIO_CLIENT_PREFIX    "audio_pal-"

static PowerStateMonitorImpl *mPsmHandler = nullptr;

/*
 * PowerStateService Death Recipient implementations
 */
void PowerStateMonitorImpl::PowerStateServiceDeathRecipient::serviceDied(
    uint64_t cookie, const wp<IBase>& who)
{
    mHandler->handlePowerStateServiceDeath(cookie, who);
}

void PowerStateMonitorImpl::handlePowerStateServiceDeath(
    uint64_t cookie, const wp<IBase>& who)
{
    ALOGD("PowerStateService HAL %p died. cookie: %llu, who: %p",
            mPssHandler.get(), static_cast<unsigned long long>(cookie), &who);

    Mutex::Autolock lock(mPssMutex);
    if (mPssHandler.get() != nullptr) {
        mPssHandler->unlinkToDeath(mPssDeathRecipient);
        ALOGD("Unlink %p from PSS death", mPssDeathRecipient.get());
        mPssHandler = nullptr;
    }
    /* TODO: Send notification to the client of PSM (SND MONITOR) */
}

/*
 * PowerStateMonitor implementations
 */
bool PowerStateMonitorImpl::attachToPowerStateServiceHal()
{
    bool retVal = false;
    sp<IPowerStateService> pss_handle = nullptr;
    int count = 0;

    /* Get PowerStateService handle */
    do {
        /* Get a power state HAL instance */
        pss_handle = IPowerStateService::tryGetService();
        if (pss_handle.get() == nullptr) {
            ALOGE("Unable to connect to powerStateService HAL, retry(%d) after 100ms", count);
            retVal = false;

            /* Sleep for power state HAL service to come-up before trying again */
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            count++;
        } else {
            ALOGD("Connected to powerStateService HAL %p", (void *)pss_handle.get());
            retVal = true;
        }

    } while (false == retVal && count < MAX_PSS_RETRY_COUNT);

    if (retVal == false) {
        ALOGE("Failed to connect to powerStateService HAL, Terminating...");
        return false;
    }

    Mutex::Autolock lock(mPssMutex);
    mPssHandler = pss_handle;

    /* Link to PowerStateService Death recipient */
    auto retl = mPssHandler->linkToDeath(mPssDeathRecipient, /*cookie=*/ 0);
    if (!retl.isOk() || retl == false) {
        ALOGE("Failed to link %p to powerStateService deathRecipient", (void *)mPssDeathRecipient.get());
        return false;
    } else {
        ALOGD("Linked %p to powerStateService deathRecipient", (void *)mPssDeathRecipient.get());
    }

    /* TODO:
     * PowerStateService (PSS) doesn't provide proper return value when client
     * is already registered. This is needed incase PAL restarted
     */
    mPssHandler->unregisterDeepSleepClient(mPsmHandler, mPssAudioClient.c_str());

    /* Register to PowerStateService */
    auto retr = mPssHandler->registerDeepSleepClient(mPsmHandler, mPssAudioClient.c_str());
    if (!retr.isOk() || retr == false) {
        ALOGE("Failed to register %s to powerStateService HAL", mPssAudioClient.c_str());
        mPssHandler->unlinkToDeath(mPssDeathRecipient);
        return false;
    } else {
        ALOGI("Registered %s to powerStateService HAL", mPssAudioClient.c_str());
    }

    return true;
}

bool PowerStateMonitorImpl::dettachFromPowerStateServiceHal()
{
    Mutex::Autolock lock(mPssMutex);
    if (mPssHandler.get() != nullptr) {
        ALOGI("Unregister %s from powerStateService HAL", mPssAudioClient.c_str());
        mPssHandler->unregisterDeepSleepClient(mPsmHandler, mPssAudioClient.c_str());
        ALOGD("Unlink %p from powerStateService deathRecipient", (void *)mPssDeathRecipient.get());
        mPssHandler->unlinkToDeath(mPssDeathRecipient);
        ALOGD("Disconnected from powerStateService HAL %p", (void *)mPssHandler.get());
        mPssHandler = nullptr;
    }

    return true;
}

int PowerStateMonitorImpl::getPowerStateMonitorDsState()
{
    return mPsmDsState;
}

Return<bool> PowerStateMonitorImpl::notifyDeepSleepEvent(state parameter)
{
    ALOGD("Power Service Deep Sleep Event: Parameter: %d", parameter);

    switch (parameter) {
        case state::DEEP_SLEEP_ENTER:
            ALOGD("DEEP_SLEEP_ENTER");
            mPsmDsState = 1;
            break;
        case state::DEEP_SLEEP_EXIT:
            ALOGD("DEEP_SLEEP_EXIT");
            mPsmDsState = 0;
            break;
        default:
            ALOGE("Deepsleep: Unkown");
    }
    return true;
}

Return<bool> PowerStateMonitorImpl::notifyHibernateEvent(state parameter) {

    ALOGD("Power Service Hibernate Event. Parameter: %d", parameter);

    switch (parameter) {
        case state::HIBERNATE_ENTER:
            ALOGD("HIBERNATE_ENTER");
            break;
        case state::HIBERNATE_EXIT:
            ALOGD("HIBERNATE_EXIT");
            break;
        default:
            ALOGE("Hibernate: Unkown");
    }
    return true;
}

PowerStateMonitorImpl::PowerStateMonitorImpl()
    : mPssDeathRecipient(new PowerStateServiceDeathRecipient(this))
{
    srand(getpid());
    mPssAudioClient = PSS_AUDIO_CLIENT_PREFIX + std::to_string(rand());

    mPsmDsState = 0;
}

PowerStateMonitorImpl *PowerStateMonitorImpl::getInstance()
{
    if(mPsmHandler == nullptr) {
        mPsmHandler = new PowerStateMonitorImpl();
    }
    return mPsmHandler;
}

PowerStateMonitorImpl::~PowerStateMonitorImpl()
{
    mPssAudioClient = "";
    mPsmDsState = 0;
}

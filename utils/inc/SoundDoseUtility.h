/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//Header file for Sound Dose impl in PAL.

#ifndef SOUND_DOSE_UTILITY_H
#define SOUND_DOSE_UTILITY_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include "PalDefs.h"
#include "apm_api.h"

class ResourceManager;

class SoundDoseUtility {
public:

    int startSoundDoseComputation(pal_device_id_t dev_id);
    void stopSoundDoseComputation(pal_device_id_t dev_id);

    //handle sound dose info
    static void handleSoundDoseCallback(uint64_t hdl, uint32_t event_id,
                                        void *event_data, uint32_t event_size);
    ~SoundDoseUtility();
    SoundDoseUtility(pal_device_id_t deviceId);

protected:

    static std::shared_ptr<ResourceManager> rm;
    std::vector<int> pcmDevIdRx;
    struct pal_device mDeviceAttr;
    struct mixer *virtMixer;
    struct pcm *rxPcm;
    pal_device_id_t mAssociatedDevId;   /**< Associated device id for which SoundDose is to be computed.*/
    /* Function to retreive any remaining MEL values cached in SPF before closing. */
    void getSoundDoseMelValues();

private:

    SoundDoseUtility(const SoundDoseUtility&) = delete;
    SoundDoseUtility& operator=(const SoundDoseUtility&) = delete;
};

#endif //SOUND_DOSE_UTILITY_H

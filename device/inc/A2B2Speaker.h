/*
 *Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef A2B2SPEAKER_H
#define A2B2SPEAKER_H

#include "Device.h"
#include "PalAudioRoute.h"

class A2B2Speaker : public Device
{
protected:
    static std::shared_ptr<Device> obj;
    A2B2Speaker(struct pal_device *device, std::shared_ptr<ResourceManager> Rm);
public:
    static std::shared_ptr<Device> getInstance(struct pal_device *device,
                                               std::shared_ptr<ResourceManager> Rm);
    static int32_t isSampleRateSupported(uint32_t sampleRate);
    static int32_t isChannelSupported(uint32_t numChannels);
    static int32_t isBitWidthSupported(uint32_t bitWidth);
    static std::shared_ptr<Device> getObject();
    int stop();
    A2B2Speaker();
    virtual ~A2B2Speaker();
};

#endif // A2B2SPEAKER_H

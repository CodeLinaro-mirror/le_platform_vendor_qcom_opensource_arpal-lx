/*
 *Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef A2BMIC_H
#define A2BMIC_H

#include "Device.h"
#include "PalAudioRoute.h"

class A2BMic : public Device
{
protected:
    static std::shared_ptr<Device> obj;
    A2BMic(struct pal_device *device, std::shared_ptr<ResourceManager> Rm);
public:
    static std::shared_ptr<Device> getInstance(struct pal_device *device,
                                               std::shared_ptr<ResourceManager> Rm);
    static int32_t isSampleRateSupported(uint32_t sampleRate);
    static int32_t isChannelSupported(uint32_t numChannels);
    static int32_t isBitWidthSupported(uint32_t bitWidth);
    static std::shared_ptr<Device> getObject();
    int stop();
    A2BMic();
    virtual ~A2BMic();
};

#endif // A2BMic_H

/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
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
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "PAL: SoundTriggerEngine"

#include "SoundTriggerEngine.h"

#include "StreamSoundTrigger.h"
#include "SoundTriggerPlatformInfo.h"

#include "PluginManager.h"

const std::map<uint32_t, std::string> smLUT {
    {ST_SM_ID_SVA_F_STAGE_GMM,    std::string{ "ST_SM_ID_SVA_F_STAGE_GMM" } },
    {ST_SM_ID_SVA_S_STAGE_PDK,    std::string{ "ST_SM_ID_SVA_S_STAGE_PDK" } },
    {ST_SM_ID_SVA_S_STAGE_RNN,    std::string{ "ST_SM_ID_SVA_S_STAGE_RNN" } },
    {ST_SM_ID_SVA_S_STAGE_USER,    std::string{ "ST_SM_ID_SVA_S_STAGE_USER" } },
    {ST_SM_ID_SVA_S_STAGE_UDK,    std::string{ "ST_SM_ID_SVA_S_STAGE_UDK" } },
};

std::shared_ptr<SoundTriggerEngine> SoundTriggerEngine::Create(
    StreamSoundTrigger *s,
    listen_model_indicator_enum type,
    st_module_type_t module_type,
    std::shared_ptr<VUIStreamConfig> sm_cfg)
{
    EngineCreate engineCreate = nullptr;
    int status = 0;
    void* plugin = nullptr;
    PAL_VERBOSE(LOG_TAG, "Enter, type %d", type);

    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream handle");
        return nullptr;
    }
    SoundTriggerEngine *engine = NULL;
    std::shared_ptr<PluginManager> pm;

    try {
        pm = PluginManager::getInstance();
        if(!pm){
            PAL_ERR(LOG_TAG, "unable to get plugin manager instance");
            return nullptr;
        }

        // Check if the sound model type exists in the map
        if (smLUT.find(type) == smLUT.end()) {
            PAL_ERR(LOG_TAG, "Invalid sound model type: %u", type);
            return nullptr;
        }

        status = pm->openPlugin(PAL_PLUGIN_MANAGER_SOUND_MODEL, smLUT.at(type),
                                plugin);
        if ((plugin == nullptr) || status) {
            PAL_ERR(LOG_TAG, "unable to get plugin for sound model type %s",
                    smLUT.at(type).c_str());
            return nullptr;
        }

        engineCreate = reinterpret_cast<EngineCreate>(plugin);
        engine = engineCreate(s, type, module_type, sm_cfg);
        if (!engine) {
            PAL_ERR(LOG_TAG, "Engine creation failed for sound model type %s",
                    smLUT.at(type).c_str());
            return nullptr;
        }
    }
    catch (const std::exception& e) {
        PAL_ERR(LOG_TAG, "Engine create failed for sound model type %s",
                smLUT.at(type).c_str());
        throw std::runtime_error(e.what());
    }

    std::shared_ptr<SoundTriggerEngine> st_engine(engine);


    PAL_VERBOSE(LOG_TAG, "Exit, engine %p", st_engine.get());

    return st_engine;
}

uint32_t SoundTriggerEngine::UsToBytes(uint64_t input_us) {
    uint32_t bytes = 0;

    bytes = sample_rate_ * bit_width_ * channels_ * input_us /
        (BITS_PER_BYTE * US_PER_SEC);

    return bytes;
}

uint32_t SoundTriggerEngine::FrameToBytes(uint32_t frames) {
    uint32_t total_bytes, bytes_per_frame = bit_width_ * channels_ / BITS_PER_BYTE;
    try {
        if ((bytes_per_frame) > (UINT32_MAX / frames))
            throw "multiplication overflow due to frames value";

        total_bytes = frames * bytes_per_frame;
    } catch (const char *e) {
        PAL_ERR(LOG_TAG, "FrametoBytes() failed with error %s . frames %u bit_width %u channels: %u",
                e, frames, bit_width_, channels_);
        return UINT32_MAX;
    }
    return total_bytes;
}

uint32_t SoundTriggerEngine::BytesToFrames(uint32_t bytes) {
    return (bytes * BITS_PER_BYTE) / (bit_width_ * channels_);
}

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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: Session"

#include "Session.h"
#include "ResourceManager.h"
#include "PalMappings.h"
#include "SessionAlsaPcm.h"
#include "SessionAlsaCompress.h"
#include "SessionAgm.h"
#include "SessionAlsaUtils.h"
#include "SessionAlsaVoice.h"

std::shared_ptr<PluginManager> Session::pm = nullptr;

Session::Session()
{
}

Session::~Session()
{
}

Session* Session::makeSession(const std::shared_ptr<ResourceManager>& rm, const struct pal_stream_attributes *sAttr)
{
    Session* s = (Session*) nullptr;
    int32_t status;
    void* plugin = nullptr;
    SessionCreate sessionCreate = NULL;

    if (!rm || !sAttr) {
        PAL_ERR(LOG_TAG, "Invalid parameters passed");
    } else {
        pm = PluginManager::getInstance();
        if (!pm) {
            PAL_ERR(LOG_TAG, "Unable to get plugin manager instance");
            return s;
        }

        try {
            status = pm->openPlugin(PAL_PLUGIN_MANAGER_SESSION,
                                    streamNameLUT.at(sAttr->type), plugin);
            if (plugin && !status) {
                sessionCreate = reinterpret_cast<SessionCreate>(plugin);
                s = sessionCreate(rm);
            }
            else {
                PAL_ERR(LOG_TAG, "unable to get session plugin for stream type %s",
                    streamNameLUT.at(sAttr->type).c_str());
            }
        }
        catch (const std::exception& e) {
            PAL_ERR(LOG_TAG, "Session create failed for stream type %s",
                streamNameLUT.at(sAttr->type).c_str());
        }
    }
    return s;
}

Session* Session::makeACDBSession(const std::shared_ptr<ResourceManager>& rm,
                                    const struct pal_stream_attributes *sAttr)
{
    if (!rm || !sAttr) {
        PAL_ERR(LOG_TAG,"Invalid parameters passed");
        return nullptr;
    }

    Session* s = (Session*) nullptr;
    s = new SessionAgm(rm);

    return s;
}

int Session::rwACDBParamTunnel(void *payload, pal_device_id_t palDeviceId,
                        pal_stream_type_t palStreamType, uint32_t sampleRate,
                        uint32_t instanceId, bool isParamWrite, Stream * s)
{
    int status = -EINVAL;
    struct pal_stream_attributes sAttr = {};

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    streamHandle = s;
    if (0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        goto exit;
    }

    PAL_INFO(LOG_TAG, "PAL device id=0x%x", palDeviceId);
    status = SessionAlsaUtils::rwACDBTunnel(s, rm, palDeviceId, payload, isParamWrite, instanceId);
    if (status) {
        PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
    }

exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}


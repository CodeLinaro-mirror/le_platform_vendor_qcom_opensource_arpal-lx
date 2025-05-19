/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 */

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022, 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PLUGIN_CONTROL_INTF_H_
#define _PLUGIN_CONTROL_INTF_H_

#include "Stream.h"
#include "PalDefs.h"
#include "PluginManagerIntf.h"

#define MAKE_PLUGIN_INTF_VERSION(maj, min) ((((maj) & 0xff) << 8) | ((min) & 0xff))
#define CONTROL_PLUGIN_GET_MAJOR_VERSION(ver) (((ver) & 0xff00) >> 8)
#define CONTROL_PLUGIN_GET_MINOR_VERSION(ver) ((ver) & 0x00ff)

#define CONTROL_PLUGIN_MAJOR_VER 1
#define CONTROL_PLUGIN_MINOR_VER 2
#define CONTROL_PLUGIN_VERSION MAKE_PLUGIN_INTF_VERSION(CONTROL_PLUGIN_MAJOR_VER,CONTROL_PLUGIN_MINOR_VER)

enum{
    USECASE_INVALID = -1,
    AUDIO_USECASE_PLAYBACK_DEEP_BUFFER = 0,
    AUDIO_USECASE_PLAYBACK_LOW_LATENCY,
    AUDIO_USECASE_PLAYBACK_VOIP,
    AUDIO_USECASE_PLAYBACK_OFFLOAD,
    AUDIO_USECASE_PLAYBACK_OFFLOAD2,
    AUDIO_USECASE_PLAYBACK_ULL,
    AUDIO_USECASE_PLAYBACK_MMAP,
    AUDIO_USECASE_MAX
};

typedef int (*plugin_set_control_fn_t) (Stream* s, plugin_control_name_t control,
                                void *payload, size_t payload_size);

typedef int (*plugin_get_control_fn_t) (Stream* s, plugin_control_name_t control,
                                void **payload, size_t *payload_size);


#endif /* _PLUGIN_CONTROL_INTF_H_ */

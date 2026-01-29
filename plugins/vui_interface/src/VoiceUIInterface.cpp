/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: VoiceUIInterface"

#include <dlfcn.h>
#include "VUIInterfaceProxy.h"
#include "SVAInterface.h"
#include "SVAExtension.h"

typedef struct vui_intf_plugin {
    void *handle;
    release_vui_intf_f release_intf;
    struct vui_intf_t *intf;
} vui_intf_plugin_t;

std::map<st_module_type_t, vui_intf_plugin_t *> vui_intf_plugin_map;


/* ============== Global Set and Get param APIs to enable SVA ======================== */
int32_t VUISetParameters(uint32_t param_id, void *param_payload, size_t payload_size) {

    int32_t status = 0;
    std::shared_ptr<SVAExtension> ext = SVAExtension::GetInstance();

    if (!param_payload || !ext) {
        return -EINVAL;
    }

    status = ext->SetParameters(param_id, param_payload, payload_size);

    return status;
}

int32_t VUIGetParameters(uint32_t param_id, void **param_payload, size_t *payload_size) {

    int32_t status = 0;
    std::shared_ptr<SVAExtension> ext = SVAExtension::GetInstance();

    if (!param_payload || !*param_payload || !payload_size || !ext) {
        return -EINVAL;
    }

    status = ext->GetParameters(param_id, param_payload, payload_size);

    return status;
}

/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SharedMemoryUtils.h"

#define LOG_TAG "PAL: SharedMemoryUtils"

int SharedMemoryUtils::shmem_buff_alloc(pal_shmem_info* payload)
{
    int status = 0;
    int devId;
    int size;
    struct mixer_ctl *ctl;
    std::ostringstream CntrlName;
    const char *control = "shmemAlloc";
    const char *stream = "PCM";
    struct mixer *mixer;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    struct agm_shmem_info buf_info;

    status = rm->getVirtualAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG, "mixer error");
        goto error;
    }

    /*
     * In normal case deviceId can be retrive using stream handle
     * but alloc is not coupled with stream and to pass mixer control
     * deviceId is required. This is used to just pass to tinymixer.
     */
    devId = 117;

    CntrlName << stream << devId << " " << control;
    ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
        status = -ENOENT;
        goto error;
    }

    size = sizeof(struct agm_shmem_info);
    buf_info.size = payload->size;
    buf_info.cache = payload->cache;

    status = mixer_ctl_set_array(ctl, &buf_info, size);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "set control failed status = %d", status);
        goto error;
    }

    status = mixer_ctl_get_array(ctl, &buf_info, size);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Get control failed status = %d", status);
        goto error;
    }

    payload->fd = buf_info.ion_fd;
    payload->spf_mem_addr_lsw = (uint32_t)buf_info.spf_addr;
    payload->spf_mem_addr_msw = (uint32_t)(buf_info.spf_addr >> 32);
    payload->spf_mem_handle = buf_info.spf_mem_handle;
    return 0;

error:
    payload->fd = 0;
    payload->spf_mem_addr_lsw = 0;
    payload->spf_mem_addr_msw = 0;
    payload->spf_mem_handle = 0;
    return status;
}

int SharedMemoryUtils::shmem_buff_free(pal_shmem_info* payload)
{
    int status = 0;
    int devId;
    int size;
    struct mixer_ctl *ctl;
    std::ostringstream CntrlName;
    const char *control = "shmemFree";
    const char *stream = "PCM";
    struct mixer *mixer;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    uint32_t spf_mem_handle;

    status = rm->getVirtualAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG, "mixer error");
        goto error;
    }

    /*
     * In normal case deviceId can be retrive using stream handle
     * but free is not coupled with stream and to pass mixer control
     * deviceId is required. This is used to just pass to tinymixer.
     */
    devId = 117;

    CntrlName << stream << devId << " " << control;
    ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
        status = -ENOENT;
        goto error;
    }

    size = sizeof(spf_mem_handle);
    spf_mem_handle = payload->spf_mem_handle;

    status = mixer_ctl_set_array(ctl, &spf_mem_handle, size);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "set control failed status = %d", status);
        goto error;
    }

    status = mixer_ctl_get_array(ctl, &spf_mem_handle, size);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Get control failed status = %d", status);
        goto error;
    }

    return 0;

error:
    return status;
}

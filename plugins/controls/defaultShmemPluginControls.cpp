/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: lib_default_set_param_plugin_controls"

#include <sys/mman.h>
#include <cutils/str_parms.h>
#include "defaultShmemPluginControls.h"

extern "C" {
struct pal_shmem_info g_buf_info;
uint32_t *g_vAddr = nullptr;
pal_stream_handle_t *g_pal_stream_handle = nullptr;
uint32_t shmem_size = 0;

int32_t shmem_buff_load_parameter(pal_stream_handle_t *stream_handle, pal_shmem_info buff_info)
{
    int32_t status = 0;
    uint8_t *payload = nullptr;
    pal_param_payload *pal_payload = nullptr;
    effect_pal_payload_t *effect_payload = nullptr;
    pal_effect_custom_payload_t *customPayload = nullptr;
    pal_load_persist_dummy_t *custom_payload = nullptr;
    uint32_t payload_size;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }

    payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_persist_dummy_t);
    payload = (uint8_t *) calloc (1, payload_size);
    if (!payload) {
        PAL_ERR(LOG_TAG,"%s calloc failed for size %d\n", __func__, payload_size);
        status = -ENOMEM;
        return status;
    }

    pal_payload = (pal_param_payload *) payload;
    pal_payload->payload_size = sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_persist_dummy_t);

    effect_payload = (effect_pal_payload_t *)(payload + sizeof(pal_param_payload));
    effect_payload->isTKV = PARAM_NONTKV;
    effect_payload->tag = TEST_SHARED_MEM_TAG;
    effect_payload->payloadSize = sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_persist_dummy_t);

    customPayload = (pal_effect_custom_payload_t *)(payload + sizeof(pal_param_payload) + sizeof(effect_pal_payload_t));
    customPayload->paramId = SPF_V2_PARAM_ID_LOAD_PERSIST_DUMMY;

    custom_payload = (pal_load_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
    custom_payload->size = buff_info.size;
    custom_payload->spf_mem_handle = buff_info.spf_mem_handle;
    custom_payload->spf_mem_addr_lsw = buff_info.spf_mem_addr_lsw;
    custom_payload->spf_mem_addr_msw = buff_info.spf_mem_addr_msw;

    status = pal_stream_set_param(stream_handle, PAL_PARAM_ID_UIEFFECT, pal_payload);
    if (status) {
        PAL_ERR(LOG_TAG,"pal_stream_set_param failed\n");
        free(payload);
        return status;
    }

    free(payload);
    return 0;
}

int32_t shmem_buff_set_parameter(pal_stream_handle_t *stream_handle, uint32_t param_id, uint8_t *payload, uint32_t size)
{
    int32_t status = 0;
    pal_param_payload *pal_payload = nullptr;
    effect_pal_payload_t *effect_payload = nullptr;
    pal_effect_custom_payload_t *customPayload = nullptr;

    if (!stream_handle || !payload) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }

    pal_payload = (pal_param_payload *) payload;
    pal_payload->payload_size = sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + size;

    effect_payload = (effect_pal_payload_t *)(payload + sizeof(pal_param_payload));
    effect_payload->isTKV = PARAM_NONTKV;
    effect_payload->tag = TEST_SHARED_MEM_TAG;
    effect_payload->payloadSize = sizeof(pal_effect_custom_payload_t) + size;

    customPayload = (pal_effect_custom_payload_t *)(payload + sizeof(pal_param_payload) + sizeof(effect_pal_payload_t));
    customPayload->paramId = param_id;

    status = pal_stream_set_param(stream_handle, PAL_PARAM_ID_UIEFFECT, pal_payload);
    if (status) {
        PAL_ERR(LOG_TAG,"pal_stream_set_param failed\n");
        return status;
    }

    return status;
}

int32_t shmem_buff_get_parameter(pal_stream_handle_t *stream_handle, uint8_t *payload, pal_load_get_persist_dummy_t *get_shmem, uint32_t size)
{
    int32_t status = 0;
    pal_param_payload *pal_payload = nullptr;
    effect_pal_payload_t *effect_payload = nullptr;
    pal_effect_custom_payload_t *customPayload = nullptr;

    if (!stream_handle || !payload) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }

    pal_payload = (pal_param_payload *) payload;
    pal_payload->payload_size = sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t);

    effect_payload = (effect_pal_payload_t *)(payload + sizeof(pal_param_payload));
    effect_payload->isTKV = PARAM_NONTKV;
    effect_payload->tag = TEST_SHARED_MEM_TAG;
    effect_payload->payloadSize = sizeof(pal_effect_custom_payload_t) + size;

    customPayload = (pal_effect_custom_payload_t *)(payload + sizeof(pal_param_payload) + sizeof(effect_pal_payload_t));
    customPayload->paramId = SPF_V2_PARAM_ID_LOAD_GET_PERSIST_DUMMY;

    status = pal_stream_get_param(stream_handle, PAL_PARAM_ID_PLUGIN_PARAM, &pal_payload);
    if (status) {
        PAL_ERR(LOG_TAG,"pal_stream_get_param failed\n");
        return status;
    } else {
        if(pal_payload) {
            memcpy(get_shmem, pal_payload, size);
            free(pal_payload);
        }
    }

    return status;
}

static int32_t stop_audio_session(pal_stream_handle_t * &stream_handle, pal_shmem_info buf_info, uint32_t * &p)
{
    int32_t status;

    if (stream_handle) {
        status = pal_stream_stop(stream_handle);
        if (status) {
            PAL_ERR(LOG_TAG,"pal_stream_stop failed\n");
        }

        status = pal_stream_close(stream_handle);
        if (status) {
            PAL_ERR(LOG_TAG,"pal_stream_close failed\n");
        }
        stream_handle = nullptr;
    }

    if (p) {
        munmap(p, buf_info.size);
        p = nullptr;
    }
    return status;
}

static int32_t alloc_shmem(pal_shmem_info *buf_info, uint32_t shmem_size, pal_shmem_cache_type_t cache_type)
{
    int32_t status;

    memset(buf_info, 0, sizeof(pal_shmem_info));
    buf_info->size = shmem_size;
    buf_info->cache = cache_type;

    status = SharedMemoryUtils::shmem_buff_alloc(buf_info);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"Shared memory buffer allocation failed\n");
        status = -EINVAL;
        return status;
    }
    PAL_INFO(LOG_TAG,"Received values: fd %d, value of spf handle %x, spf_mem_addr [%x %x]\n", buf_info->fd, buf_info->spf_mem_handle, buf_info->spf_mem_addr_lsw, buf_info->spf_mem_addr_msw);
    PAL_DBG(LOG_TAG,"Exit with status: %d\n", status);
    return status;
}

static int32_t free_shmem(pal_shmem_info *buf_info)
{
    int32_t status = -EINVAL;

    if (buf_info->spf_mem_handle && buf_info->fd) {
        status = SharedMemoryUtils::shmem_buff_free(buf_info);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"Shared memory buffer Free failed\n");
            status = -EINVAL;
        }
    }

    memset(buf_info, 0, sizeof(pal_shmem_info));

    PAL_DBG(LOG_TAG,"Exit with status: %d\n", status);
    return status;
}

static int32_t start_audio_session(pal_stream_handle_t * &stream_handle, pal_shmem_info buf_info, uint32_t * &p)
{
    struct pal_stream_attributes stream_attr;
    int32_t status;
    int32_t channels = NUMBER_OF_CHANNEL;
    struct pal_channel_info ch_info;
    struct pal_device devices[1] = {};
    ch_info.channels = channels;
    ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
    ch_info.ch_map[1] = PAL_CHMAP_CHANNEL_FR;

    stream_attr.type = PAL_STREAM_DEEP_BUFFER;
    stream_attr.out_media_config.sample_rate = SAMPLE_RATE;
    stream_attr.out_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    stream_attr.out_media_config.bit_width = BIT_WIDTH;
    stream_attr.out_media_config.ch_info = ch_info;
    stream_attr.direction = PAL_AUDIO_OUTPUT;

    devices[0].id = PAL_DEVICE_OUT_SPEAKER;
    devices[0].config.sample_rate = SAMPLE_RATE;
    devices[0].config.bit_width = BIT_WIDTH;
    devices[0].config.ch_info = ch_info;
    devices[0].config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    memset(devices[0].custom_config.custom_key, 0, sizeof(devices[0].custom_config.custom_key));
    strlcpy(devices[0].custom_config.custom_key,
            "capi_load",
            sizeof(devices[0].custom_config.custom_key));

    status = pal_stream_open(&stream_attr,
            1,
            devices,
            0,
            nullptr,
            nullptr,
            0,
            &stream_handle);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"failed to open pal stream status=%d\n", status);
        status = -EINVAL;
        return status;
    }

    status = pal_stream_start(stream_handle);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"failed to start pal stream \n");
        status = -EINVAL;
        goto close_pal;
    }

    p = (uint32_t *)mmap(nullptr, buf_info.size, PROT_READ | PROT_WRITE, MAP_SHARED, buf_info.fd, 0);
    if (p == nullptr) {
        PAL_ERR(LOG_TAG,"Error: mmap return nullptr\n");
        status = -ENOMEM;
        goto stop_pal;
    }

    status = shmem_buff_load_parameter(stream_handle, buf_info);
    if (status) {
        PAL_ERR(LOG_TAG,"shmem_buff_load_parameter failed\n");
        goto exit;
    }
    else
        PAL_DBG(LOG_TAG,"shmem_buff_load_parameter is successful\n");

    PAL_DBG(LOG_TAG,"Exit with status: %d\n", status);
    return status;

exit:
    if (p) {
        munmap(p, buf_info.size);
        p = nullptr;
    }

stop_pal:
    if (stream_handle) {
        pal_stream_stop(stream_handle);
    }

close_pal:
    if (stream_handle) {
        pal_stream_close(stream_handle);
        stream_handle = nullptr;
    }

    PAL_DBG(LOG_TAG,"Exit with status: %d\n", status);
    return status;
}

int32_t SPF_set_shmem(pal_stream_handle_t *stream_handle, uint32_t index, uint32_t value)
{
    int32_t status;
    uint8_t *payload = nullptr;
    uint32_t payload_size;
    pal_load_set_persist_dummy_t *set_shmem = nullptr;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }

    payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_set_persist_dummy_t);
    payload = (uint8_t *) calloc (1, payload_size);
    if (!payload) {
        PAL_ERR(LOG_TAG,"calloc failed for size %d\n", payload_size);
        status = -ENOMEM;
        return status;
    }
    set_shmem = (pal_load_set_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
    set_shmem->effective = 1;
    set_shmem->index = index;
    set_shmem->value = value;
    status = shmem_buff_set_parameter(stream_handle, SPF_V2_PARAM_ID_LOAD_SET_PERSIST_DUMMY, payload, sizeof(pal_load_set_persist_dummy_t));
    if (status) {
        PAL_ERR(LOG_TAG,"shmem_buff_set_parameter failed\n");
        free(payload);
        status = -EINVAL;
        return status;
    }
    else
        PAL_DBG(LOG_TAG,"shmem_buff_set_parameter is successful\n");

    free(payload);
    return status;
}

int32_t SPF_get_shmem(pal_stream_handle_t *stream_handle, uint32_t index, uint32_t number, pal_load_get_persist_dummy_t *get_shmem, uint32_t gpd_size)
{
    int32_t status;
    {
        uint8_t *payload = nullptr;
        uint32_t payload_size;
        pal_load_get_persist_dummy_t *set_shmem = nullptr;

        if (!stream_handle || !get_shmem) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
            return status;
        }

        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_get_persist_dummy_t);
        payload = (uint8_t *) calloc (1, payload_size);
        if (!payload) {
            PAL_ERR(LOG_TAG,"calloc failed for size %d\n", payload_size);
            status = -ENOMEM;
            return status;
        }
        set_shmem = (pal_load_get_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
        set_shmem->index = index;
        set_shmem->num = number;
        status = shmem_buff_set_parameter(stream_handle, SPF_V2_PARAM_ID_LOAD_GET_PERSIST_DUMMY, payload, sizeof(pal_load_get_persist_dummy_t));
        if (status) {
            PAL_ERR(LOG_TAG,"shmem_buff_set_parameter failed\n");
            free(payload);
            status = -EINVAL;
            return status;
        }
        else
            PAL_DBG(LOG_TAG,"shmem_buff_set_parameter is successful\n");

        free(payload);
    }

    {
        uint8_t *payload = nullptr;
        uint32_t payload_size;

        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t);
        payload = (uint8_t *) calloc (1, payload_size);
        if (!payload) {
            PAL_ERR(LOG_TAG,"calloc failed for size %d\n", payload_size);
            status = -ENOMEM;
            return status;
        }
        status = shmem_buff_get_parameter(stream_handle, payload, get_shmem, gpd_size);
        if (status) {
            PAL_ERR(LOG_TAG,"shmem_buff_get_parameter failed\n");
            free(payload);
            status = -EINVAL;
            return status;
        }
        else
            PAL_DBG(LOG_TAG,"shmem_buff_get_parameter is successful\n");

        free(payload);
    }
    return status;
}

int32_t SPF_flush_shmem(pal_stream_handle_t *stream_handle, uint32_t index, uint32_t number, uint32_t cache_conf)
{
    int32_t status;
    uint8_t *payload = nullptr;
    uint32_t payload_size;
    pal_load_cache_op_t *set_cache = nullptr;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }

    payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_cache_op_t);
    payload = (uint8_t *) calloc (1, payload_size);
    if (!payload) {
        PAL_ERR(LOG_TAG,"calloc failed for size %d\n", payload_size);
        status = -ENOMEM;
        return status;
    }
    set_cache = (pal_load_cache_op_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
    set_cache->effective = 1;
    set_cache->invalidate = (cache_conf >> 1) & 0x1;
    set_cache->flush = cache_conf & 0x1;
    set_cache->offset = index * sizeof(uint32_t);
    set_cache->size = 4 * number;
    status = shmem_buff_set_parameter(stream_handle, SPF_V2_PARAM_ID_LOAD_CACHE_OP, payload, sizeof(pal_load_cache_op_t));
    if (status) {
        PAL_ERR(LOG_TAG,"shmem_buff_set_parameter failed\n");
        free(payload);
        status = -EINVAL;
        return status;
    }
    else
        PAL_DBG(LOG_TAG,"shmem_buff_set_parameter is successful\n");

    free(payload);
    return status;
}

int32_t HLOS_set_shmem(uint32_t *p, uint32_t index, uint32_t number, uint32_t value)
{
    uint32_t j;
    if (p) {
        for(j = 0; j < number; j++){
            p[index + j] = value;
        }
    }
    else
        return -EINVAL;
    return 0;
}

int32_t HLOS_get_shmem(uint32_t *p, uint32_t index)
{
    if (p) {
        return p[index];
    }
    return -EINVAL;
}

__attribute__ ((visibility ("default")))
int32_t plugin_get(Stream* s, plugin_control_name_t control, void **payload,
        size_t *size)
{
    int32_t status = 0;
    return status;
}

/*interface function definition*/
__attribute__ ((visibility ("default")))
int32_t plugin_set(Stream* s, plugin_control_name_t control, void *payload,
        size_t size)
{
    int32_t ret = 0;
    int32_t status = -EINVAL;
    char value[256];
    pal_shmem_cache_type_t cache = PAL_SHMEM_MAP_UNCACHED;
    uint32_t index = 0;
    uint32_t count = 0;
    uint32_t numOf_element = 0;
    uint32_t index_start = 0;
    uint32_t index_end = 0;
    uint32_t number = 0;
    uint32_t i, j;
    struct str_parms *parms = nullptr;
    struct str_parms *parms_t = nullptr;

    PAL_INFO(LOG_TAG,"Enter with control %d size=%d", control, size);

    if (!payload) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }

    parms = (str_parms *) payload;

    switch (control) {
    case PLUGIN_CONTROL_KV_PARAM:
    {
        ret = str_parms_get_str(parms, "pal_plugin_param", value, sizeof(value));
        if (ret >= 0) {
            parms_t = str_parms_create_str(value);
            if (!parms_t) {
                PAL_ERR(LOG_TAG,"Error in str_parms_create_str");
                ret = 0;
                break;
            }

            ret = str_parms_get_str(parms_t, "alloc", value, sizeof(value));
            if (ret >= 0) {
                shmem_size = atoi(value);
                str_parms_del(parms_t, "alloc");
            }

            ret = str_parms_get_str(parms_t, "set_index", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms_t, "set_index");
                sscanf(value, "<%d>", &index_start);
                numOf_element = 1;
                if (((index_start + 1)*numOf_element) > (shmem_size/sizeof(uint32_t)))
                {
                    PAL_ERR(LOG_TAG,"Error : Buffer Overflow Allocated=%d, Required=%d\n", shmem_size, (index_start + 1)*numOf_element*sizeof(uint32_t));
                    status = -EINVAL;
                    goto cleanup;
                }
                index_end = index_start;
            }

            ret = str_parms_get_str(parms_t, "set_index_range", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms_t, "set_index_range");
                sscanf(value, "<%d,%d>", &index_start, &index_end);
                numOf_element = 1024;
                if (index_start > index_end) {
                    PAL_ERR(LOG_TAG,"Error : index_start=%d is greater than index_end=%d\n", index_start, index_end);
                    status = -EINVAL;
                    goto cleanup;
                }
                if (((index_end + 1)*numOf_element) > (shmem_size/sizeof(uint32_t))) {
                    PAL_ERR(LOG_TAG,"Error : Buffer Overflow Allocated=%d, Required=%d\n", shmem_size, (index_end + 1)*numOf_element*sizeof(uint32_t));
                    status = -EINVAL;
                    goto cleanup;
                }
            }

            ret = str_parms_get_str(parms_t, "free", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms_t, "free");
                if (!strncmp(value, "true", size)){
                    if (g_pal_stream_handle) {
                        status = stop_audio_session(g_pal_stream_handle, g_buf_info, g_vAddr);
                        if (status) {
                            PAL_ERR(LOG_TAG,"%s:%d stop_audio_session failed failed\n", __func__, __LINE__);
                            goto shmem_cleanup;
                        }
                    }
                    status = free_shmem(&g_buf_info);
                    if (status) {
                        PAL_ERR(LOG_TAG,"%s:%d shared memory free failed failed\n", __func__, __LINE__);
                        goto shmem_cleanup;
                    }
                } else if (!strncmp(value, "false", size)){
                }
            }
            str_parms_destroy(parms_t);

            ret = str_parms_get_str(parms, "type", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms, "type");
                if (!strncmp(value, "uncached", size)){
                    cache = PAL_SHMEM_MAP_UNCACHED;
                } else if (!strncmp(value, "cached", size)){
                    cache = PAL_SHMEM_MAP_CACHED;
                }
                status = alloc_shmem(&g_buf_info, shmem_size, cache);
                if (status) {
                    PAL_ERR(LOG_TAG,"shared memory allocation failed\n");
                    return -EINVAL;
                }
            }

            ret = str_parms_get_str(parms, "set_index", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms, "set_index");
                sscanf(value, "<%d>", &index_start);
                numOf_element = 1;
                if (((index_start + 1)*numOf_element) > (shmem_size/sizeof(uint32_t))) {
                    PAL_ERR(LOG_TAG,"Error : Buffer Overflow Allocated=%d, Required=%d\n", shmem_size, (index_start + 1)*numOf_element*sizeof(uint32_t));
                    status = -EINVAL;
                    goto shmem_cleanup;
                }
                index_end = index_start;
            }

            ret = str_parms_get_str(parms, "set_index_range", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms, "set_index_range");
                sscanf(value, "<%d,%d>", &index_start, &index_end);
                numOf_element = 1024;
                if (index_start > index_end) {
                    PAL_ERR(LOG_TAG,"Error : index_start=%d is greater than index_end=%d\n", index_start, index_end);
                    status = -EINVAL;
                    goto shmem_cleanup;
                }
                if (((index_end + 1)*numOf_element) > (shmem_size/sizeof(uint32_t))) {
                    PAL_ERR(LOG_TAG,"Error : Buffer Overflow Allocated=%d, Required=%d\n", shmem_size, (index_end + 1)*numOf_element*sizeof(uint32_t));
                    status = -EINVAL;
                    goto shmem_cleanup;
                }
            }

            ret = str_parms_get_str(parms, "value", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms, "value");
                number = atoi(value);
                if (g_pal_stream_handle == nullptr) {
                    status = start_audio_session(g_pal_stream_handle, g_buf_info, g_vAddr);
                    if (status) {
                        PAL_ERR(LOG_TAG,"stream start failed\n");
                        goto shmem_cleanup;
                    }
                }
            }

            ret = str_parms_get_str(parms, "write_validate", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms, "write_validate");
                if (!strncmp(value, "HLOS", size)) {
                    count = 0;
                    for (i = index_start; i<= index_end; i++) {
                        index = i*numOf_element;
                        status = HLOS_set_shmem(g_vAddr, index, numOf_element, number);
                        if (status) {
                            PAL_ERR(LOG_TAG,"HLOS_set_shmem failed\n");
                            goto cleanup;
                        }

                        if (cache == PAL_SHMEM_MAP_CACHED) {
                            status = SPF_flush_shmem(g_pal_stream_handle, index, numOf_element, INVALIDATE_MODE);
                            if (status) {
                                PAL_ERR(LOG_TAG,"SPF_flush_shmem failed\n");
                                goto cleanup;
                            }
                        }

                        {
                            pal_load_get_persist_dummy_t *get_shmem = nullptr;

                            uint32_t gpd_size = sizeof(pal_load_get_persist_dummy_t)+(numOf_element*sizeof(uint32_t));
                            get_shmem = (pal_load_get_persist_dummy_t *) calloc (1, gpd_size);
                                if (!get_shmem) {
                                    PAL_ERR(LOG_TAG,"calloc failed for size %d\n", gpd_size);
                                    goto cleanup;
                                }
                            status = SPF_get_shmem(g_pal_stream_handle, index, numOf_element, get_shmem, gpd_size);
                            if (status) {
                                PAL_ERR(LOG_TAG,"SPF_get_shmem failed\n");
                                if (get_shmem)
                                    free(get_shmem);
                                goto cleanup;
                            }
                            for (j = 0; j < numOf_element; j++) {
                                if (get_shmem->array[j] != number) {
                                    PAL_ERR(LOG_TAG,"HLOS write value %d is not matching with SPF read value[%d] %d\n", number, j, get_shmem->array[j]);
                                    count++;
                                }
                            }
                            if (get_shmem)
                                free(get_shmem);
                        }
                    }
                    if (count == 0) {
                        PAL_INFO(LOG_TAG,"HLOS test passed\n");
                    } else {
                        PAL_ERR(LOG_TAG,"HLOS test failed\n");
                    }
                } else if (!strncmp(value, "SPF", size)) {
                    count = 0;
                    for (i = index_start; i<= index_end; i++) {
                        index = i*numOf_element;
                        for (j = 0; j< numOf_element; j++) {
                            status = SPF_set_shmem(g_pal_stream_handle, index + j, number);
                            if (status) {
                                PAL_ERR(LOG_TAG,"SPF_set_shmem failed\n");
                                goto cleanup;
                            }
                        }
                        if (cache == PAL_SHMEM_MAP_CACHED) {
                            status = SPF_flush_shmem(g_pal_stream_handle, index, numOf_element, FLUSH_MODE);
                            if (status) {
                                PAL_ERR(LOG_TAG,"SPF_flush_shmem failed\n");
                                goto cleanup;
                            }
                        }
                        for (j = 0; j< numOf_element; j++) {
                            if(HLOS_get_shmem(g_vAddr, index + j) != number) {
                                PAL_ERR(LOG_TAG,"SPF set value %d is not matching with HLOS read value p[%d]=%d\n", number, index + j, g_vAddr[index + j]);
                                count++;
                            }
                        }
                    }
                    if (count == 0) {
                        PAL_INFO(LOG_TAG,"SPF test passed\n");
                    } else {
                        PAL_ERR(LOG_TAG,"SPF test failed\n");
                    }
                }
            }

            ret = str_parms_get_str(parms, "free", value, sizeof(value));
            if (ret >= 0) {
                str_parms_del(parms, "free");
                if (!strncmp(value, "true", size)){
                    if (g_pal_stream_handle) {
                        status = stop_audio_session(g_pal_stream_handle, g_buf_info, g_vAddr);
                        if (status) {
                            PAL_ERR(LOG_TAG,"%s:%d stop_audio_session failed failed\n", __func__, __LINE__);
                            goto shmem_cleanup;
                        }
                    }
                    status = free_shmem(&g_buf_info);
                    if (status) {
                        PAL_ERR(LOG_TAG,"%s:%d shared memory free failed failed\n", __func__, __LINE__);
                        break;
                    }
                } else if (!strncmp(value, "false", size)){
                }
            }
        }

        PAL_DBG(LOG_TAG,"Exit with status: %d", status);
        break;

        if (!g_pal_stream_handle) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
            break;
        }

cleanup:
        stop_audio_session(g_pal_stream_handle, g_buf_info, g_vAddr);
shmem_cleanup:
        free_shmem(&g_buf_info);
        PAL_DBG(LOG_TAG,"Exit with status: %d", status);
        break;
    }
    default:
        PAL_ERR(LOG_TAG,"control not supported in this plugin");
        break;
    }

    return status;
}
}

/*
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SHARED_MEMORY_UTILS_H
#define SHARED_MEMORY_UTILS_H

#include <errno.h>
#include <sstream>
#include "ResourceManager.h"
#include <agm/agm_api.h>
#include <tinyalsa/asoundlib.h>
#include <sound/asound.h>
#include "PalDefs.h"

/** enum for shmem cache type*/
typedef enum pal_shmem_cache_type {
    /** 0  cached.*/
    PAL_SHMEM_MAP_CACHED,
    /** 1  uncached.*/
    PAL_SHMEM_MAP_UNCACHED
} pal_shmem_cache_type_t;

struct pal_shmem_info
{
    /** size of shared memory to be allocated */
    uint32_t size;
    /** Indicates memory is cached or uncached */
    pal_shmem_cache_type_t cache;
    /** fd to the virtual address of the shared memory */
    uint32_t fd;
    /** Memory handle to the shared memory */
    uint32_t spf_mem_handle;
    /** Lower 32 bits of the spf_addr returned from allocation */
    uint32_t spf_mem_addr_lsw;
    /** Upper 32 bits of the spf_addr returned from allocation */
    uint32_t spf_mem_addr_msw;
};

class SharedMemoryUtils
{
public:
    SharedMemoryUtils() {};
    ~SharedMemoryUtils();
    static int shmem_buff_alloc(pal_shmem_info* payload);
    static int shmem_buff_free(pal_shmem_info* payload);

};
#endif //SHARED_MEMORY_UTILS_H

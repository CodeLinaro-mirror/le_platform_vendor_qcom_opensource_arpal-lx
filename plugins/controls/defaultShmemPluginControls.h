/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** \file defaultShmemPluginControls.h
 *  \brief struture, enum constant defintions of the
 *         PAL(Platform Audio Layer).
 *
 *  This file contains macros, constants, or global variables
 *  exposed to the client of PAL(Platform Audio Layer).
 */

#ifndef DEFAULT_SHMEM_PLUGIN_CONTROLS_H
#define DEFAULT_SHMEM_PLUGIN_CONTROLS_H

#include "SharedMemoryUtils.h"

#ifdef __cplusplus

extern "C" {
#endif

#define SPF_V2_PARAM_ID_LOAD_PERSIST_DUMMY     0x080012AA
#define SPF_V2_PARAM_ID_LOAD_GET_PERSIST_DUMMY 0x080012AB
#define SPF_V2_PARAM_ID_LOAD_SET_PERSIST_DUMMY 0x080012AC
#define SPF_V2_PARAM_ID_LOAD_CACHE_OP          0x080012AD
#define TEST_SHARED_MEM_TAG                    0xC00FF019
#define SAMPLE_RATE                            48000
#define NUMBER_OF_CHANNEL                      2
#define BIT_WIDTH                              16
#define FLUSH_MODE                             1
#define INVALIDATE_MODE                        2

/** @h2xmlp_parameter   {"AUDIO_LOAD_PERSIST_DUMMY", SPF_V2_PARAM_ID_LOAD_PERSIST_DUMMY}
    @h2xmlp_description {set shared memory info} */
typedef struct {
    /** Memory handle to the shared memory */
    uint32_t spf_mem_handle;
    /** Lower 32 bits of the address of the buffer containing the data */
    uint32_t spf_mem_addr_lsw;
    /** Upper 32 bits of the address of the buffer containing the data */
    uint32_t spf_mem_addr_msw;
    /** Contains the size of allocated shared memory */
    uint32_t size;
    uint32_t is_ref_counted;
} pal_load_persist_dummy_t;

/** @h2xmlp_parameter   {"AUDIO_LOAD_GET_PERSIST_DUMMY", SPF_V2_PARAM_ID_LOAD_GET_PERSIST_DUMMY}
    @h2xmlp_description {read partial payload of persist dummy parameter. Set index and num than read back, array should contain the result.} */
typedef struct {
    uint32_t index;
    /**< @h2xmle_description  {persist dummy parameter is interpreted as uint32_t[], start reading uint32_t values at index of the persist dummy array} */
    uint32_t num;
    /**< @h2xmle_description  {number of elements in array} */
    uint32_t array[0];
     /**< @h2xmle_variableArraySize {num}
          @h2xmle_description  {variable array with num elements} */
} pal_load_get_persist_dummy_t;

/** @h2xmlp_parameter   {"AUDIO_LOAD_SET_PERSIST_DUMMY", SPF_V2_PARAM_ID_LOAD_SET_PERSIST_DUMMY}
    @h2xmlp_description {write to persist dummy parameter} */
typedef struct {
    uint32_t effective;
    /**< @h2xmle_rangeList{"No"=0; "Yes"=1} */
    uint32_t index;
    /**< @h2xmle_description  {persist dummy parameter is interpreted as uint32_t[]} */
    uint32_t value;
} pal_load_set_persist_dummy_t;

typedef struct {
    uint32_t effective;
    uint16_t invalidate;
    uint16_t flush;
    uint32_t offset;
    uint32_t size;
} pal_load_cache_op_t;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /*DEFAULT_SHMEM_PLUGIN_CONTROLS_H*/

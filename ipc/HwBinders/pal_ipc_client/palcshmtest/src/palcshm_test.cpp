/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <log/log.h>
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <vector>
#include <tuple>
#include <fstream>
#include <thread>
#include <future>
#include <string>
#include <random>
#include <json/json.h>
#include <vendor/qti/hardware/pal/1.0/IPAL.h>
#include <vendor/qti/hardware/paleventnotifier/1.0/IPALEventNotifier.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <hidlmemory/mapping.h>
#include "palcshm_test.h"

#define LOG_NDEBUG 0
#define LOG_TAG "cshm_test_client"
#define MAX_SET_CFG_SIZE 64
#define DEFAULT_FRAME_NUM 50
#define DEFAULT_FRAME_SIZE 1024
#define DEFAULT_DL_DURATION 10
#define CLIENT_COOKIE 0x6373686d
#define SHMEM_MODULE_TAG 0xc000005b
#define DEFAULT_TEST_CONF "/data/cshm_test.json"
#define ALIGN_8(x) (((x) + 7) & (~7))

using android::sp;
using android::Thread;
using android::status_t;
using android::hardware::Return;
using android::hardware::hidl_vec;
using android::hardware::EventFlag;
using android::hardware::hidl_memory;
using android::hardware::MessageQueue;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hidl::allocator::V1_0::IAllocator;
using vendor::qti::hardware::paleventnotifier::V1_0::IPALEventNotifier;
using vendor::qti::hardware::paleventnotifier::V1_0::IPALEventNotifierCallback;
using vendor::qti::hardware::paleventnotifier::V1_0::implementation::PALEventNotifierCallback;
using vendor::qti::hardware::pal::V1_0::IPAL;
using vendor::qti::hardware::pal::V1_0::implementation::PalCallback;

uint32_t cshm_tc_log_lvl = (CSHM_TC_LOG_ERR|CSHM_TC_LOG_INFO);
const uint32_t TAG_MODULE_LIST_SIZE = 1024;
bool is_pal_alive = true;
std::atomic<bool> is_adsp_down(false);

static std::mutex pal_notifier_mutex;
static std::mutex pal_server_lock;

android::sp<IPAL> pal_server = NULL;
sp<server_death_notifier> Server_death_notifier = NULL;
sp<IPALEventNotifier> PALEventNotifier_client = nullptr;
sp<IPALEventNotifierCallback> ClbkBinder = nullptr;
sp<IPALCallback> PalCallbackBinder = nullptr;
sp<IAllocator> ashmemAllocator = NULL;

static std::vector<Json::Value> alloced_mem_info;
static std::vector<std::future<int>> data_logging_futures;
std::map<std::string, pal_stream_handle_t *> tc_notifier_info;

enum {
    CMD_ALLOC = 1,
    CMD_DEALLOC,
    CMD_SET_CFG,
    CMD_GET_CFG,
    CMD_DATA_LOGGING,
    CMD_WAIT,
};

std::map<std::string, int> cmd_look_up = {
                                {"alloc",CMD_ALLOC },
                                {"dealloc", CMD_DEALLOC},
                                {"set_cfg", CMD_SET_CFG},
                                {"get_cfg", CMD_GET_CFG},
                                {"data_logging", CMD_DATA_LOGGING},
                                {"wait", CMD_WAIT}};


std::map<std::string, PalStreamType> string_to_enum_stream = {
    {"PAL_STREAM_COMPRESSED", PalStreamType::PAL_STREAM_COMPRESSED},
    {"PAL_STREAM_LOW_LATENCY", PalStreamType::PAL_STREAM_LOW_LATENCY},
    {"PAL_STREAM_DEEP_BUFFER", PalStreamType:: PAL_STREAM_DEEP_BUFFER},
    {"PAL_STREAM_PCM_OFFLOAD", PalStreamType::PAL_STREAM_PCM_OFFLOAD}
};

std::map<std::string, PalDeviceId> string_to_enum_device = {
    {"PAL_DEVICE_OUT_SPEAKER", PalDeviceId::PAL_DEVICE_OUT_SPEAKER},
    {"PAL_DEVICE_OUT_BLUETOOTH_A2DP", PalDeviceId::PAL_DEVICE_OUT_BLUETOOTH_A2DP},
    {"PAL_DEVICE_IN_HANDSET_MIC", PalDeviceId::PAL_DEVICE_IN_HANDSET_MIC},
    {"PAL_DEVICE_IN_HANDSET_VA_MIC", PalDeviceId::PAL_DEVICE_IN_HANDSET_VA_MIC},
    {"PAL_DEVICE_OUT_WIRED_HEADSET", PalDeviceId::PAL_DEVICE_OUT_WIRED_HEADSET},
    {"PAL_DEVICE_OUT_RECORD_PROXY", PalDeviceId::PAL_DEVICE_OUT_RECORD_PROXY}
};


void wait_for_duration(int duration, std::atomic<bool>& keep_reading) {
    CSHM_TC_DBG(LOG_TAG, "Notification thread dispatched with sleep duration: %d", duration);
    std::this_thread::sleep_for(std::chrono::seconds(duration));
    keep_reading.store(false);
    return;
}

static uint64_t get_timeofday()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0 ) {
        return ts.tv_sec *1000 + ts.tv_nsec/1000000;
    }
    ALOGE("%s: Error in getting time", LOG_TAG);//TODO: Check if abort
    return 0;

}


android::sp<IPAL> get_pal_server() {
    std::lock_guard<std::mutex> guard(pal_server_lock);
    if (pal_server == NULL) {
        pal_server = IPAL::getService();
        if (pal_server == nullptr) {
            ALOGE("PAL service not initialized\n");
            goto exit;
        }
        if(Server_death_notifier == NULL) {
            Server_death_notifier = new server_death_notifier();
            pal_server->linkToDeath(Server_death_notifier, 0);
            CSHM_TC_DBG(LOG_TAG, "palclient linked to death server death \n");
        }
        ashmemAllocator = IAllocator::getService("ashmem");
    }
exit:
    return pal_server ;
}

void server_death_notifier::serviceDied(uint64_t cookie,
                   const android::wp<::android::hidl::base::V1_0::IBase>& who)
{
    CSHM_TC_INFO(LOG_TAG, "PAL Service died ,cookie : %lu", cookie);
    is_pal_alive = false;
    _exit(1);
}

PalCallback::~PalCallback() {
    CSHM_TC_VERBOSE(LOG_TAG, "Destructor for PalCallback");
}

/* Implementing these to keep compiler happy */
Return<int32_t> PalCallback::event_callback(uint64_t strm_handle,
                                 uint32_t event_id,
                                 uint32_t event_data_size,
                                 const hidl_vec<uint8_t>& event_data,
                                 uint64_t cookie) {
    return int32_t {};
}

Return<void> PalCallback::event_callback_rw_done(uint64_t strm_handle,
                                 uint32_t event_id,
                                 uint32_t event_data_size,
                                 const hidl_vec<PalCallbackBuffer>& event_data,
                                 uint64_t cookie) {
    return Void();
}

Return<void> PalCallback::prepare_mq_for_transfer(PalStreamHandle streamHandle,
                            uint64_t cookie, prepare_mq_for_transfer_cb _hidl_cb) {
    status_t status;

    return Void();
}

Return<void> PalCallback::ssr_event(uint8_t state, uint32_t subsystem, uint64_t cookie) {

    CSHM_TC_DBG(LOG_TAG, "State: %d", state);
    return Void();
}

Return<void> PALEventNotifierCallback::onStart(const PalCallbackConfig_hidl& config) {
    const PalStreamType &stream_type = config.stream_attributes.type;
    const hidl_vec<PalDeviceId> &current_devices = config.currentdevices;

    std::lock_guard<std::mutex> lock(pal_notifier_mutex);

    CSHM_TC_DBG(LOG_TAG, "Recieved onStart");

    for (auto itr = tc_notifier_info.begin(); itr != tc_notifier_info.end(); itr++) {
        if(string_to_enum_stream[itr->first] == config.stream_attributes.type) {
            tc_notifier_info[itr->first] = (pal_stream_handle_t *)(config.stream_handle);
            CSHM_TC_INFO(LOG_TAG, "Stored stream handle %pK for stream type: %d", tc_notifier_info[itr->first],
                                                            config.stream_attributes.type);
        }
        else {
            for (int i = 0; i < config.currentdevices.size(); i++) {
                if (string_to_enum_device[itr->first] == config.currentdevices[i]) {
                    tc_notifier_info[itr->first] = (pal_stream_handle_t *)(config.stream_handle);
                    CSHM_TC_INFO(LOG_TAG, "Stored stream handle %pK for device type: %d", tc_notifier_info[itr->first], config.currentdevices[i]);
                }
            }
        }
    }

  return Void();
}

Return<void> PALEventNotifierCallback::onStop(const PalCallbackConfig_hidl& config) {
    const PalStreamType &stream_type = config.stream_attributes.type;
    const hidl_vec<PalDeviceId> &current_devices = config.currentdevices;
    std::lock_guard<std::mutex> lock(pal_notifier_mutex);

    CSHM_TC_DBG(LOG_TAG, "Recieved onStop");
    for (auto itr = tc_notifier_info.begin(); itr != tc_notifier_info.end(); itr++) {
        if(string_to_enum_stream[itr->first] == config.stream_attributes.type) {
            tc_notifier_info[itr->first] = nullptr;
            CSHM_TC_INFO(LOG_TAG, "Erased stream handle for stream type: %d",
                                                            config.stream_attributes.type);
        }
        else {
            for (int i = 0; i < config.currentdevices.size(); i++) {
                if (string_to_enum_device[itr->first] == config.currentdevices[i]) {
                    tc_notifier_info[itr->first] = nullptr;
                    CSHM_TC_INFO(LOG_TAG, "Erased stream handle for device type: %d", config.currentdevices[i]);
                }
            }
        }
    }
    return Void();
}

Return<void> PALEventNotifierCallback::onDeviceSwitch(const PalCallbackConfig_hidl& config) {

    CSHM_TC_DBG(LOG_TAG, "Recieved onDeviceSwitch");
    const hidl_vec<PalDeviceId> &current_devices = config.currentdevices;
    for (auto itr = tc_notifier_info.begin(); itr != tc_notifier_info.end(); itr++) {
        for (int i = 0; i < config.currentdevices.size(); i++) {
            if (string_to_enum_device[itr->first] == config.currentdevices[i]) {
                CSHM_TC_INFO(LOG_TAG, "Got device switch for device: %d", config.currentdevices[i]);
            }
        }
    }
    return Void();
}

PALEventNotifierCallback::~PALEventNotifierCallback() {
    CSHM_TC_DBG(LOG_TAG, "PALEventNotifierCallback: deconstructor");
}

/* Helper functions to manager HIDL memory */
int32_t mapToHidlMemory(void* inp_data, int32_t size, const hidl_memory& mem)
{
    void *data = NULL;

    sp<IMemory> memory = mapMemory(mem);
    if (memory == NULL) {
        ALOGE("%s: Could not map HIDL mem to IMemory", __func__);
        return -EINVAL;
    }
    if (memory->getSize() != mem.size()) {
        ALOGE("%s: Size mismatch in memory mapping", __func__);
        return -EINVAL;
    }
    data = memory->getPointer();
    if (data == NULL) {
        ALOGE("%s: Could not get memory pointer", __func__);
        return -EINVAL;
    }

    memory->update();
    memcpy(data, inp_data, size);
    memory->commit();

    return 0;
}

int32_t getHidlMemory(void *inp_data, int32_t size, hidl_memory& hidl_mem)
{
    int32_t status = 0;

    if (ashmemAllocator == NULL) {
        ALOGE("%s: Memory allocator is invalid", __func__);
        return -ENOMEM;
    }

    ashmemAllocator->allocate(size, [&](bool success, const hidl_memory& mem) {
        if (!success) {
            ALOGE("%s: Memory allocation failed", __func__);
            status = -ENOMEM;
            return;
        }
        hidl_mem = mem;
        status = mapToHidlMemory(inp_data, size, mem);
        if (status < 0) {
            return;
        }
    });

    return status;
}

/* Helper functions for getting values from Json::Value */

int64_t get_int64(const Json::Value &node, std::string key) {
    return node[key].asInt64();
}

bool isValid(const Json::Value &json_obj, const std::string &key) {
    return json_obj.isMember(key) && !json_obj[key].isNull();
}

bool isValid(const Json::Value &json_obj) {
    return json_obj != Json::nullValue;
}

int check_and_get(const Json::Value &val_json, std::string name) {

    int ret = 0;

    if (!isValid(val_json, name))
        return ret;

    if(val_json[name].isString()) {
        std::stringstream ss;
        ss << std::hex << val_json[name].asString();
        ss >> ret;
        CSHM_TC_DBG(LOG_TAG, "Got int from string: 0x%x", ret);
    }

    if(val_json[name].isInt()) {
        ret = val_json[name].asInt();
    }

    return ret;

}

/* HIDL Wrappers */
int32_t cshm_alloc_hidl(uint32_t size, pal_cshm_info_t *mem_info) {
    int32_t ret = -EINVAL;
    const native_handle *memFdHandle = nullptr;

    if (is_pal_alive) {
       CSHM_TC_INFO(LOG_TAG, "Alloc for size 0x%x", size);
        android::sp<IPAL> pal_server = get_pal_server();
        if (pal_server == nullptr)
            return ret;
        hidl_vec<PalCShmInfo> cshmInfo(1);
        cshmInfo.data()->flags = mem_info->flags;
        cshmInfo.data()->type = (PalCShmType)(mem_info->type);
        pal_server->ipc_pal_cshm_alloc(size,cshmInfo,
                            [&](int32_t ret_, hidl_vec<PalCShmInfo> cshmInfoResult)
                            {
                                if (!ret_) {
                                mem_info->mem_id = cshmInfoResult.data()->mem_id;
                                memFdHandle = cshmInfoResult.data()->fdMemory.handle();
                                mem_info->fd = dup(memFdHandle->data[0]);
                                }
                                ALOGV("ret %d fd %d", ret_, mem_info->fd);
                                ret = ret_;
                            });
    }
    return ret;
}

int32_t cshm_dealloc_hidl(pal_cshm_id_t mem_id) {

    int ret = -EINVAL;

    if (is_pal_alive) {
        ALOGV("%s:%d: mem_id %d", __func__, __LINE__, mem_id);
        android::sp<IPAL> pal_server = get_pal_server();
        if (pal_server == nullptr)
            return ret;
        ret = pal_server->ipc_pal_cshm_dealloc(mem_id);
    }

    return ret;
}

int cshm_msg_hidl(pal_stream_handle_t *stream_handle, void *payload) {

    int status = -1;
    char param_key[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH] = "sendMsg";
    int payload_size = sizeof(pal_cshm_msg_payload_t);

    hidl_memory paramPayload;
    std::string paramString("sendMsg");

    if (is_pal_alive) {
        android::sp<IPAL> pal_server = get_pal_server();
        if (pal_server == nullptr)
            return status;
        status = getHidlMemory(payload, payload_size, paramPayload);
        if (status < 0) {
            CSHM_TC_ERR(LOG_TAG, "Cannot obtain hidl memory: %d", __func__, status);
            status = -ENOMEM;
            return status;
        }

        status = pal_server->ipc_pal_stream_set_custom_param((PalStreamHandle)stream_handle,
                                    paramString, paramPayload, payload_size);
    }
    return status;
}

int32_t get_tags_with_module_info_hidl(pal_stream_handle_t *stream_handle,
                                             size_t *size ,uint8_t *payload) {

    int32_t ret = -EINVAL;
        CSHM_TC_VERBOSE(LOG_TAG, "Enter");
        android::sp<IPAL> pal_server = get_pal_server();
        if (pal_server == nullptr)
            return ret;
        pal_server->ipc_pal_stream_get_tags_with_module_info((PalStreamHandle)stream_handle,(uint32_t)*size,
                             [&](int32_t ret_, uint32_t size_ret, hidl_vec<uint8_t> payload_ret)
                              {
                                    if (!ret_) {
                                        if (payload && (*size > 0) && (*size <= size_ret)) {
                                            memcpy(payload, payload_ret.data(), *size);
                                        } else if (payload && (*size > size_ret)) {
                                            memcpy(payload, payload_ret.data(), size_ret);
                                        }
                                        *size = size_ret;
                                    }
                                    ALOGV("ret %d size_ret %d", ret_, size_ret);
                                    ret = ret_;
                              });
    return ret;

}

uint32_t get_miid_with_tag(pal_stream_handle_t *stream_handle, uint32_t tag_u, std::vector<uint32_t> &miids) {
    int32_t rc = 0;
    size_t tag_module_size = 1024;
    std::vector<uint8_t> tag_module_info(tag_module_size);
    struct pal_tag_module_info* tag_info;
    struct pal_tag_module_mapping* tag_entry;
    std::vector<uint32_t> miid_list;

    rc = get_tags_with_module_info_hidl(stream_handle,
        &tag_module_size, (uint8_t*)(tag_module_info.data()));

    if (rc == ENODATA) {
        tag_module_info.resize(tag_module_size);

        rc = get_tags_with_module_info_hidl(stream_handle,
            &tag_module_size, (uint8_t*)tag_module_info.data());
    }

    if (rc) {
        ALOGE("failed in get only rc %d", rc);
        goto exit;
    }

    tag_info = (struct pal_tag_module_info*)tag_module_info.data();
    if (!tag_info) {
        rc = -ENODATA;
        goto exit;
    }

    tag_entry = (struct pal_tag_module_mapping*)&tag_info->pal_tag_module_list[0];
    if (!tag_entry) {
        rc = -ENODATA;
        goto exit;
    }

    for (uint32_t idx = 0, offset = 0; idx < tag_info->num_tags; idx++) {
        tag_entry += offset / sizeof(struct pal_tag_module_mapping);

        offset = sizeof(struct pal_tag_module_mapping) +
            (tag_entry->num_modules * sizeof(struct module_info));
        /* compare against all tags on the usecase */
        if (tag_entry->tag_id == tag_u) {
            for (uint32_t i = 0; i < tag_entry->num_modules; ++i) {
                miids.push_back(tag_entry->mod_list[i].module_iid);
            }
        }
    }

    if (cshm_tc_log_lvl == 15) {
        CSHM_TC_VERBOSE(LOG_TAG, "************************* MIID info*************************\n");
        CSHM_TC_VERBOSE(LOG_TAG, "Total miids: %d", miids.size());
        for(auto &miid: miids) {
            CSHM_TC_VERBOSE(LOG_TAG, "miid: 0x%x", miid);
        }
        CSHM_TC_VERBOSE(LOG_TAG, "************************* MIID info Done*************************\n");
    }

exit:
    CSHM_TC_VERBOSE(LOG_TAG, "Exit rc %d", rc);
    return rc;
}

uint8_t *form_set_get_config_payload(void *vaddr, const std::vector<std::tuple<config_type_t, uint32_t>>& commands) {

    shared_memory_ipc_protocol_t *header = nullptr;
    int num_configs = commands.size();

    if (vaddr == nullptr) {
        return nullptr;
    }

    header = (shared_memory_ipc_protocol_t *) vaddr;//(shared_memory_ipc_protocol_t *)calloc(1, sizeof(shared_memory_ipc_protocol_t) + (num_configs -1) * sizeof(shared_memory_config_t));
    header->num_config = num_configs;
    header->config_arrary[0].config_type = std::get<0>(commands[0]);
    header->config_arrary[0].pid = SHMEM_MODULE_PID;
    header->config_arrary[0].config_addr_offset = ALIGN_8(sizeof(shared_memory_ipc_protocol_t)) + (num_configs - 1) * sizeof(shared_memory_config_t);
    header->config_arrary[0].config_size = std::get<1>(commands[0]);

    return (uint8_t *)header + header->config_arrary[0].config_addr_offset;
}


#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};


int read_data_logging(uint8_t* data_prop, std::string file_name, int duration, uint32_t mem_id,
        uint32_t miid, int offset, int size, pal_stream_handle_t *stream_handle, bool avoid_release, bool write_header) // ptr should be config_addr_offset sent to custom module
{
    int status = 0;
    bool do_wait = false;
    bool parse_mf = true;
    int wait_time_ms = 5;
    uint32_t shmem_id = 0;
    uint32_t num_frames = 0;
    uint32_t bits_per_sample = 0;
    uint8_t *shmem_virt_addr = nullptr;
    uint32_t frame_size_bytes = 0;
    uint64_t *frame_start_ts_ptr = nullptr;
    uint64_t *frame_end_ts_ptr = nullptr;
    uint64_t *frame_media_fmt_ptr = nullptr;
    uint32_t *frame_actual_data_len_ptr = nullptr;
    int8_t *frame_ptr = nullptr;
    FILE *file = nullptr;
    uint32_t frame_access_idx = 0;
    uint64_t last_frame_accessed_end_ts = 0;
    uint32_t num_valid_frames = 0;
    uint64_t access_start_ts = 0;
    uint64_t num_frames_written = 0;
    shared_memory_ipc_protocol_t *ipc_header;
    pal_cshm_msg_payload_t payload = {};
    struct wav_header header;
    std::atomic<bool> keep_reading(true);

    int total_bytes = 0;
    int pcm_frames = 0;
    int channels = 0;
    int rate = 0;

    num_frames = *(uint32_t*)data_prop;
    data_prop += sizeof(uint32_t);

    frame_size_bytes = *(uint32_t*)data_prop;
    data_prop += sizeof(uint32_t);

    frame_start_ts_ptr = (uint64_t *)data_prop;
    frame_end_ts_ptr   = frame_start_ts_ptr + num_frames;
    frame_media_fmt_ptr = (frame_end_ts_ptr + num_frames);
    frame_actual_data_len_ptr = (uint32_t *)(frame_media_fmt_ptr + num_frames);
    frame_ptr = (int8_t *)(frame_actual_data_len_ptr + num_frames);

    CSHM_TC_DBG(LOG_TAG, "num_frame: %d and frame size %d", num_frames, frame_size_bytes);

    //file_name += "_" + std::to_string(mf->sampling_rate) + "_" +std::to_string(mf->bit_width) + "_" + std::to_string(mf->num_channel);
    file = fopen(file_name.c_str(), "w");
    if (file == nullptr) {
        ALOGE("%s: Failed to open file %s", LOG_TAG, file_name.c_str());
        return false;
    }

    keep_reading.store(true);
    std::thread timer_thread(wait_for_duration, duration, std::ref(keep_reading));
    while(keep_reading.load()) {
        if (is_adsp_down.load()) {
            return -2;
        }
        if (do_wait) {
            usleep(wait_time_ms);
            do_wait = false;
        }
        /*
            Each one custom module write contains
            shared_memory_ipc_protocol_t|num_frames|each_frame_size_byte|num_frames*[start_timestamp|end_timestamp|media_format|
            written_frame_size|frame_size]
        */
        // Capture the timestamp when frame access is started.
        access_start_ts = get_timeofday();
        num_valid_frames = 0;
        for (int i = 0; i < num_frames; i++) {
            /*
                * if i-th frame end timestamp is greater than start timestamp then this frame is valid and not actively
                * being written. if i-th frame start timestamp is greater than the end timestamp of the last accessed frame
                * then this frame valid and not yet accessed.
                * */
            if (last_frame_accessed_end_ts < frame_start_ts_ptr[i] &&
                frame_end_ts_ptr[i] > frame_start_ts_ptr[i])
            {
                num_valid_frames++;

            }
        }

        if (num_valid_frames == 0)
        {
            do_wait = true;
            continue;
        }

        if (num_valid_frames > (num_frames / 2))
        {
            ALOGE("%s: Module writing too fast", LOG_TAG);
            status = -1;
            break;
        }

        if ((get_timeofday() - access_start_ts) >= 1000)
        {
            // spent significant time in calculating valid frames
            // can not rely on the calculation of "num_valid_frames". Do it again.
            ALOGE("Spent more time in getting valid frames. Took %d ms",(get_timeofday() - access_start_ts));
            continue;
        }

        int8_t *frame_access_ptr = frame_ptr + (frame_access_idx * frame_size_bytes);
        if (parse_mf) {
            shmem_media_format_t *mf = (shmem_media_format_t *)(&frame_media_fmt_ptr[frame_access_idx]);
            channels = mf->num_channel;
            rate = mf->sampling_rate;
            bits_per_sample = mf->bit_width;

            fseek(file, sizeof(struct wav_header), SEEK_SET);

            CSHM_TC_VERBOSE(LOG_TAG, "Media format: Channel: %d, Bit Width: %d, SR: %d for file: %s",
                    mf->num_channel, mf->bit_width, mf->sampling_rate, file_name.c_str());
            parse_mf = false;
        }

        num_frames_written = fwrite((const char *)frame_access_ptr,sizeof(int8_t),frame_actual_data_len_ptr[frame_access_idx], file );

        if (write_header)
            total_bytes += num_frames_written;

        if (num_frames_written < 0 ) {
            ALOGE("file write failed");
            status = -1;
        }

        last_frame_accessed_end_ts = frame_end_ts_ptr[frame_access_idx];
        frame_access_idx = (frame_access_idx + 1) % num_frames;
    }
    timer_thread.join();
    CSHM_TC_VERBOSE(LOG_TAG, "Timer thread joined for file: %s", file_name.c_str());

    if (write_header) {
        header.riff_id = ID_RIFF;
        header.riff_sz = 0;
        header.riff_fmt = ID_WAVE;
        header.fmt_id = ID_FMT;
        header.fmt_sz = 16;
        header.audio_format = FORMAT_PCM;
        header.bits_per_sample = bits_per_sample;
        pcm_frames = (total_bytes)/ (channels * (bits_per_sample >> 3));
        header.num_channels = channels;
        header.sample_rate = rate;
        header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
        header.block_align = channels * (header.bits_per_sample / 8);
        header.data_id = ID_DATA;
        header.data_sz = pcm_frames * header.block_align;
        header.data_sz = pcm_frames * header.block_align;
        header.riff_sz = header.data_sz + sizeof(header) - 8;
        fseek(file, 0, SEEK_SET);
        fwrite(&header, sizeof(struct wav_header), 1, file);
    }

    fclose(file);

    if (avoid_release)
        return status;

    payload.mem_id = mem_id;
    payload.offset = offset;
    payload.length = size;
    payload.flags = 0x02;
    payload.miid = miid;
    status = cshm_msg_hidl((pal_stream_handle_t *)stream_handle, &payload);

    return status;
}

uint64_t get_data_logging_size (int num_frames, int frame_size) {

    uint64_t total_size = ALIGN_8(sizeof(shared_memory_ipc_protocol_t))
                            + sizeof(uint32_t) //num_frames
                            + num_frames * (
                                sizeof(uint64_t)
                                +sizeof(uint64_t)
                                +sizeof(shmem_media_format_t)
                                +sizeof(uint32_t)
                                +frame_size
                                );
    return total_size;
}


/*
        Caller should check if it has allocated enough memory as in return size
*/
int form_data_logging_header(uint8_t * ptr, int num_frames, int frame_size) {

    uint64_t total_size = get_data_logging_size(num_frames, frame_size);
    CSHM_TC_VERBOSE(LOG_TAG, "Actually got header pointer: %pK", ptr);

    shared_memory_ipc_protocol_t *ipc_header = (shared_memory_ipc_protocol_t *) ptr;
    ipc_header->num_config                          = 1;
    ipc_header->config_arrary[0].config_type        = DATA_LOGGING;
    ipc_header->config_arrary[0].pid                = 0;
    ipc_header->config_arrary[0].config_addr_offset = ALIGN_8(sizeof(shared_memory_ipc_protocol_t));
    ipc_header->config_arrary[0].config_size = total_size - sizeof(shared_memory_ipc_protocol_t);
    uint32_t *frame_sz_ptr = nullptr;
    frame_sz_ptr =(uint32_t *)( ptr + ipc_header->config_arrary[0].config_addr_offset);
    *frame_sz_ptr = num_frames;
    frame_sz_ptr += 1;
    *frame_sz_ptr = frame_size;

    CSHM_TC_DBG(LOG_TAG, "Total required memory for data logging: 0x%x", total_size);
    return ipc_header->config_arrary[0].config_addr_offset;
}

int parse_msg_payload(const Json::Value &payload_json, uint32_t *alloc_id, uint32_t *offset, uint32_t *size, uint32_t *miid, uint32_t *flags) {

    if (isValid(payload_json, "id") && alloc_id)
        *alloc_id = payload_json["id"].asInt();
    else {
        CSHM_TC_ERR(LOG_TAG, "Alloc ID missing");
        return -1;
    }

    if (offset)
        *offset = check_and_get(payload_json, "offset");
    if (size)
        *size = check_and_get(payload_json, "size");
    if (flags)
        *flags = check_and_get(payload_json, "flags");
    if (miid)
        *miid= check_and_get(payload_json, "miid");

    return 0;
}

/*
    Input format

    {
        "size"
    }

*/
int test_alloc(const Json::Value &alloc_info) {

    uint32_t mem_id = -1;
    int size = -1;
    int type = -1;
    int flag = 0;
    int status = -1;
    pal_cshm_info_t mem_info;
    void *vaddr = nullptr;
    Json::Value alloc_info_ext = alloc_info;

    size = check_and_get(alloc_info, "size");
    type = check_and_get(alloc_info, "type");
    flag = check_and_get(alloc_info, "flag");

    mem_info.type = (pal_cshm_type)type;
    mem_info.flags = flag;

    status = cshm_alloc_hidl(size, &mem_info);

    vaddr  = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_info.fd, 0);

    if (vaddr == nullptr) {
        ALOGE("%s: Failed to do_alloc", LOG_TAG);
        return -1;
    }

    alloc_info_ext["mem_id"] = mem_info.mem_id;
    alloc_info_ext["vaddr"] = (uint64_t) vaddr;
    alloc_info_ext["fd"] = mem_info.fd;
    alloced_mem_info.push_back(alloc_info_ext);

    return status;
}

int test_dealloc(const Json::Value &dealloc_info) {
    int status = -1;
    int alloc_id = check_and_get(dealloc_info, "id");

    for(auto &alloc_item:alloced_mem_info){
        if (alloc_item["id"].asInt() == alloc_id) {
            int mem_id = check_and_get(alloc_item, "mem_id");
            status = cshm_dealloc_hidl(mem_id);
            break;
        } else {
            CSHM_TC_ERR(LOG_TAG, "Failed to get with id %d %d", alloc_item["id"].asInt(), alloc_id);
        }
    }

    return status;
}


int test_set_get_cfg(const Json::Value &cfg_cmd, int cmd) {
    int status = -1;
    int module_pos = -1;
    int avoid_release = 0;
    config_type_t cfg = (cmd == CMD_SET_CFG) ? SET_CFG : GET_CFG;
    uint32_t offset = 0;
    uint32_t alloc_id = 0;
    uint32_t data_size = MAX_SET_CFG_SIZE;
    uint32_t mem_id = 0;
    uint32_t miid = 0;
    uint32_t flags = 0;
    int seed = 1;
    char param_key[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH] = "sendMsg";
    pal_cshm_msg_payload_t payload = {};
    void *vaddr = nullptr;
    uint8_t *cfg_ptr = nullptr;
    pal_stream_handle_t *stream_handle = nullptr;
    shared_memory_ipc_protocol_t *header = nullptr;
    std::vector<uint32_t> miids;
    Json::Value alloc_info_json = Json::nullValue;
    Json::Value misc_json = Json::nullValue;

    status = parse_msg_payload(cfg_cmd["payload"], &alloc_id, &offset, &data_size, &miid, &flags);
    if (status) {
        CSHM_TC_ERR(LOG_TAG, "Mandatory payload info missing");
        return -1;
    }

    if (data_size <= 0) {
        ALOGE("%s: %s: data size is invalid", LOG_TAG, __func__);
        return -1;
    }

    if (data_size > MAX_SET_CFG_SIZE) {
        CSHM_TC_INFO(LOG_TAG, "Set data size is more than max size, using first %d bytes only", MAX_SET_CFG_SIZE);
        data_size = MAX_SET_CFG_SIZE;
    }

    if (isValid(cfg_cmd, "miscInfo")) {
        misc_json = cfg_cmd["miscInfo"];
        stream_handle = tc_notifier_info[misc_json["usecase"].asString()];
        if (stream_handle == nullptr) {
            CSHM_TC_ERR(LOG_TAG, "stream handle null for usecase: %s", misc_json["usecase"].asString().c_str());
            return -EINVAL;
        }
        module_pos = misc_json["module_pos"].asInt();

        if (isValid(misc_json, "seed")) {
            seed = misc_json["seed"].asInt();
        }

        if (isValid(misc_json, "avoid_release")) {
            avoid_release = misc_json["avoid_release"].asInt();
        }
        CSHM_TC_DBG(LOG_TAG, "UsecaseInfo: Stream: %pK, module_pos: %d, seed: %d, avoid_release: %d", stream_handle, module_pos, seed, avoid_release);
    }
    else {
        CSHM_TC_ERR(LOG_TAG, "UsecaseInfo not present!!");
        return -EINVAL;
    }

    for(auto &alloc_item:alloced_mem_info){
        if (alloc_item["id"].asInt() == alloc_id) {
            alloc_info_json = alloc_item;
        }
    }

    vaddr = (void *)get_int64(alloc_info_json, "vaddr");
    mem_id = check_and_get(alloc_info_json, "mem_id");

    if (vaddr == nullptr) {
        CSHM_TC_ERR(LOG_TAG, "vaddr is null!");
        return -1;
    }

    /* If no valid MIID in test conf, get all available ones
       otherwise, skip
     */
    if (miid == 0) {
        status = get_miid_with_tag(stream_handle, SHMEM_MODULE_TAG, miids);
        if (status) {
            CSHM_TC_ERR(LOG_TAG, "Failed to get miid");
            return status;
        }
    }

    cfg_ptr = form_set_get_config_payload((uint8_t *)vaddr+offset, {std::make_tuple(cfg,data_size)});

    payload.mem_id = mem_id;
    payload.offset = offset;
    payload.length = data_size;
    payload.flags = flags;
    /* If miid mentioned in payload, give preference to that over module_pos in usecaseInfo */
    if (miid > 0)
        payload.miid = miid;
    else if (module_pos > 0 && module_pos < miids.size() + 1)
        payload.miid = miids[module_pos - 1];
    else {
        CSHM_TC_ERR(LOG_TAG, "No valid miid found, miid: 0x%x; module_pos: %d", miid, module_pos);
        return -1;
    }


    /*TODO: Use a better random sequence generator */
    if (cfg == SET_CFG) {
        srand(seed);
        for(int i = 0; i < data_size; i++) {
            cfg_ptr[i] = rand() % 256;
            CSHM_TC_INFO(LOG_TAG, "Set data for module (0x%x) - Byte: %d, value: %d", payload.miid, i, cfg_ptr[i]);
        }
        CSHM_TC_DBG(LOG_TAG, "Set cfg payload - mem_id: 0x%x, offset: %d, length: %d, miid: 0x%x, flags: 0x%x",
                                                payload.mem_id, payload.offset, payload.length, payload.miid, payload.flags);
    }

    status = cshm_msg_hidl((pal_stream_handle_t *)stream_handle, &payload);

    if (status != 0) {
        CSHM_TC_ERR(LOG_TAG, "MSG call failed!");
        return status;
    }

    if (cfg == GET_CFG) {
        for(int i = 0; i < data_size; i++) {
            CSHM_TC_INFO(LOG_TAG, "Get data for module (0x%x) - Byte: %d, value: %d", payload.miid, i, cfg_ptr[i]);
        }
    }

    if (avoid_release)
        return status;

    payload.flags = 0x2;
    status = cshm_msg_hidl((pal_stream_handle_t *)stream_handle, &payload);

    if (status!=0) {
        CSHM_TC_ERR(LOG_TAG, "Failed to release memory");
    }

    return status;

}

int test_data_logging(const Json::Value &cmd_json) {
    int status = -1;
    uint32_t alloc_id = 0;
    int is_async =  0;
    int read_offset = 0;
    uint32_t offset = 0x0;
    uint32_t size = 0;
    uint32_t flags = 0x0;
    uint32_t miid = 0x0;
    int duration = DEFAULT_DL_DURATION;
    int num_frames = DEFAULT_FRAME_NUM;
    int frame_size = DEFAULT_FRAME_SIZE;
    int module_pos = -1;
    uint32_t mem_id = 0;
    void *vaddr = nullptr;
    pal_stream_handle_t *stream_handle = nullptr;
    pal_cshm_msg_payload_t payload = {};
    std::string file_name = "/data/data_logging_test.raw";
    std::vector<uint32_t> miids;
    Json::Value alloc_info_json = Json::nullValue;
    Json::Value misc_json = Json::nullValue;
    bool avoid_release = false;
    int write_header = 0;

    status = parse_msg_payload(cmd_json["payload"], &alloc_id, &offset, &size, &miid, &flags);
    if (status) {
        CSHM_TC_ERR(LOG_TAG, "Mandatory payload info missing");
        return -1;
    }

    if (isValid(cmd_json, "miscInfo")) {
        misc_json = cmd_json["miscInfo"];

        if(isValid(misc_json, "usecase"))
            stream_handle = tc_notifier_info[misc_json["usecase"].asString()];

        if (stream_handle == nullptr) {
            CSHM_TC_ERR(LOG_TAG, "stream handle null for usecase: %s", misc_json["usecase"].asString().c_str());
            return -EINVAL;
        }

        if (isValid(misc_json, "module_pos"))
            module_pos = misc_json["module_pos"].asInt();

        if (isValid(misc_json, "avoid_release")) {
            avoid_release = misc_json["avoid_release"].asInt();
        }

        CSHM_TC_DBG(LOG_TAG, "UsecaseInfo: Stream: %pK, module_pos: %d", stream_handle, module_pos);
    }
    else {
        CSHM_TC_ERR(LOG_TAG, "Misc info not present!!");
        return -EINVAL;
    }

    /* Check if optional info exists and populate */
    if (isValid(misc_json, "filename"))
        file_name = misc_json["filename"].asString();
    if (isValid(misc_json, "duration"))
        duration = misc_json["duration"].asInt();
    if (isValid(misc_json, "is_async"))
        is_async = misc_json["is_async"].asInt();
    if (isValid(misc_json, "num_frames"))
        num_frames = misc_json["num_frames"].asInt();
    if (isValid(misc_json, "frame_size"))
        frame_size = misc_json["frame_size"].asInt();
    if (isValid(misc_json, "write_header"))
        write_header = misc_json["write_header"].asInt();

    /* Find the memory block to use for data logging */
    for(auto &alloc_item:alloced_mem_info){
        if (alloc_item["id"].asInt() == alloc_id) {
            alloc_info_json = alloc_item;
        }
    }

    if (!isValid(alloc_info_json)) {
        CSHM_TC_ERR(LOG_TAG, "Failed to find alloced memory with id: %d", alloc_id);
        return -1;
    }

    if (isValid(alloc_info_json, "vaddr"))
        vaddr = (void *)(alloc_info_json["vaddr"].asInt64());

    if(isValid(alloc_info_json, "mem_id"))
        mem_id = alloc_info_json["mem_id"].asInt();

    CSHM_TC_VERBOSE(LOG_TAG, "Forming logging header with vaddr : %pK, offset: %d", vaddr, offset);
    read_offset = form_data_logging_header((uint8_t *)vaddr+offset, num_frames, frame_size);

    status = get_miid_with_tag(stream_handle, SHMEM_MODULE_TAG, miids);
    if (status) {
        CSHM_TC_ERR(LOG_TAG, "Failed to get miid using stream handle %pK", stream_handle);
        return status;
    }

    payload.mem_id = mem_id;
    payload.offset = offset;
    payload.length = size;
    payload.flags = flags;

    /*If miid is mentioned in test conf, give preference to that*/
    if (miid > 0)
        payload.miid = miid;
    else if (module_pos > 0 && module_pos <= miids.size())
        payload.miid = miids[module_pos - 1];
    else {
        CSHM_TC_ERR(LOG_TAG, "No valid miid found, miid: 0x%x; module_pos: %d", miid, module_pos);
        return -1;
    }

    status = cshm_msg_hidl(stream_handle, &payload);

    if (status) {
        CSHM_TC_ERR(LOG_TAG, "SendMsg failed!");
        return status;
    }

    if (is_async == 0)
        status = read_data_logging((uint8_t *)vaddr + read_offset+offset, file_name, duration, mem_id, payload.miid, offset, size, stream_handle, avoid_release, write_header);
    else
        data_logging_futures.emplace_back(std::async(std::launch::async,read_data_logging,(uint8_t *)vaddr + read_offset+offset, file_name, duration,
                        mem_id, payload.miid,offset, size, stream_handle, avoid_release, write_header));

    if (status) {
        CSHM_TC_ERR(LOG_TAG, "reading data from module failed");
        return -1;
    }

    return status;
}

int do_wait(const Json::Value &wait_json) {
    int wait_in_sec = -1;
    int ret = 0;

    if (isValid(wait_json, "duration")) {
        wait_in_sec = wait_json["duration"].asInt();
    }

    if (wait_in_sec > 0){
        CSHM_TC_VERBOSE(LOG_TAG, "Waiting for %d seconds", wait_in_sec);
        sleep(wait_in_sec);
    }
    else {
        std::cout<<"Press Enter to continue"<<std::endl;
        std::cin.get();
    }

    return 0;
}

/*
   Input Json:;Value format
{
    "name": "",
    "payload": {}

}
*/

int run_command(const Json::Value &command_json){

    int status = -1;
    int command = -1;

    if (isValid(command_json, "name")) {
        command = cmd_look_up[command_json["name"].asString()];
    }
    else {
        ALOGE("%s: 'name' missing for this command", LOG_TAG);
        return -1;
    }

    std::cout<<"Running CMD: "<< command_json["name"].asString() << std::endl;
    CSHM_TC_INFO(LOG_TAG, " Running CMD: %s", command_json["name"].asString().c_str());

    switch (command){
            case CMD_ALLOC:
                status = test_alloc(command_json["payload"]);
                break;
            case CMD_DEALLOC:
                status = test_dealloc(command_json["payload"]);
                break;
            case CMD_SET_CFG:
                /* fallthrough */
            case CMD_GET_CFG:
                status = test_set_get_cfg(command_json, command);
                break;
            case CMD_DATA_LOGGING:
                status = test_data_logging(command_json);
                break;
            case CMD_WAIT:
                status = do_wait(command_json["payload"]);
                break;
            default:
                status = -1;
                ALOGE("%s: Unsupported command", LOG_TAG);
                break;
        }
    return status;
}


/* Input Json Format
   {
        "name":
        "streams"
        "devices"
   }
*/

int register_with_notifier(const Json::Value &notifier_json) {
    int status = -1;
    int streams_size = 0;
    int devices_size = 0;
    Json::Value streams = Json::nullValue;
    Json::Value devices= Json::nullValue;

    if (isValid(notifier_json, "streams")) {
        streams = notifier_json["streams"];
        streams_size = streams.size();
    }
    if (isValid(notifier_json, "devices")) {
        devices= notifier_json["devices"];
        devices_size = devices.size();
    }

    hidl_vec<PalStreamType> str(streams_size);
    hidl_vec<PalDeviceId> dev(devices_size);

    for (Json::ArrayIndex i = 0; i < streams_size; i++) {
        CSHM_TC_ERR(LOG_TAG, "Found stream: %s", streams[i].asString().c_str());
        str[i] =  string_to_enum_stream[streams[i].asString()];
        tc_notifier_info.insert({streams[i].asString(), nullptr});
    }

    for (Json::ArrayIndex i = 0; i < devices_size; i++) {
        CSHM_TC_ERR(LOG_TAG, "Found device: %s", devices[i].asString().c_str());
        dev[i] =  string_to_enum_device[devices[i].asString()];
        tc_notifier_info.insert({devices[i].asString(), nullptr});
    }

    CSHM_TC_INFO(LOG_TAG, "Registering with notifier");
    PALEventNotifier_client->ipc_pal_notify_register_callback(ClbkBinder, dev, str);
    return status;
}

int unregister_with_notifier() {
    int status = -1;
    hidl_vec<PalStreamType> str(0);
    hidl_vec<PalDeviceId> dev(0);

    CSHM_TC_INFO(LOG_TAG, "Unregistering with notifier");
    PALEventNotifier_client->ipc_pal_notify_register_callback(nullptr, dev, str);
    return status;
}

/*
   Input Json value format:
[
    {},
    {},
    {}
]
*/

int execute_single_test(const Json::Value &test_case) {

    int status = -1;
    int dl_status = 0;

    /* Reset the stored alloc and tc notifier info before starting each test case*/
    alloced_mem_info.clear();
    tc_notifier_info.clear();

    if (test_case[0]["name"].asString() == std::string("palnotifier")) {
        status = register_with_notifier(test_case[0]);
    } else {
        CSHM_TC_ERR(LOG_TAG, "Pal notifer info not present");
        return -1;
    }

    for (Json::ArrayIndex i = 1; i < test_case.size(); i++) {
        status = run_command(test_case[i]);
        if (status != 0) {
            std::cout<<"Command failed, exiting.."<<std::endl;
            break;
        }
    }

    /* Wait till all data logging tasks are finished. Fail the test-case even
       if one of those tasks failed.
   */

    for(int i =0; i< data_logging_futures.size(); i++) {
        std::cout<< "Waiting for data logging task ("<<i<<")to finish.."<<std::endl;
        dl_status = data_logging_futures[i].get();
        CSHM_TC_INFO(LOG_TAG, "Data logging task: %d, status = %d",i, dl_status);
        if (dl_status != 0)
            status = dl_status;
    }
    std::cout<< "TC Execution done!"<<std::endl;

    unregister_with_notifier();

    return status;
}

/*
Input Json::Value format
{
    "tc_1":[],
    "tc_2":[]
}
*/
int find_and_run_test(int tc_num, const Json::Value &test_cases) {
    std::string base = "tc_" + std::to_string(tc_num);
    int status = -1;

    for (auto itr = test_cases.begin(); itr != test_cases.end(); itr++ ) {
        if (itr.key().asString() == base) {
            std::cout<<"Running test case " << tc_num <<std::endl;
            CSHM_TC_INFO(LOG_TAG, "Running test case %d", tc_num);
            status = execute_single_test(*itr);
        }
    }

    return status;
}

Json::Value parse_json(std::string file_name) {
    Json::Reader reader;
    Json::Value root = Json::nullValue;
    std::ifstream ifs(file_name);
    if (ifs) {
        reader.parse(ifs, root);
    }

    return root;
}

void parse_and_apply_configs(const Json::Value& root) {

    if (!isValid(root, "global_configs")) {
        CSHM_TC_INFO(LOG_TAG, "Global configs not present");
        return;
    }

    if (!isValid(root["global_configs"], "logging_level")) {
        CSHM_TC_ERR(LOG_TAG, "Logging level not present");
        return;
    }

    cshm_tc_log_lvl = root["global_configs"]["logging_level"].asInt();

    return;
}

void print_help() {
    std::cout << "Usage: palcshm_test [options]\n"
              << "Options:\n"
              << "  --conf <file_path>\n"
              << "      Test conf file. Default path is " << DEFAULT_TEST_CONF<< "\n"
              << "  -tc <number>\n"
              << "      Test case to execute from test conf file\n"
              << "  --dlsize <num_frames> <frame_size>\n"
              << "      Print minimum memory required for a given number\n"
              << "      of frames and size of frames. If both are not\n"
              << "      given, then default values (" << DEFAULT_FRAME_NUM<< " frames, "
              << DEFAULT_FRAME_SIZE << " bytes per frame) will be used\n"
              << "      for calculation.\n"
              << "  --help, -h\n"
              << "      Show this help message\n";

    return;
}

 int main(int argc, char *argv[]) {

    int ret = -1;
    int i = 1;
    int tc_num = -1;
    Json::Value root = Json::nullValue;
    std::string conf_file = DEFAULT_TEST_CONF;
    bool get_dl_size = false;
    uint32_t num_frames = DEFAULT_FRAME_NUM;
    uint32_t frame_size = DEFAULT_FRAME_SIZE;

    CSHM_TC_INFO(LOG_TAG, "Starting test client");

    while((i<argc) && (i+1<=argc)) {
        if (strcmp(argv[i], "-tc") == 0) {
            tc_num = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "--conf") == 0) {
            conf_file =  std::string(argv[i+1]);
        } else if ((strcmp(argv[i], "--help") && strcmp(argv[i], "-h")) == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "--dlsize") == 0) {
            get_dl_size = true;
            if (argc > 2) {
                num_frames = atoi(argv[i+1]);
                frame_size = atoi(argv[i+2]);
            }
        }
        i++;
    }

    if (get_dl_size) {
        std::cout<< get_data_logging_size(num_frames, frame_size) <<std::endl;
        return 0;
    }

    CSHM_TC_DBG(LOG_TAG, "Using conf file: %s", conf_file.c_str());

    root = parse_json(conf_file);
    if (root == Json::nullValue) {
        CSHM_TC_ERR(LOG_TAG, "Invalid test file");
        return -1;
    }

    /* Apply logging level */
    parse_and_apply_configs(root);

    /* Obtain notifier service, will register as part of test case setup */
    PALEventNotifier_client = IPALEventNotifier::getService();
    if (PALEventNotifier_client == nullptr) {
        CSHM_TC_ERR(LOG_TAG, "Failed to get PALEventNotifier service!");
        return -1;
    }
    ClbkBinder = new PALEventNotifierCallback();
    PalCallbackBinder = new PalCallback();

    /* Register with PAL for SSR callback  */
    if (is_pal_alive) {
        android::sp<IPAL> pal_server = get_pal_server();
        if (pal_server == nullptr)
            return -EINVAL;
        sp<IPALCallback> ClbkBinder = new PalCallback();
        ret = pal_server->ipc_pal_register_global_callback(PalCallbackBinder, CLIENT_COOKIE);
    }

    ret = find_and_run_test(tc_num, root);

    std::cout<<"Test case " << tc_num << " status " << ret <<std::endl;
    return 0;
 }

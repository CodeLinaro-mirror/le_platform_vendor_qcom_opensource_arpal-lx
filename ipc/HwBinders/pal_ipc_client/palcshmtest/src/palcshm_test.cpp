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
#include "palcshm_test.h"
#include <json/json.h>
#include <fstream>
#include <thread>
#include <future>
#include <string>

#define LOG_TAG "cshm_test_client"
#define DEFAULT_MEM_SIZE 0xA000
#define DEFAULT_MEM_SIZE_DL 0xD000
#define MAX_SET_CFG_SIZE 64
#define DEFAULT_FRAME_NUM 50
#define DEFAULT_FRAME_SIZE 1024
#define DEFAULT_DL_DURATION 10

#define CLIENT_COOKIE 0x6373686d
#define DEFAULT_MIID 0x4319//0x4314   /* CSHM client - Example*/
#define SHMEM_MODULE_TAG 0xc0000057


std::atomic<bool> keep_reading(true);
std::atomic<bool> is_adsp_down(false);

#define ALIGN_8(x) (((x) + 7) & (~7))

using namespace std;
static const uint32_t TAG_MODULE_LIST_SIZE = 1024;

static std::vector<Json::Value> alloced_mem_info;
static std::vector<std::future<int>> data_logging_futures;
static pal_stream_handle_t *stream_handle_global;

enum {
    CMD_ALLOC = 1,
    CMD_DEALLOC,
    CMD_SET_CFG,
    CMD_GET_CFG,
    CMD_DATA_LOGGING,
    CMD_WAIT,
};

std::map<std::string, int> cmd_look_up = {{"alloc",CMD_ALLOC }, {"dealloc", CMD_DEALLOC}, {"set_cfg", CMD_SET_CFG}, {"get_cfg", CMD_GET_CFG}, {"data_logging", CMD_DATA_LOGGING}, {"wait", CMD_WAIT}};

void wait_for_duration(int duration) {
    ALOGE("%s: Waiting for duration: %d", LOG_TAG, duration);
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

void print_help() {
    printf("-tc <number>:    Execute test case <number>\n\t\t\t\t 1 - Alloc/Dealloc test\n\t\t\t\t 2 - Get MIID test\n-h OR --help: Show this help");
    return;
}

static int32_t ssr_callback(uint32_t event_id, uint32_t *event_data,
                                    uint64_t cookie) {

    switch(event_id) {
        case PAL_SND_CARD_STATE:
            ALOGI("%s: Received SSR event: %d", LOG_TAG, *event_data);
            if (*event_data == CARD_STATUS_OFFLINE) {
                is_adsp_down.store(true);
            } else
                is_adsp_down.store(false);
            break;
        default:
            ALOGE("%s: Unrecognized event %d", LOG_TAG, event_id);
            break;
    }
    return 0;
}

static uint32_t get_miid_with_tag(pal_stream_handle_t *stream_handle, uint32_t tag_u, std::vector<uint32_t> &miids) {
    int32_t rc = 0;
    size_t tag_module_size = 1024;
    std::vector<uint8_t> tag_module_info(tag_module_size);
    struct pal_tag_module_info* tag_info;
    struct pal_tag_module_mapping* tag_entry;
    std::vector<uint32_t> miid_list;

    rc = pal_stream_get_tags_with_module_info(stream_handle,
        &tag_module_size, (uint8_t*)(tag_module_info.data()));
    if (rc == ENODATA) {
        tag_module_info.resize(tag_module_size);

        rc = pal_stream_get_tags_with_module_info(stream_handle,
            &tag_module_size, (uint8_t*)tag_module_info.data());
    }
    if (rc) {
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
exit:
    ALOGV("Exit rc:%d", rc);
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




int read_data_logging(uint8_t* data_prop, string file_name, int duration) // ptr should be config_addr_offset sent to custom module
{
    bool read_failed = false;
    bool do_wait = false;
    int wait_time_ms = 5;
    shared_memory_ipc_protocol_t *ipc_header;
    uint32_t shmem_id;
    uint8_t *shmem_virt_addr;
    uint32_t num_frames;
    uint32_t frame_size_bytes;
    uint64_t *frame_start_ts_ptr;
    uint64_t *frame_end_ts_ptr;
    uint64_t *frame_media_fmt_ptr;
    uint32_t *frame_actual_data_len_ptr;
    int8_t *frame_ptr;
    uint32_t frame_access_idx = 0;
    uint64_t last_frame_accessed_end_ts = 0;
    uint32_t num_valid_frames = 0;
    uint64_t access_start_ts = 0;
    uint64_t num_frames_written = 0;
    FILE *file = nullptr;

    num_frames = *(uint32_t*)data_prop;
    data_prop += sizeof(uint32_t);

    frame_size_bytes = *(uint32_t*)data_prop;
    data_prop += sizeof(uint32_t);

    frame_start_ts_ptr = (uint64_t *)data_prop;
    frame_end_ts_ptr   = frame_start_ts_ptr + num_frames;
    frame_media_fmt_ptr = (frame_end_ts_ptr + num_frames);
    frame_actual_data_len_ptr = (uint32_t *)(frame_media_fmt_ptr + num_frames);
    frame_ptr = (int8_t *)(frame_actual_data_len_ptr + num_frames);

    ALOGI("%s: num_frame: %d and frame size %d",LOG_TAG, num_frames,  frame_size_bytes);

    shmem_media_format_t *mf = (shmem_media_format_t *)frame_media_fmt_ptr;

    file = fopen(file_name.c_str(), "w");
    if (file == nullptr) {
        ALOGE("%s: Failed to open file %s", LOG_TAG, file_name.c_str());
        return false;
    }

    keep_reading.store(true);
    std::thread timer_thread(wait_for_duration, duration);
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
            read_failed = true;
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
        num_frames_written = fwrite((const char *)frame_access_ptr,sizeof(int8_t),frame_actual_data_len_ptr[frame_access_idx], file );
        if (num_frames_written < 0 ) {
            ALOGE("file write failed");
            read_failed = true;
        }
        last_frame_accessed_end_ts = frame_end_ts_ptr[frame_access_idx];
        frame_access_idx = (frame_access_idx + 1) % num_frames;
    }
    timer_thread.join();
    fclose(file);
    return read_failed;
}

/*
        Caller should check if it has allocated enough memory as in return size
*/
int  form_data_logging_header(uint8_t * ptr, int num_frames, int frame_size) {

    uint64_t total_size = ALIGN_8(sizeof(shared_memory_ipc_protocol_t))
                            + sizeof(uint32_t) //num_frames
                            + num_frames * (
                                sizeof(uint64_t)
                                +sizeof(uint64_t)
                                +sizeof(shmem_media_format_t)
                                +sizeof(uint32_t)
                                +frame_size
                                );
    
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

    return ipc_header->config_arrary[0].config_addr_offset;
}

static uint32_t get_type_from_json(const Json::Value &node) {
    if (node["type"] != Json::nullValue && node["type"].isUInt()) {
        return node["type"].asUInt();
    }
    return 0;
}

int get_int(const Json::Value &node, std::string key) {
    return node[key].asInt();
}

int64_t get_int64(const Json::Value &node, std::string key) {
    return node[key].asInt64();
}


bool isValid(const Json::Value &json_obj, const std::string &key) {
    return json_obj.isMember(key) && !json_obj[key].isNull();
}

bool isValid(const Json::Value &json_obj) {
    return json_obj != Json::nullValue;
}



void *do_alloc(int alloc_size, int type, int flags, uint32_t *mem_id) {

    int status = -1;
    pal_cshm_info_t mem_info;
    void *vaddr = nullptr;

    mem_info.type = (pal_cshm_type)type;
    mem_info.flags = flags;
    status = pal_cshm_alloc(alloc_size, &mem_info);
    if (status) {
        ALOGE("%s: %s: pal_cshm_alloc failed!", LOG_TAG, __func__);
        return nullptr;
    }
    /*mmap the fd to get a virtual address, which is easier to work with*/
    vaddr  = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_info.fd, 0);
    if (vaddr == nullptr) {
        ALOGE("%s: %s: mmap failed with %d", LOG_TAG, __func__, errno);
        return nullptr;
    }

    *mem_id = mem_info.mem_id;
    ALOGI("%s: %s: do_alloc for size %d successful", LOG_TAG, __func__,alloc_size);

    return vaddr;
}



/*
    Input format

    {
        "size"
    }

*/
int test_alloc(const Json::Value &alloc_info) {

    Json::Value alloc_info_ext = alloc_info;
    void *vaddr = nullptr;
    uint32_t mem_id = -1;
    int size = -1;
    int type = -1;
    int flag = 0;

    if (isValid(alloc_info, "size") && isValid(alloc_info, "type")) {
        size = alloc_info["size"].asInt();
        type = alloc_info["type"].asInt();
    }
    else {
        ALOGE("%s: Critial info not available, aborting %s", LOG_TAG, __func__);
        return -1;
    }

    if (isValid(alloc_info, "flags"))
        flag = alloc_info["flag"].asInt();

    vaddr = do_alloc(size, type, flag, &mem_id);
    if (vaddr == nullptr) {
        ALOGE("%s: Failed to do_alloc", LOG_TAG);
        return -1;
    }
    alloc_info_ext["mem_id"] = mem_id;
    alloc_info_ext["vaddr"] = (uint64_t) vaddr;
    alloced_mem_info.push_back(alloc_info_ext);
    return 0;
}

int test_dealloc(const Json::Value &dealloc_info) {
    int status = -1;
    int alloc_id = get_int(dealloc_info, "id");

    for(auto &alloc_item:alloced_mem_info){
        if (alloc_item["id"].asInt() == alloc_id) {
            int mem_id = get_int(alloc_item, "mem_id");
            status = pal_cshm_dealloc(mem_id);
            break;
        } else {
            ALOGE("%s: Failed to get with id %d %d", LOG_TAG, alloc_item["id"].asInt(), alloc_id);
        }
    }

    return status;
}

int test_set_cfg(const Json::Value &set_cfg_json) {
    int status = -1;
    int alloc_id = get_int(set_cfg_json, "id");
    uint32_t mem_id;
    void *vaddr = nullptr;
    char param_key[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH] = "sendMsg";
    shared_memory_ipc_protocol_t *header = nullptr;
    uint8_t *set_cfg_ptr = nullptr;
    std::vector<uint32_t> miids;
    pal_cshm_msg_payload_t payload = {};
    int set_data_size = get_int(set_cfg_json, "size");
    Json::Value alloc_info_json = Json::nullValue;

    if (set_data_size <= 0) {
        ALOGE("%s: %s: Set data is invalid", LOG_TAG, __func__);
        return -1;
    }

    if (set_data_size > MAX_SET_CFG_SIZE) {
        ALOGI("%s: %s: Set data size is more than max size, using first %d bytes only", LOG_TAG, __func__, MAX_SET_CFG_SIZE);
        set_data_size = MAX_SET_CFG_SIZE;
    }

    for(auto &alloc_item:alloced_mem_info){
        if (alloc_item["id"].asInt() == alloc_id) {
            alloc_info_json = alloc_item;
        }
    }
    vaddr = (void *)get_int64(alloc_info_json, "vaddr");
    mem_id = get_int(alloc_info_json, "mem_id");

    if (vaddr == nullptr) {
        ALOGE("%s: %s: memory alloc failed", LOG_TAG, __func__);
        return -1;
    }

    status = get_miid_with_tag(stream_handle_global, SHMEM_MODULE_TAG, miids);
    if (status) {
        ALOGE("%s: %s: Failed to get miid", LOG_TAG, __func__);
        return status;
    }

    set_cfg_ptr = form_set_get_config_payload(vaddr, {std::make_tuple(SET_CFG,set_data_size)});
    for(int i = 0; i < set_data_size; i++) {
        set_cfg_ptr[i] = i;
    }
    payload.mem_id = mem_id;
    payload.offset = 0x0;
    payload.length = set_data_size;
    payload.miid = miids[0];
    payload.flags = 0;
    status = pal_stream_set_custom_param(stream_handle_global, param_key, &payload, sizeof(pal_cshm_msg_payload_t));

    if (status != 0) {
        ALOGE("%s: %s: MSG call failed!", LOG_TAG, __func__);
        return status;
    }
    return status;

}

int test_get_cfg(const Json::Value &get_cfg_json) {
    int status = -1;
    int alloc_id = -1;
    int offset = 0;
    uint32_t mem_id;
    void *vaddr = nullptr;
    char param_key[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH] = "sendMsg";
    shared_memory_ipc_protocol_t *header = nullptr;
    uint8_t *get_cfg_ptr = nullptr;
    std::vector<uint32_t> miids;
    pal_cshm_msg_payload_t payload = {};
    int get_data_size = 0;
    Json::Value alloc_info_json = Json::nullValue;


    if (isValid(get_cfg_json, "size"))
            get_data_size = get_cfg_json["size"].asInt();

    if (get_data_size <= 0) {
        ALOGE("%s: %s: Set data is invalid", LOG_TAG, __func__);
        return -1;
    }

    if (get_data_size > MAX_SET_CFG_SIZE) {
        ALOGI("%s: %s: Set data size is more than max size, using first %d bytes only", LOG_TAG, __func__, MAX_SET_CFG_SIZE);
        get_data_size = MAX_SET_CFG_SIZE;
    }

    for(auto &alloc_item:alloced_mem_info){
        if (alloc_item["id"].asInt() == alloc_id) {
            alloc_info_json = alloc_item;
        }
    }

    if (!isValid(alloc_info_json)) {
        ALOGE("%s: Failed to find alloced memory with id: %d", __func__, alloc_id);
        return -1;
    }

    if (isValid(alloc_info_json, "vaddr"))
        vaddr = (void *)(alloc_info_json["vaddr"].asInt64());

    if(isValid(alloc_info_json, "mem_id"))
        mem_id = alloc_info_json["mem_id"].asInt();

    if (vaddr == nullptr) {
        ALOGE("%s: %s: memory alloc failed", LOG_TAG, __func__);
        return -1;
    }

    status = get_miid_with_tag(stream_handle_global, SHMEM_MODULE_TAG, miids);
    if (status) {
        ALOGE("%s: %s: Failed to get miid", LOG_TAG, __func__);
        return status;
    }

    get_cfg_ptr = form_set_get_config_payload(vaddr, {std::make_tuple(GET_CFG,get_data_size)});

    payload.mem_id = mem_id;
    payload.offset = offset;
    payload.length = get_data_size;
    payload.miid = miids[0];
    status = pal_stream_set_custom_param(stream_handle_global, param_key, &payload, sizeof(pal_cshm_msg_payload_t));
    if (status != 0) {
        ALOGE("%s: %s: MSG call failed!", LOG_TAG, __func__);
        return status;
    }

    for(int i = 0; i < get_data_size; i++) {
        ALOGE("%s: Get config byte %d value %d", LOG_TAG, i ,get_cfg_ptr[i]);
    }

    return status;

}

int test_data_logging(const Json::Value &dl_json) {
    int status = -1;
    int alloc_id = -1;
    int is_async =  0;
    int read_offset = 0;
    int offset = 0x0;
    int length = -1;
    int flags = 0x0;
    int duration = DEFAULT_DL_DURATION;
    int num_frames = DEFAULT_FRAME_NUM;
    int frame_size = DEFAULT_FRAME_SIZE;
    uint32_t mem_id = 0;
    char param_key[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH] = "sendMsg";
    void *vaddr = nullptr;
    pal_cshm_msg_payload_t payload = {};
    std::string file_name;
    std::vector<uint32_t> miids;
    Json::Value alloc_info_json = Json::nullValue;

    if (isValid(dl_json,"filename") && isValid(dl_json,"id")) {
        file_name = dl_json["filename"].asString();
        alloc_id = dl_json["id"].asInt();
    }
    else {
        ALOGE("%s: Mandatory info missing", LOG_TAG);
        return -1;
    }

    /* Check if optional info exists and populate */
    if (isValid(dl_json, "duration"))
        duration = dl_json["duration"].asInt();
    if (isValid(dl_json, "is_async"))
        is_async = dl_json["is_async"].asInt();
    if (isValid(dl_json, "num_frames"))
        num_frames = dl_json["num_frames"].asInt();
    if (isValid(dl_json, "frame_size"))
        frame_size = dl_json["frame_size"].asInt();
    if (isValid(dl_json, "offset"))
        offset = dl_json["offset"].asInt();
    if (isValid(dl_json, "flags"))
        flags = dl_json["flags"].asInt();

    /* Find the memory block to use for data logging */
    for(auto &alloc_item:alloced_mem_info){
        if (alloc_item["id"].asInt() == alloc_id) {
            alloc_info_json = alloc_item;
        }
    }

    if (!isValid(alloc_info_json)) {
        ALOGE("%s: Failed to find alloced memory with id: %d", __func__, alloc_id);
        return -1;
    }

    if (isValid(alloc_info_json, "vaddr"))
        vaddr = (void *)(alloc_info_json["vaddr"].asInt64());

    if(isValid(alloc_info_json, "mem_id"))
        mem_id = alloc_info_json["mem_id"].asInt();

    if (isValid(dl_json, "length"))
        length = dl_json["length"].asInt();
    else if (isValid(alloc_info_json, "size"))
        length = alloc_info_json["size"].asInt();

    read_offset = form_data_logging_header((uint8_t *)vaddr, num_frames, frame_size);

    status = get_miid_with_tag(stream_handle_global, SHMEM_MODULE_TAG, miids);
    if (status) {
        ALOGE("%s: %s: Failed to get miid", LOG_TAG, __func__);
        return status;
    }

    payload.mem_id = mem_id;
    payload.offset = offset;
    payload.length = length;
    payload.miid = miids[0];
    payload.flags = flags;
    status = pal_stream_set_custom_param(stream_handle_global, param_key, &payload, sizeof(pal_cshm_msg_payload_t));

    if (status) {
        ALOGE("%s: SendMsg failed!", LOG_TAG);
        return status;
    }

    if (is_async == 0)
        status = read_data_logging((uint8_t *)vaddr + read_offset, file_name, duration);
    else
        data_logging_futures.emplace_back(std::async(std::launch::async,read_data_logging,(uint8_t *)vaddr + read_offset, file_name, duration ));

    if (status == 1) {
        status = -1;
        ALOGE("%s: reading data from module failed ", __func__);
        return -1;
    } else {
        status = 0;
    }

    return status;
}

int do_wait(const Json::Value &wait_json) {
    int wait_in_sec = get_int(wait_json, "duration");
    ALOGI("%s: waiting for %d seconds", LOG_TAG, wait_in_sec);
    sleep(wait_in_sec);
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
    ALOGI("%s: Running CMD: %s", LOG_TAG, command_json["name"].asString().c_str());

    switch (command){
            case CMD_ALLOC:
                status = test_alloc(command_json["payload"]);
                break;
            case CMD_DEALLOC:
                status = test_dealloc(command_json["payload"]);
                break;
            case CMD_SET_CFG:
                status = test_set_cfg(command_json["payload"]);
                break;
            case CMD_GET_CFG:
                status = test_get_cfg(command_json["payload"]);
                break;
            case CMD_DATA_LOGGING:
                status = test_data_logging(command_json["payload"]);
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

    /* Reset the stored alloc info before starting each test case*/
    alloced_mem_info.clear();

    for (Json::ArrayIndex i = 0; i < test_case.size(); i++) {
        status = run_command(test_case[i]);
    }

    /* Wait till all data logging tasks are finished. Fail the test-case even
       if one of those tasks failed.
   */

    for(int i =0; i< data_logging_futures.size(); i++) {
        dl_status = data_logging_futures[i].get();
        ALOGI("%s: Data logging task: %d, status = %d", LOG_TAG, i, dl_status);
        if (dl_status != 0)
            status = dl_status;
    }

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
            status = execute_single_test(*itr);
        }
    }
    return status;
}

Json::Value parse_json(std::string file_name) {
    Json::Reader reader;
    Json::Value root = Json::nullValue;
    ifstream ifs(file_name);
    if (ifs) {
        reader.parse(ifs, root);
    }

    return root;
}

 int main(int argc, char *argv[]) {

    int ret = -1;
    int i = 1;
    int tc_num = -1;
    Json::Value root = Json::nullValue;

    ALOGI("%s: Starting test client...", LOG_TAG);
    while((i<argc) && (i+1<=argc)) {
        if (strcmp(argv[i], "-tc") == 0) {
            tc_num = atoi(argv[i+1]);
        } else if ((strcmp(argv[i], "--help") || strcmp(argv[i], "-h")) == 0) {
            print_help();
        }
        i++;
    }

    /* Register with PAL for SSR callback */
    ret = pal_register_global_callback(ssr_callback, CLIENT_COOKIE);
    if (ret) {
        ALOGE("%s: Failed to register global callback", LOG_TAG);
        return -1;
    }

    /* TODO: Take test file name from command line */
    root = parse_json("/data/cshm_test.json");
    if (root == Json::nullValue) {
        ALOGE("%s: Invalid test file", LOG_TAG);
        return 0;
    }

    ret = find_and_run_test(tc_num, root);
    std::cout<<"Test case " << tc_num << " status " << ret <<std::endl;
    return 0;
 }

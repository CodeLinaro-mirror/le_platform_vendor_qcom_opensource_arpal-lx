/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "pal_server_notify"
#include <cutils/android_filesystem_config.h>
#include <hwbinder/IPCThreadState.h>
#include "inc/pal_server_notify.h"


using vendor::qti::hardware::paleventnotifier::V1_0::IPALEventNotifier;
using android::hardware::hidl_handle;

namespace vendor {
namespace qti {
namespace hardware {
namespace paleventnotifier {
namespace V1_0 {
namespace implementation {

static list_declare(client_list);
static pthread_mutex_t client_list_lock = PTHREAD_MUTEX_INITIALIZER;

PALEventNotifier* PALEventNotifier::sInstance;

int is_device_specified(client_info *client_handle, PalCallbackConfig *config, uint32_t event) {
    for (int i = 0; i < client_handle->deviceID.size(); i++) {
        int k = 0;
        for (int j = 0; j < config->no_of_currentdevices; j++) {
            if ((config->currentdevices[j] == client_handle->deviceID[i]) ||((k < config->no_of_prevdevices)
               && (config->prevdevices[k] == client_handle->deviceID[i]) && (event == PAL_NOTIFY_DEVICESWITCH))) {
                return true;
            }
            k++;
        }
    }
    return false;
}

bool is_stream_specified(client_info *client_handle, PalCallbackConfig *config) {
    for (int i = 0; i < client_handle->streamType.size(); i++) {
        if (config->stream_attributes.type == client_handle->streamType[i])
            return true;
    }
    return false;
}

bool is_notification_required(client_info *client_handle, PalCallbackConfig *config, uint32_t event) {
    bool streamSpecified, deviceSpecified;
    bool ret = false;

    streamSpecified = is_stream_specified(client_handle, config);
    deviceSpecified = is_device_specified(client_handle, config, event);
    if (streamSpecified && deviceSpecified) {
        ret = true;
    } else if(client_handle->deviceID.empty() && client_handle->streamType.empty()) {
        ret = true;
    } else if(streamSpecified  && client_handle->deviceID.empty()) {
            ret = true;
    } else if(deviceSpecified  && client_handle->streamType.empty()) {
            ret = true;
    }
    return ret;
}

static int32_t handle_pal_cb(pal_callback_config_t *config, uint32_t event, bool isregister) {
    PalCallbackConfig palCallbackConfig;
    client_info *client_handle = NULL;
    struct listnode* node = NULL;
    uint16_t in_channels = 0;
    uint16_t out_channels = 0;
    struct pal_stream_info info;

    ALOGI("handle_pal_cb is called with event %d", event);
    if(config) {
        info = config->streamAttributes.info.opt_stream_info;
        in_channels = config->streamAttributes.in_media_config.ch_info.channels;
        out_channels = config->streamAttributes.out_media_config.ch_info.channels;
        palCallbackConfig.currentdevices.resize(config->noOfCurrentDevices);
        for(int i = 0; i < config->noOfCurrentDevices; i++) {
            palCallbackConfig.currentdevices[i] = (PalDeviceId)config->currentDevices[i];
            ALOGD("handle_pal_cb current device %d", palCallbackConfig.currentdevices[i]);
        }
        if(event == PAL_NOTIFY_DEVICESWITCH) {
            palCallbackConfig.prevdevices.resize(config->noOfPrevDevices);
            for(int i = 0; i < config->noOfPrevDevices; i++) {
                palCallbackConfig.prevdevices[i] = (PalDeviceId)config->prevDevices[i];
                ALOGD("handle_pal_cb previous device %d", palCallbackConfig.prevdevices[i]);
            }
        }
        palCallbackConfig.no_of_currentdevices = config->noOfCurrentDevices;
        palCallbackConfig.no_of_prevdevices = config->noOfPrevDevices;
        palCallbackConfig.stream_attributes.type = (PalStreamType)config->streamAttributes.type;
        palCallbackConfig.stream_attributes.info.version = info.version;
        palCallbackConfig.stream_attributes.info.size = info.size;
        palCallbackConfig.stream_attributes.info.duration_us = info.duration_us;
        //palCallbackConfig.stream_attributes.info.rx_proxy_type = info.rx_proxy_type;
        palCallbackConfig.stream_attributes.info.tx_proxy_type = info.tx_proxy_type;
        palCallbackConfig.stream_attributes.info.has_video = info.has_video;
        palCallbackConfig.stream_attributes.info.is_streaming = info.is_streaming;
        palCallbackConfig.stream_attributes.info.loopback_type = info.loopback_type;
        palCallbackConfig.stream_attributes.info.haptics_type = info.haptics_type;
        palCallbackConfig.stream_attributes.flags = (PalStreamFlag)config->streamAttributes.flags;
        palCallbackConfig.stream_attributes.direction = (PalStreamDirection)config->streamAttributes.direction;
        palCallbackConfig.stream_attributes.in_media_config.sample_rate = config->streamAttributes.in_media_config.sample_rate;
        palCallbackConfig.stream_attributes.in_media_config.bit_width = config->streamAttributes.in_media_config.bit_width;
        palCallbackConfig.stream_attributes.out_media_config.sample_rate = config->streamAttributes.out_media_config.sample_rate;
        palCallbackConfig.stream_attributes.out_media_config.bit_width = config->streamAttributes.out_media_config.bit_width;
        palCallbackConfig.stream_handle = config->streamHandle;
        if(in_channels) {
            palCallbackConfig.stream_attributes.in_media_config.ch_info.channels = config->streamAttributes.in_media_config.ch_info.channels;
            memset(&palCallbackConfig.stream_attributes.in_media_config.ch_info.ch_map, 0, sizeof(uint8_t[PAL_MAX_CHANNELS_SUPPORTED]));
            memcpy(&palCallbackConfig.stream_attributes.in_media_config.ch_info.ch_map, &config->streamAttributes.in_media_config.ch_info.ch_map, in_channels);
        }
        palCallbackConfig.stream_attributes.in_media_config.aud_fmt_id = (PalAudioFmt)config->streamAttributes.in_media_config.aud_fmt_id;
        if(out_channels) {
            palCallbackConfig.stream_attributes.out_media_config.ch_info.channels = config->streamAttributes.out_media_config.ch_info.channels;
            memset(&palCallbackConfig.stream_attributes.out_media_config.ch_info.ch_map, 0, sizeof(uint8_t[PAL_MAX_CHANNELS_SUPPORTED]));
            memcpy(&palCallbackConfig.stream_attributes.out_media_config.ch_info.ch_map, &config->streamAttributes.out_media_config.ch_info.ch_map, out_channels);
        }
        palCallbackConfig.stream_attributes.out_media_config.aud_fmt_id = (PalAudioFmt)config->streamAttributes.out_media_config.aud_fmt_id;
    }

    list_for_each(node, &client_list) {
        client_handle = node_to_item(node, client_info, list);
        if(is_notification_required(client_handle, &palCallbackConfig, event) == true) {
            if(isregister) {
                if(client_handle->isnotified == false)
                    client_handle->pal_clbk->onStart(palCallbackConfig);
            }
            else {
                if(event == PAL_NOTIFY_START) {
                    ALOGI("PAL_NOTIFY_START notification for client %d", client_handle->pid);
                    client_handle->pal_clbk->onStart(palCallbackConfig);
                }
                else if (event == PAL_NOTIFY_STOP) {
                    ALOGI("PAL_NOTIFY_STOP notification for client %d", client_handle->pid);
                    client_handle->pal_clbk->onStop(palCallbackConfig);
                }
                else if (event == PAL_NOTIFY_DEVICESWITCH) {
                    ALOGI("PAL_NOTIFY_DEVICESWITCH notification for client %d", client_handle->pid);
                    client_handle->pal_clbk->onDeviceSwitch(palCallbackConfig);
                }
            }
        }
    else {
            ALOGV("Notification not needed!!!!");
    }
    }
    return 0;
}

void removeClient(int pid) {
    client_info *client_handle = NULL;
    struct listnode* node = NULL;

    pthread_mutex_lock(&client_list_lock);
    list_for_each(node, &client_list) {
        client_handle = node_to_item(node, client_info, list);
        if (client_handle->pid == pid) {
            list_remove(node);
            free(client_handle);
            ALOGI("removed client with pid: %d", client_handle->pid);
            break;
        }
    }
    pthread_mutex_unlock(&client_list_lock);
}

void NotifierClientDeathRecipient::serviceDied(uint64_t cookie,
         const android::wp<::android::hidl::base::V1_0::IBase>& who) {
    removeClient(cookie);
    ALOGI("%s : PAL client died, PID: %lu",__func__,cookie);
}

Return<int32_t> PALEventNotifier::ipc_pal_notify_register_callback(const sp<IPALEventNotifierCallback>& cb, const hidl_vec<PalDeviceId>& devID, const hidl_vec<PalStreamType>& streamType) {
    int pid = ::android::hardware::IPCThreadState::self()->getCallingPid();
    client_info *client_handle = NULL;
    struct listnode* node = NULL;
    int client_unregistered = -EINVAL;
    int ret = 0;

    if (cb == NULL) {
        pthread_mutex_lock(&client_list_lock);
        list_for_each(node, &client_list) {
            client_handle = node_to_item(node, client_info, list);
            if (client_handle->pid == pid) {
                list_remove(node);
                free(client_handle);
                client_unregistered = 0;
            }
        }
        pthread_mutex_unlock(&client_list_lock);
        return client_unregistered;
    }
    pthread_mutex_lock(&client_list_lock);
    list_for_each(node, &client_list) {
        client_handle = node_to_item(node, client_info, list);
        if (client_handle->pid == pid)
            goto registered_cb;
    }
    client_handle = (client_info *)calloc(1, sizeof(client_info));
    if (client_handle == NULL) {
        ALOGE("%s: Cannot allocate memory for client handle\n", __func__);
        pthread_mutex_unlock(&client_list_lock);
        return -ENOMEM;
    }
    client_handle->pid = pid;
    if (cb != NULL) {
        if (this->mNotifierDeathRecipient.get() == nullptr) {
            this->mNotifierDeathRecipient = new NotifierClientDeathRecipient(this);
        }
        cb->linkToDeath(this->mNotifierDeathRecipient, pid);
    }
    if (devID.size() != 0) {
        client_handle->deviceID.resize(devID.size());
        for(int i = 0; i < devID.size(); i++) {
            client_handle->deviceID[i] = static_cast<PalDeviceId>(devID[i]);
        }
    }
    if (streamType.size() != 0) {
        client_handle->streamType.resize(streamType.size());
        for(int i = 0; i < streamType.size(); i++) {
            client_handle->streamType[i] = static_cast<PalStreamType>(streamType[i]);
        }
    }
    list_add_tail(&client_list, &client_handle->list);
registered_cb:
    if (!client_handle->pal_clbk) {
        client_handle->pal_clbk = cb;
    }
    pthread_mutex_unlock(&client_list_lock);
    ret = pal_register_for_events(handle_pal_cb);
    client_handle->isnotified = true;
    return 0;
}

IPALEventNotifier* HIDL_FETCH_IPALEventNotifier(const char* /* name */) {
    ALOGV("%s");
    return new PALEventNotifier();
}
}
}
}
}
}
}

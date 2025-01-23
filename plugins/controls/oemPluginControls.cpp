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
 * Copyright (c) 2022,2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: lib_oem_plugin_controls"
#define PADDING_8BYTE_ALIGN(x)  ((((x) + 7) & 7) ^ 7)
#include <log/log.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <agm/agm_api.h>
#include <sstream>
#include <system/audio.h>

#include "PluginControlIntf.h"
#include "SessionAlsaUtils.h"
#include "apm_api.h"
#include "vcpm_api.h"
#include "ResourceManager.h"
#include "SessionAlsaVoice.h"

#define NUM_OF_CAL_KEYS 3
#define MAX_VOL_INDEX 5
#define MIN_VOL_INDEX 0
#define percent_to_index(val, min, max) \
            ((val) * ((max) - (min)) * 0.01 + (min) + .5)

#define RXDIR 0
#define TXDIR 1
#define HFP_VOLUME_DB_LINEAR_STEP (-3.0f)

/**
* Following are the default latencies
* Customers can choose to updates these as per their requirements
**/
#define LOW_LATENCY_DELAY (13*1000LL)
#define DEEP_BUFFER_DELAY (29*1000LL)
#define PCM_OFFLOAD_DELAY (30*1000LL)
#define MMAP_DELAY        (3*1000LL)
#define ULL_DELAY         (4*1000LL)

/**
* Following are the default supported BUS addresses
* Following list should be extended by customers if more
  BUS devices are added.
**/
#define BUS_MEDIA            "BUS00_MEDIA"
#ifdef RBVM
#define BUS_MEDIA            "BUS00_WS"
#endif
#define BUS_SYS_NOTIFICATION "BUS01_SYS_NOTIFICATION"
#define BUS_NAV_GUIDANCE     "BUS02_NAV_GUIDANCE"
#define BUS_PHONE            "BUS03_PHONE"
#define BUS_ALERTS           "BUS05_ALERTS"
#define BUS_FRONT_PASSENGER  "BUS08_FRONT_PASSENGER"
#define BUS_REAR_SEAT        "BUS16_REAR_SEAT"
#define BUS_NAVIGATION2      "BUS0F_NAV_GUIDANCE2"
#define BUS_VWARN            "BUS01_no_ASIL"
#define BUS_ROAD_ADAS        "BUS02_Road_ADAS"
#define BUS_SWARN            "BUS03_ASIL"
#define BUS_HFP_DL_RX        "BUSnn_HFP_DL_RX"
/**
* Following are the default defined period sizes
* Customers can add/modify the period sizes as per their requirements
**/
#define DEFAULT_OUTPUT_SAMPLING_RATE    48000
#define MMAP_PERIOD_SIZE (DEFAULT_OUTPUT_SAMPLING_RATE/1000)
#define ULL_PERIOD_SIZE (DEFAULT_OUTPUT_SAMPLING_RATE / 1000) /** 1ms; frames */
#define LOW_LATENCY_PLAYBACK_PERIOD_SIZE 240 /** 5ms; frames */
#define ULL_PERIOD_MULTIPLIER 3
#define BUF_SIZE_PLAYBACK 1024
#define COMPRESS_VOIP_IO_BUF_SIZE_FB 1920
#define COMPRESS_VOIP_IO_BUF_SIZE_SWB 1280
#define COMPRESS_VOIP_IO_BUF_SIZE_WB 640
#define COMPRESS_VOIP_IO_BUF_SIZE_NB 320
#define COMPRESS_OFFLOAD_FRAGMENT_SIZE (32 * 1024)
#define FLAC_COMPRESS_OFFLOAD_FRAGMENT_SIZE (256 * 1024)
#define PCM_OFFLOAD_OUTPUT_PERIOD_DURATION 80
#define DEEP_BUFFER_OUTPUT_PERIOD_DURATION 40
#define MIN_PCM_FRAGMENT_SIZE 512
#define MAX_PCM_FRAGMENT_SIZE (240 * 1024)
#define MAX_BUS_ADDRESS 256

/**
* Following are the default defined period counts
* Customers can add/modify the period counts as per their requirements
**/
#define MMAP_PERIOD_COUNT_MAX 512
#define MMAP_PERIOD_COUNT_DEFAULT (MMAP_PERIOD_COUNT_MAX)
#define ULL_PERIOD_COUNT_DEFAULT 512
#define LOW_LATENCY_PLAYBACK_PERIOD_COUNT 2
#define PCM_OFFLOAD_PLAYBACK_PERIOD_COUNT 2 /** Direct PCM */
#define DEEP_BUFFER_PLAYBACK_PERIOD_COUNT 2 /** Deep Buffer*/
#define NO_OF_BUF 4

#define DIV_ROUND_UP(x, y) (((x) + (y) - 1)/(y))
#define ALIGN(x, y) ((y) * DIV_ROUND_UP((x), (y)))

#define MIN_VOLUME -9000
#define MAX_VOLUME 0

#define AWX_VOLUME 0x11112501
#define AWX_VOLUME_RAMP_SHAPE 0x11112504
#define AWX_VOLUME_RAMP_UP_TIME 0x11112502
#define AWX_VOLUME_RAMP_DOWN_TIME 0x11112503
#define TAG_MODULE_CUSTOM_AWX 0XC0000057
typedef struct pal_awx_volume_data
{
    uint16_t volume_func;
    uint16_t reserved;
    int32_t value[16];
}pal_awx_volume_data_t;



typedef enum pal_awx_bus_type
{
    BUS_MEDIA_WS_AWX,
    BUS_SYS_NOTIFICATION_AWX,
    BUS_NAVIGATION_AWX,
    BUS_PHONE_AWX,
    BUS_NAVIGATION2_AWX,
    BUS_VWARN_AWX,
    BUS_VROADADAS_AWX,
    BUS_RESERVED =0xF
}pal_awx_bus_type_t;

/*
* Bit field for setting all FVM BUSES & RBVM BUSES
* bit0: media/welcome bus bit1: Sys bus bit2: Nav bus bit3: Phone bus bit4: Nav2 bus
* bit5: V-warn bus bit6: V-RoadADAS bus bit7~15: Reserved
*/
#define AWX_FVM_BUS (uint16_t)0x0F

#define RAMP_SHAPE_LINEAR 1
#define RAMP_SHAPE_EXP 2

typedef struct pal_ramp_data
{
    pal_awx_ramp_t ramp;
    uint32_t rampshape;
    uint32_t rampuptime;
    uint32_t rampdowntime;;
} pal_awx_volume_ramp_data_t;

extern "C" {

int setAWXVolumeRAMP(Stream* s, pal_awx_ramp_t ramp, std::shared_ptr<ResourceManager> rm);

const char bus_address[BUS_RESERVED][MAX_BUS_ADDRESS]={
        BUS_MEDIA,
        BUS_SYS_NOTIFICATION,
        BUS_NAV_GUIDANCE,
        BUS_PHONE,
        BUS_NAVIGATION2,
        BUS_VWARN,
        BUS_ROAD_ADAS
};

pal_awx_volume_ramp_data_t ramp_info[PAL_AWX_VOL_RAMP_INVALID]=
{
    { PAL_AWX_ID_RAMP_VERY_FAST,RAMP_SHAPE_LINEAR,6,6},
    { PAL_AWX_ID_RAMP_FAST,RAMP_SHAPE_EXP,20,20},
    { PAL_AWX_ID_RAMP_MED,RAMP_SHAPE_EXP,90,90},
    { PAL_AWX_VOL_RAMP_SLOW,RAMP_SHAPE_EXP,250,250}
};
int payloadCalKeys(Session* s, uint8_t **payload, size_t *size, float volume)
{
    int status = 0;
    apm_module_param_data_t* header;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t *vol_pl;
    vcpm_param_cal_keys_payload_t cal_keys;
    vcpm_ckv_pair_t cal_key_pair[NUM_OF_CAL_KEYS];
    int vol;
    ckv_data_t *data = nullptr;

    PAL_DBG(LOG_TAG,"volume sent:%f", volume);

    payloadSize = sizeof(apm_module_param_data_t) +
                  sizeof(vcpm_param_cal_keys_payload_t) +
                  sizeof(vcpm_ckv_pair_t)*NUM_OF_CAL_KEYS;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        status = -EINVAL;
        goto exit;
    }
     data = (ckv_data_t *)calloc(1, sizeof(ckv_data_t));
    if (!data) {
            PAL_ERR(LOG_TAG, "calloc failed for data");
            status = -EINVAL;
            goto exit;
    }

    /*get CKV data*/
    status = s->getParameters(NULL,VOICE_CKV_DATA,0,(void**)&data);
    if (status) {
         PAL_ERR(LOG_TAG, "failed to get voice ckv data");
         goto exit;
    }

    header = (apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_CAL_KEYS;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    cal_keys.vsid = data->vsid;
    cal_keys.num_ckv_pairs = NUM_OF_CAL_KEYS;

    vol = volume;


    /*volume key*/
    cal_key_pair[0].cal_key_id = VCPM_CAL_KEY_ID_VOLUME_LEVEL;
    cal_key_pair[0].value = percent_to_index(vol, MIN_VOL_INDEX, MAX_VOL_INDEX);

    /*cal key for volume boost*/
    cal_key_pair[1].cal_key_id = VCPM_CAL_KEY_ID_VOL_BOOST;
    cal_key_pair[1].value = data->volume_boost;

     /*cal key for BWE/HD_VOICE*/
    cal_key_pair[2].cal_key_id = VCPM_CAL_KEY_ID_BWE;
    cal_key_pair[2].value = data->hd_voice;

    vol_pl = (uint8_t*)payloadInfo + sizeof(apm_module_param_data_t);
    memcpy(vol_pl, &cal_keys, sizeof(vcpm_param_cal_keys_payload_t));

    vol_pl += sizeof(vcpm_param_cal_keys_payload_t);
    memcpy(vol_pl, &cal_key_pair, sizeof(vcpm_ckv_pair_t)*NUM_OF_CAL_KEYS);


    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "Volume level: %lf, volume boost: %d, HD voice: %d",
            percent_to_index(vol, MIN_VOL_INDEX, MAX_VOL_INDEX),
            cal_key_pair[1].value, cal_key_pair[2].value);

exit:
    if (data) {
        free(data);
    }
    return status;
}

char* getMixerVoiceStream(Stream *s, int dir)
{
    char *stream = (char*)"VOICEMMODE1p";
    struct pal_stream_attributes sAttr;

    memset(&sAttr, 0, sizeof(sAttr));

    s->getStreamAttributes(&sAttr);
    if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
        sAttr.info.voice_call_info.VSID == VOICELBMMODE1) {
        if (dir == TXDIR) {
            stream = (char*)"VOICEMMODE1c";
        } else {
            stream = (char*)"VOICEMMODE1p";
        }
    } else {
        if (dir == TXDIR) {
            stream = (char*)"VOICEMMODE2c";
        } else {
            stream = (char*)"VOICEMMODE2p";
        }
    }

    return stream;
}

int setVoiceMixerParameter(Stream * s, struct mixer *mixer,
                           void *payload, int size, int dir)
{
    char *control = (char*)"setParam";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;
    struct pal_stream_attributes sAttr;
    char *stream = getMixerVoiceStream(s, dir);

    ret = s->getStreamAttributes(&sAttr);

    if (ret) {
         PAL_ERR(LOG_TAG, "could not get stream attributes\n");
        return ret;
    }

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if (!mixer_str) {
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s %s", stream, control);

    PAL_VERBOSE(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        ret = -ENOENT;
        goto exit;
    }


    ret = mixer_ctl_set_array(ctl, payload, size);

exit:
    PAL_VERBOSE(LOG_TAG, "ret = %d, cnt = %d\n", ret, size);
    free(mixer_str);
    return ret;
}

int setVoiceCal(Stream* s, float volume, std::shared_ptr<ResourceManager> rm)
{
    uint32_t status = 0;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    Session *sess = nullptr;
    struct mixer *mixer;

    status = rm->getHwAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG,"mixer error");
        goto exit;
    }

    if (s) {
        s->getAssociatedSession(&sess);
    } else {
        status = -EINVAL;
        goto exit;
    }

    if (!sess) {
        status = -EINVAL;
        goto exit;
    }
    status = payloadCalKeys(sess, &paramData, &paramSize, volume);
    if (!paramData) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
        goto exit;
    }
    status = setVoiceMixerParameter(s, mixer, paramData, paramSize,
                                        RXDIR);
exit:
    if (paramData) {
        free(paramData);
    }
    return status;
}


bool checkIfAWXBus(char* address,pal_awx_bus_type_t* bus_index)
{
    bool bus_match = false ;

    PAL_INFO(LOG_TAG,"check if AWX bux");
    if ((NULL != address) && (NULL!= bus_index))
    {
        // Assing the BUS To MAX address
        *bus_index = BUS_RESERVED;
        // Check if the bus address is a valid AWX BUS_ADDRESS
        if(strncmp(address,BUS_HFP_DL_RX, strlen(BUS_HFP_DL_RX) + 1) == 0)
        {
            bus_match = true;
            // Routing HFP usecase volume to Bit 3 Phone Bus
            *bus_index = BUS_PHONE_AWX;
            PAL_INFO(LOG_TAG,"%s address %s index %d\n", __func__,address,*bus_index);
        }
        else {
            for (int i=0; i<BUS_RESERVED ; i++)
            {
                PAL_INFO(LOG_TAG,"%s address %s index %d\n", __func__,address,i);
                if (strncmp((char *)bus_address[i], (char *)address,strlen(address)) == 0 && strlen(address) != 0)
                {
                    bus_match = true;
                    // Assing the index value and send it
                    *bus_index =  (pal_awx_bus_type_t)(i);
                    PAL_INFO(LOG_TAG,"%s address %s index %d\n", __func__,address,*bus_index);
                    break;
                }
            }
        }
    }

    return bus_match;
}

pal_awx_volume_ramp_data_t* get_Ramp_data(pal_awx_ramp_t ramp)
{
    for (int i=0;i<4;i++)
    {
        if (ramp_info[i].ramp == ramp)
        {
            return &ramp_info[i];
        }
    }
    return NULL;

}

int setAWXVolumRampAttributes(Stream* s,pal_awx_volume_ramp_data_t* rampAttr, std::shared_ptr<ResourceManager> rm)
{
   std::string backendName;
    mixer_ctl *ctl = NULL;
    struct mixer *mixer;
    Session *sess = nullptr;
    int device = 0 ;
    int status = 0;
    uint32_t miid = 0;
    uint16_t* pcmChannel = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0, size;
    uint32_t effect_tag = TAG_MODULE_CUSTOM_AWX;
    struct apm_module_param_data_t* header = NULL;
    pal_awx_volume_data_t* vol_Data =NULL;
    char const *control = "setParam";
    std::ostringstream calCntrlName_awx;
    const char *stream = "PCM";
    std::string backend_name;

    PAL_INFO(LOG_TAG,"Enter. setAWXVolumRampAttributes");

    // Get Backend Name
    status = rm->getBackendName(PAL_DEVICE_OUT_SPEAKER,backend_name);
    if (status) {
        PAL_ERR(LOG_TAG,"Error in get Backend name ");
        goto exit;
    }


    PAL_INFO(LOG_TAG,"backend Name %s ",backend_name.c_str());
    // Get the Mixer
    status = rm->getVirtualAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG,"mixer error");
        goto exit;
    }

    // Get the Associated Session
    status = s->getAssociatedSession(&sess);
    if (status || !sess) {
        PAL_ERR(LOG_TAG,"failed to get session");
        goto exit;
    }

    // Get the Associated Device
    status = sess->getPCMDeviceID(s, &device);
    if (status) {
        PAL_ERR(LOG_TAG,"failed to get device id");
        goto exit;
    }

    // Get the tag
    status = SessionAlsaUtils::getModuleInstanceId(mixer, device,backend_name.c_str(),effect_tag, &miid);

    if (status) {
            PAL_ERR(LOG_TAG,"%s Get MIID from tag data failed\n", __func__);
            goto exit;
    }

    PAL_INFO(LOG_TAG,"%s Get MIID from tag data  Module ID %d\n", __func__,miid);

    payloadSize = sizeof(struct apm_module_param_data_t) +
                sizeof(pal_awx_volume_data_t);

    padBytes = PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        status=-ENOMEM;
        goto exit;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    vol_Data = (pal_awx_volume_data_t*)(payloadInfo +
                sizeof(struct apm_module_param_data_t));


    // Set_ramp_Shape
    header->module_instance_id = miid;
    header->param_id = AWX_VOLUME_RAMP_SHAPE;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    // Fill the data as per the AWX payload.
    vol_Data->volume_func = AWX_FVM_BUS;

    // Pass the Ramp Shape
    vol_Data->value[0] = rampAttr->rampshape;

    size = payloadSize + padBytes;

    PAL_INFO(LOG_TAG,"%s Module ID %d, Param ID  %x param size %d  Volume Func %d Volume value %d\n", __func__,header->module_instance_id,header->param_id,header->param_size,vol_Data->volume_func,vol_Data->value[0]);

    calCntrlName_awx<<stream<<device<<" "<<control;

    ctl = mixer_get_ctl_by_name(mixer, calCntrlName_awx.str().data());

    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", calCntrlName_awx.str().data());
        status = -ENOENT;
        goto exit;
    }
    status = mixer_ctl_set_array(ctl, payloadInfo, size);

    if (status) {
            PAL_ERR(LOG_TAG,"%s mixer_ctl_set_array data failed %d status\n", __func__,status);
            goto exit;
    }

    // Pass the Ramp UP Time
    // Set_ramp_Shape
    header->module_instance_id = miid;
    header->param_id = AWX_VOLUME_RAMP_UP_TIME;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    // Fill the data as per the AWX payload.
    vol_Data->volume_func = AWX_FVM_BUS ;
    vol_Data->value[0] = rampAttr->rampuptime;

    size = payloadSize + padBytes;

    PAL_INFO(LOG_TAG,"%s Module ID %d, Param ID  %x param size %d  Volume Func %d Volume value %d\n", __func__,header->module_instance_id,header->param_id,header->param_size,vol_Data->volume_func,vol_Data->value[0]);

    status = mixer_ctl_set_array(ctl, payloadInfo, size);

    if (status) {
            PAL_ERR(LOG_TAG,"%s mixer_ctl_set_array data failed %d status \n", __func__,status);
            goto exit;
    }

     // Pass the Ramp Down  Time
    header->module_instance_id = miid;
    header->param_id = AWX_VOLUME_RAMP_DOWN_TIME;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    // Fill the data as per the AWX payload.
    vol_Data->volume_func = AWX_FVM_BUS;
    vol_Data->value[0] = rampAttr->rampdowntime;

    size = payloadSize + padBytes;

    PAL_INFO(LOG_TAG,"%s Module ID %d, Param ID  %x param size %d  Volume Func %d Volume value %d\n", __func__,header->module_instance_id,header->param_id,header->param_size,vol_Data->volume_func,vol_Data->value[0]);

    status = mixer_ctl_set_array(ctl, payloadInfo, size);

    if (status) {
            PAL_ERR(LOG_TAG,"%s mixer_ctl_set_array data failed\n", __func__);
            goto exit;
    }

exit:
    ctl = NULL;
    if (payloadInfo)
        free(payloadInfo);
    return status;
}
int setAWXVolumeRAMP(Stream* s, pal_awx_ramp_t ramp, std::shared_ptr<ResourceManager> rm)
{
    uint32_t status = 0;
    //enum Key_AWX_BUS_MAPPING bus_index;
    pal_stream_attributes sAttr;
    pal_awx_volume_ramp_data_t *rampAttr=nullptr;
    PAL_DBG(LOG_TAG, "Enter. ramp:%d \n", ramp);

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        status = -EINVAL;
        goto exit;
    }
    if (!s) {
        PAL_ERR(LOG_TAG,"invalid stream handle");
        status = -EINVAL;
        goto exit;
    }

    rampAttr = get_Ramp_data(ramp);
    if(rampAttr == nullptr) {
        PAL_ERR(LOG_TAG,"invalid ramp Attribute");
        goto exit;
    }
    else
    {
        status = setAWXVolumRampAttributes(s,rampAttr,rm);
    }
    if (status != 0) {
        PAL_ERR(LOG_TAG,"RAMP attributes failed");
        status = -EINVAL;
        goto exit;
    }
exit:
    return status;
}

int setAWXVolume(Stream* s, float voldB, std::shared_ptr<ResourceManager> rm,pal_awx_bus_type_t busIndex)
{
    mixer_ctl *ctl = NULL;
    struct mixer *mixer = nullptr;
    Session *sess = nullptr;
    int device = 0 ;
    int status = 0;
    uint32_t miid = 0;
    uint16_t* pcmChannel = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0, size;
    int32_t param_id = AWX_VOLUME;
    uint32_t effect_tag = TAG_MODULE_CUSTOM_AWX;
    struct apm_module_param_data_t* header = NULL;
    pal_awx_volume_data_t* vol_Data =NULL;
    char const *control = "setParam";
    std::ostringstream calCntrlName_awx;
    const char *stream = "PCM";
    pal_stream_attributes sAttr;
    const char *stream_compress = "COMPRESS";
    std::string backend_name;

    PAL_INFO(LOG_TAG,"set_volume_awx %f BUS Index %x",voldB,busIndex);
    if (!s) {
        PAL_ERR(LOG_TAG,"invalid streeam handle");
        status = -EINVAL;
        goto exit;
    }
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        status = -EINVAL;
        goto exit;
    }

    // Get Backend Name
    status = rm->getBackendName(PAL_DEVICE_OUT_SPEAKER,backend_name);

    if (status) {
        PAL_ERR(LOG_TAG,"Error in get Backend name %d",status);
        goto exit;
    }


    PAL_INFO(LOG_TAG,"backend Name %s BUS Index %x",backend_name.c_str(),busIndex);
    // Get the Mixer
    status = rm->getVirtualAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG,"mixer error");
        goto exit;
    }

    // Get the Associated Session
    status = s->getAssociatedSession(&sess);
    if (status || !sess) {
        PAL_ERR(LOG_TAG,"failed to get session");
        goto exit;
    }

    // Get the Associated Device
    status = sess->getPCMDeviceID(s, &device);
    if (status) {
        PAL_ERR(LOG_TAG,"failed to get device id");
        goto exit;
    }

    // Get the tag
    status = SessionAlsaUtils::getModuleInstanceId(mixer, device,backend_name.c_str(),effect_tag, &miid);

    if (status) {
            PAL_ERR(LOG_TAG,"%s Get MIID from tag data failed\n", __func__);
            goto exit;
    }

    PAL_INFO(LOG_TAG,"%s Get MIID from tag data  Module ID %d\n", __func__,miid);

    payloadSize = sizeof(struct apm_module_param_data_t) +
                sizeof(pal_awx_volume_data_t);

    padBytes = PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        status=-ENOMEM;
        goto exit;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    vol_Data = (pal_awx_volume_data_t*)(payloadInfo +
                sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    // Fill the data as per the AWX payload.
    vol_Data->volume_func = ((uint16_t)1 << busIndex);

    // Pass the volume Gain in the respective value field.
    vol_Data->value[0] = voldB;

    // Check volume range
    if(vol_Data->value[0] > MAX_VOLUME)
            vol_Data->value[0] = MAX_VOLUME;
    else if (vol_Data->value[0] < MIN_VOLUME)
            vol_Data->value[0] = MIN_VOLUME;

    size = payloadSize + padBytes;

    PAL_INFO(LOG_TAG,"%s Module ID %d, Param ID  %x param size %d  Volume Func %d Volume value %d\n", __func__,header->module_instance_id,header->param_id,header->param_size,vol_Data->volume_func,vol_Data->value[0]);
    if (sAttr.type == PAL_STREAM_COMPRESSED) {
        calCntrlName_awx<<stream_compress<<device<<" "<<control;
    }
    else {
        calCntrlName_awx<<stream<<device<<" "<<control;
    }

    ctl = mixer_get_ctl_by_name(mixer, calCntrlName_awx.str().data());

    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", calCntrlName_awx.str().data());
        status = -ENOENT;
        goto exit;
    }

    status = mixer_ctl_set_array(ctl, payloadInfo, size);

    if (status) {
            PAL_ERR(LOG_TAG,"%s mixer_ctl_set_array data failed\n", __func__);
            goto exit;
    }

exit:
    ctl = NULL;
    if (payloadInfo)
        free(payloadInfo);

    return status;
}
int setAudioVolume(Stream* s, float voldB, std::shared_ptr<ResourceManager> rm)
{
    uint32_t status = 0;
    std::vector <std::pair<int, int>> ckv;
    const char *stream = "PCM";
    const char *stream_compress = "COMPRESS";
    const char *setCalibrationControl = "setCalibration";
    struct mixer_ctl *ctl = nullptr;
    struct agm_cal_config *calConfig = nullptr;
    std::ostringstream calCntrlName;
    pal_stream_attributes sAttr;
    Session *sess = nullptr;
    struct mixer *mixer;
    int ckv_size = 0;
    int deviceId;
    pal_awx_bus_type_t bus_index;

    if (!s) {
        PAL_ERR(LOG_TAG,"invalid streeam handle");
        status = -EINVAL;
        goto exit;
    }
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        status = -EINVAL;
        goto exit;
    }

    // if AWX BUS Handle the volume appropriately
    if (true == checkIfAWXBus(sAttr.bus_addr ,&bus_index) && sAttr.direction != PAL_AUDIO_INPUT)
    {
        PAL_ERR(LOG_TAG,"AWX Bus index %d",bus_index);
        // if its a valid AWX bus Additional Check should not happen ideally
        if (BUS_RESERVED != bus_index)
        {
            status = setAWXVolume(s,voldB, rm,bus_index);
        }
        else
        {
            goto exit;
        }

    }
    else
    {
        #if 0
            if ((sAttr.info.opt_stream_info.loopback_type ==
                PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) ||
                (sAttr.info.opt_stream_info.loopback_type ==
                PAL_STREAM_LOOPBACK_CAPTURE_ONLY)) {
                PAL_DBG(LOG_TAG, "RX/TX only Loopback don't support volume");
                status = -EINVAL;
                goto exit;
            }
        #endif

            status = rm->getVirtualAudioMixer(&mixer);
            if (status) {
                PAL_ERR(LOG_TAG,"mixer error");
                goto exit;
            }

            status = s->getAssociatedSession(&sess);
            if (status || !sess) {
                PAL_ERR(LOG_TAG,"failed to get session");
                goto exit;
            }

            status = sess->getPCMDeviceID(s, &deviceId);
            if (status) {
                PAL_ERR(LOG_TAG,"failed to get device id");
                goto exit;
            }


            if (voldB > 1.0) {
                // need to rebase voldB level
                // doing a map for volume in audio hal is recommended if volume range is
                // not 0.0 ~ 1.0
                voldB = ((voldB > 15.000000) ? 1.0 : (voldB / 15));
            }

            PAL_INFO(LOG_TAG, "Set stream (%s) volume to %f", streamNameLUT.at(sAttr.type).c_str(), voldB);

            for (int i = LEVEL_15; i >= LEVEL_0; i--) {
                if (voldB <= (float)pow(10.0, i * HFP_VOLUME_DB_LINEAR_STEP / 20)) {
                    ckv.push_back(std::make_pair(VOLUME, i));
                    PAL_INFO(LOG_TAG, "select VOLUME_LEVEL_%d", i);
                    break;
                }
            }

            if (ckv.size() == 0) {
                PAL_ERR(LOG_TAG, "no ckv got with voldB: %f", voldB);
                status = -EINVAL;
                goto exit;
            }

            calConfig = (struct agm_cal_config*)malloc(sizeof(struct agm_cal_config) +
                                                    (ckv.size() * sizeof(agm_key_value)));

            if (!calConfig) {
                status = -EINVAL;
                goto exit;
            }

            status = SessionAlsaUtils::getCalMetadata(ckv, calConfig);
            if (sAttr.type == PAL_STREAM_COMPRESSED) {
                calCntrlName<<stream_compress<<deviceId<<" "<<setCalibrationControl;
            } else {
                calCntrlName<<stream<<deviceId<<" "<<setCalibrationControl;
            }

            ctl = mixer_get_ctl_by_name(mixer, calCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", calCntrlName.str().data());
                status = -ENOENT;
                goto exit;
            }


            ckv_size = ckv.size()*sizeof(struct agm_key_value);

            status = mixer_ctl_set_array(ctl, calConfig, sizeof(struct agm_cal_config) + ckv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG,"failed to set the tag calibration %d", status);
                goto exit;
            }
    }

 exit:
    ctl = NULL;
    if (calConfig)
        free(calConfig);
    ckv.clear();

    return status;
}

int GetUsecase(audio_output_flags_t halStreamFlags)
{
    // need to update other usecases in future
    int usecase = AUDIO_USECASE_PLAYBACK_LOW_LATENCY;

    if (halStreamFlags & AUDIO_OUTPUT_FLAG_VOIP_RX)
        usecase = AUDIO_USECASE_PLAYBACK_VOIP;
    else if ((halStreamFlags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) ||
             (halStreamFlags == AUDIO_OUTPUT_FLAG_DIRECT)) {
        if (halStreamFlags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD)
            usecase = AUDIO_USECASE_PLAYBACK_OFFLOAD;
        else
            usecase = AUDIO_USECASE_PLAYBACK_OFFLOAD2;
    } else if (halStreamFlags & AUDIO_OUTPUT_FLAG_RAW)
        usecase = AUDIO_USECASE_PLAYBACK_ULL;
    else if (halStreamFlags & AUDIO_OUTPUT_FLAG_FAST)
        usecase = AUDIO_USECASE_PLAYBACK_LOW_LATENCY;
    else if (halStreamFlags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER)
        usecase = AUDIO_USECASE_PLAYBACK_DEEP_BUFFER;
    else if (halStreamFlags & AUDIO_OUTPUT_FLAG_MMAP_NOIRQ)
        usecase = AUDIO_USECASE_PLAYBACK_MMAP;

    return usecase;
}

int pcm_buffer_size(pal_stream_attributes sattr)
{
    uint8_t channels = audio_channel_count_from_out_mask(sattr.ch_mask);
    uint8_t bytes_per_sample = audio_bytes_per_sample((audio_format_t)sattr.format);
    uint32_t fragment_size = 0;

    PAL_DBG(LOG_TAG, "config_ format:%x, SR %d ch_mask 0x%x",
            sattr.format, sattr.out_media_config.sample_rate,
            sattr.ch_mask);
    if (sattr.type == PAL_STREAM_PLAYBACK_BUS) {
        fragment_size = DEEP_BUFFER_OUTPUT_PERIOD_DURATION *
            sattr.out_media_config.sample_rate * bytes_per_sample * channels;
    } else {
        fragment_size = PCM_OFFLOAD_OUTPUT_PERIOD_DURATION *
            sattr.out_media_config.sample_rate * bytes_per_sample * channels;
    }
    fragment_size /= 1000;

    if (fragment_size < MIN_PCM_FRAGMENT_SIZE)
        fragment_size = MIN_PCM_FRAGMENT_SIZE;
    else if (fragment_size > MAX_PCM_FRAGMENT_SIZE)
        fragment_size = MAX_PCM_FRAGMENT_SIZE;

    fragment_size = ALIGN(fragment_size, (bytes_per_sample * channels * 32));

    PAL_DBG(LOG_TAG, "fragment size: %d", fragment_size);
    return fragment_size;
}

int compressed_buffer_size(audio_format_t format)
{
    int fragment_size = COMPRESS_OFFLOAD_FRAGMENT_SIZE;
    PAL_DBG(LOG_TAG, "Format %x", format);
    if(format ==  AUDIO_FORMAT_FLAC ) {
        fragment_size = FLAC_COMPRESS_OFFLOAD_FRAGMENT_SIZE;
    } else {
        fragment_size =  COMPRESS_OFFLOAD_FRAGMENT_SIZE;
    }
    return fragment_size;
}

int voip_buffer_size(uint32_t sample_rate)
{
    if (sample_rate == 48000)
        return COMPRESS_VOIP_IO_BUF_SIZE_FB;
    else if (sample_rate == 32000)
        return COMPRESS_VOIP_IO_BUF_SIZE_SWB;
    else if (sample_rate == 16000)
        return COMPRESS_VOIP_IO_BUF_SIZE_WB;
    else
        return COMPRESS_VOIP_IO_BUF_SIZE_NB;
}

uint32_t get_buffer_size(pal_stream_attributes streamAttributes_) {
    if (streamAttributes_.type == PAL_STREAM_VOIP_RX) {
        return voip_buffer_size(streamAttributes_.out_media_config.sample_rate);
    } else if (streamAttributes_.type == PAL_STREAM_COMPRESSED) {
        return compressed_buffer_size((audio_format_t)streamAttributes_.format);
    } else if (streamAttributes_.type == PAL_STREAM_PCM_OFFLOAD
              || streamAttributes_.type == PAL_STREAM_DEEP_BUFFER) {
        return pcm_buffer_size(streamAttributes_);
    } else if (streamAttributes_.type == PAL_STREAM_LOW_LATENCY) {
        return LOW_LATENCY_PLAYBACK_PERIOD_SIZE *
            audio_bytes_per_frame(
                    audio_channel_count_from_out_mask(streamAttributes_.ch_mask),
                    (audio_format_t)streamAttributes_.format);
    } else if (streamAttributes_.type == PAL_STREAM_ULTRA_LOW_LATENCY) {
        return ULL_PERIOD_SIZE * ULL_PERIOD_MULTIPLIER *
            audio_bytes_per_frame(
                    audio_channel_count_from_out_mask(streamAttributes_.ch_mask),
                    (audio_format_t)streamAttributes_.format);
    } else if (streamAttributes_.type == PAL_STREAM_PLAYBACK_BUS) {
        if ((strcmp(streamAttributes_.bus_addr, BUS_MEDIA) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_SYS_NOTIFICATION) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_NAV_GUIDANCE) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_ALERTS) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_FRONT_PASSENGER) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_REAR_SEAT) == 0)) {
            return pcm_buffer_size(streamAttributes_);
        } else if((strcmp(streamAttributes_.bus_addr, BUS_PHONE) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_VWARN) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_ROAD_ADAS) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_SWARN) == 0)) {
            PAL_DBG(LOG_TAG, " setting low latency period size %d", streamAttributes_.type);
            return LOW_LATENCY_PLAYBACK_PERIOD_SIZE *
            audio_bytes_per_frame(
                    audio_channel_count_from_out_mask(streamAttributes_.ch_mask),
                    (audio_format_t)streamAttributes_.format);
        } else {
            return BUF_SIZE_PLAYBACK * NO_OF_BUF;
        }
    } else {
        return BUF_SIZE_PLAYBACK * NO_OF_BUF;
    }
}

void get_buffer_configration(Stream* s, void** payload, size_t* payload_size)
{
    struct pal_buffer_data *buff_config = (struct pal_buffer_data*)*payload;
    struct pal_stream_attributes streamAttributes_;
    int usecase;

    s->getStreamAttributes(&streamAttributes_);
    PAL_DBG(LOG_TAG, " PAL stream type %d", streamAttributes_.type);

    usecase = GetUsecase((audio_output_flags_t)streamAttributes_.hal_flags);

    if (usecase == AUDIO_USECASE_PLAYBACK_MMAP) {
        buff_config->buffer_size = MMAP_PERIOD_SIZE * audio_bytes_per_frame(
                    audio_channel_count_from_out_mask(streamAttributes_.ch_mask),
                    (audio_format_t)streamAttributes_.format);
        buff_config->buffer_count = MMAP_PERIOD_COUNT_DEFAULT;
    } else if (usecase == AUDIO_USECASE_PLAYBACK_ULL) {
        buff_config->buffer_size = ULL_PERIOD_SIZE * audio_bytes_per_frame(
                    audio_channel_count_from_out_mask(streamAttributes_.ch_mask),
                    (audio_format_t)streamAttributes_.format);
        buff_config->buffer_count = ULL_PERIOD_COUNT_DEFAULT;
    } else
        buff_config->buffer_size = get_buffer_size(streamAttributes_);

    if ((usecase == AUDIO_USECASE_PLAYBACK_LOW_LATENCY)
        || (strcmp(streamAttributes_.bus_addr, BUS_VWARN) == 0)
        || (strcmp(streamAttributes_.bus_addr, BUS_ROAD_ADAS) == 0)
        || (strcmp(streamAttributes_.bus_addr, BUS_SWARN) == 0))
            buff_config->buffer_count = LOW_LATENCY_PLAYBACK_PERIOD_COUNT;
    else if (usecase == AUDIO_USECASE_PLAYBACK_OFFLOAD2)
            buff_config->buffer_count = PCM_OFFLOAD_PLAYBACK_PERIOD_COUNT;
    else if (usecase == AUDIO_USECASE_PLAYBACK_DEEP_BUFFER)
            buff_config->buffer_count = DEEP_BUFFER_PLAYBACK_PERIOD_COUNT;

    *payload_size = sizeof(struct pal_buffer_data);
}
void get_render_latency(Stream* s, void **payload)
{
    struct pal_stream_attributes streamAttributes_;
    long long** latency = (long long**)payload;

    s->getStreamAttributes(&streamAttributes_);
    PAL_DBG(LOG_TAG, " PAL stream type %d", streamAttributes_.type);

    switch (streamAttributes_.type) {
         case PAL_STREAM_DEEP_BUFFER:
             **latency = DEEP_BUFFER_DELAY;
             break;
         case PAL_STREAM_LOW_LATENCY:
             **latency = LOW_LATENCY_DELAY;
             break;
         case PAL_STREAM_COMPRESSED:
         case PAL_STREAM_PCM_OFFLOAD:
              **latency = PCM_OFFLOAD_DELAY;
              break;
         case PAL_STREAM_ULTRA_LOW_LATENCY:
              **latency = ULL_DELAY;
              break;
         case PAL_STREAM_PLAYBACK_BUS:
              {
                  if ((strcmp(streamAttributes_.bus_addr, BUS_MEDIA) == 0) ||
                      (strcmp(streamAttributes_.bus_addr, BUS_NAV_GUIDANCE) == 0) ||
                      (strcmp(streamAttributes_.bus_addr, BUS_FRONT_PASSENGER) == 0) ||
                      (strcmp(streamAttributes_.bus_addr, BUS_REAR_SEAT) == 0)) {
                      **latency = DEEP_BUFFER_DELAY;
                  } else if ((strcmp(streamAttributes_.bus_addr, BUS_SYS_NOTIFICATION) == 0) ||
                             (strcmp(streamAttributes_.bus_addr, BUS_PHONE) == 0) ||
                             (strcmp(streamAttributes_.bus_addr, BUS_ALERTS) == 0)) {
                      **latency = LOW_LATENCY_DELAY;
                  }
                  //TODO: Customers should add handling of additional BUS devices here if added.
              }
              break;
         //TODO: Add more usecases/type as in current hal, once they are available in pal
         default:
             **latency = 0;
             break;
     }
}

/*interface function definition*/

__attribute__ ((visibility ("default")))
int plugin_get(Stream* s, plugin_control_name_t control, void **payload,
               size_t *playload_size)
{
    int status = 0;
    ckv_data_t *data = nullptr;
    Session *sess = nullptr;

    PAL_DBG(LOG_TAG,"Enter with control %d", control);


    if (s) {
        s->getAssociatedSession(&sess);
    } else {
        status = -EINVAL;
        goto exit;
    }

    switch (control) {
    case PLUGIN_CONTROL_VOLUME:
        s->getVolumeData((struct pal_volume_data *)*payload, playload_size);
        break;
    case PLUGIN_CONTROL_VOLUME_BOOST:
         data = (ckv_data_t *)calloc(1, sizeof(ckv_data_t));
         if (!data) {
            PAL_ERR(LOG_TAG, "calloc failed for data");
            status = -EINVAL;
            goto exit;
         }
         /*get CKV data*/
         sess->getParameters(NULL,VOICE_CKV_DATA,0,(void**)&data);
         memcpy((bool*)*payload, &(data->volume_boost), sizeof(bool));
         *playload_size = sizeof(bool);
         break;
    case PLUGIN_CONTROL_HD_VOICE:
        data = (ckv_data_t *)calloc(1, sizeof(ckv_data_t));
        if (!data) {
            PAL_ERR(LOG_TAG, "calloc failed for data");
            status = -EINVAL;
            goto exit;
        }
        /*get CKV data*/
        sess->getParameters(NULL,VOICE_CKV_DATA,0,(void**)&data);
        memcpy((bool*)*payload, &(data->hd_voice), sizeof(bool));
        *playload_size = sizeof(bool);
        break;
    case PLUGIN_CONTROL_AUDIO_BUFFER:
        get_buffer_configration(s, payload, playload_size);
        break;
    case PLUGIN_CONTROL_AUDIO_LATENCY:
        get_render_latency(s, payload);
        break;
    case PLUGIN_CONTROL_VOLUME_RAMP:
        s->getVolumeRAMPData((struct pal_awx_ramp_curve_t *)*payload, playload_size);
        break;
    default:
        PAL_ERR(LOG_TAG,"control not supported in this plugin");
        status = -EINVAL;
    }
exit:
    if (data) {
        free(data);
    }
    PAL_DBG(LOG_TAG,"Exit with status: %d", status);
    return status;
}

__attribute__ ((visibility ("default")))
int plugin_set(Stream* s, plugin_control_name_t control, void *payload,
               size_t playload_size)
{
    uint32_t status = 0;
    pal_stream_type_t type;
    std::shared_ptr<ResourceManager> rm;
    struct pal_volume_data *voldata = nullptr;
    float volume = 0.0;
    size_t vol_size = 0;
    pal_awx_ramp_t *ramp = nullptr;

    PAL_DBG(LOG_TAG,"Enter with control %d", control);

    if (!s) {
        status = -EINVAL;
        goto exit;
    }

    rm = ResourceManager::getInstance();
    if (!rm) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "ResourceManager getInstance failed");
        goto exit;
    }

    switch (control) {
    case PLUGIN_CONTROL_VOLUME:
        voldata = (struct pal_volume_data *)payload;
        if (!playload_size) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Recieved invalid playload size");
            goto exit;
        }
        if (!voldata) {
            status = -ENOMEM;
            PAL_ERR(LOG_TAG, "Recieved invalid playload pointer");
            goto exit;
        }

        PAL_INFO(LOG_TAG,"recieved volume: %f", (voldata->volume_pair[0].vol));
        volume = (voldata->volume_pair[0].vol);

        s->getStreamType(&type);

        if (type == PAL_STREAM_VOICE_CALL) {
            status = setVoiceCal(s, volume, rm);
        } else {
            status = setAudioVolume(s, volume, rm);
        }
        voldata = NULL;
        break;
    case PLUGIN_CONTROL_VOLUME_BOOST:
    case PLUGIN_CONTROL_HD_VOICE:
        voldata = (struct pal_volume_data *)malloc(sizeof(uint32_t) +
                                                  (sizeof(struct pal_channel_vol_kv)));
        if (!voldata) {
            status = -ENOMEM;
            PAL_ERR(LOG_TAG, "volume malloc failed %s", strerror(errno));
            goto exit;
        }
        status = plugin_get(s,PLUGIN_CONTROL_VOLUME,(void**)&voldata, &vol_size);
        if (status) {
             PAL_ERR(LOG_TAG,"control not get volume data");
             status = -EINVAL;
             goto exit;
        }
        PAL_DBG(LOG_TAG,"recieved volume: %f", (voldata->volume_pair[0].vol));
        volume = (voldata->volume_pair[0].vol);
        setVoiceCal(s, volume, rm);
        break;
    case PLUGIN_CONTROL_VOLUME_RAMP:
        ramp =(pal_awx_ramp_t*)payload;
        if (!playload_size) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Recieved invalid playload size");
            goto exit;
        }
        if (!ramp) {
            status = -ENOMEM;
            PAL_ERR(LOG_TAG, "Recieved invalid ramp playload pointer");
            goto exit;
        }
        s->getStreamType(&type);
        status = setAWXVolumeRAMP(s, *ramp, rm);
        ramp = NULL;
        break;
    default:
        PAL_ERR(LOG_TAG,"control not supported in this plugin");
        status = -EINVAL;
    }
exit:
    PAL_DBG(LOG_TAG,"Exit with status: %d", status);
    return status;
}
}

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

#define LOG_TAG "PAL: lib_default_plugin_controls"

#include <log/log.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <agm_api.h>
#include <sstream>

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

extern "C" {
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
    if (volume < 0.0) {
            volume = 0.0;
    } else if (volume > 1.0) {
        volume = 1.0;
    }

    vol = lrint(volume * 100.0);

    // Voice volume levels from android are mapped to driver volume levels as follows.
    // 0 -> 5, 20 -> 4, 40 ->3, 60 -> 2, 80 -> 1, 100 -> 0
    // So adjust the volume to get the correct volume index in driver
    vol = 100 - vol;

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

    status = rm->getAudioMixer(&mixer);
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
    if ((sAttr.info.opt_stream_info.loopback_type ==
         PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) ||
        (sAttr.info.opt_stream_info.loopback_type ==
         PAL_STREAM_LOOPBACK_CAPTURE_ONLY)) {
         PAL_DBG(LOG_TAG, "RX/TX only Loopback don't support volume");
         status = -EINVAL;
         goto exit;
    }

    status = rm->getAudioMixer(&mixer);
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

    if (voldB == 0.0f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_15));
    }
    else if (voldB <= 0.002172f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_14));
    }
    else if (voldB <= 0.004660f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_13));
    }
    else if (voldB <= 0.01f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_12));
    }
    else if (voldB <= 0.014877f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_11));
    }
    else if (voldB <= 0.023646f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_10));
    }
    else if (voldB <= 0.037584f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_9));
    }
    else if (voldB <= 0.055912f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_8));
    }
    else if (voldB <= 0.088869f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_7));
    }
    else if (voldB <= 0.141254f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_6));
    }
    else if (voldB <= 0.189453f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_5));
    }
    else if (voldB <= 0.266840f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_4));
    }
    else if (voldB <= 0.375838f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_3));
    }
    else if (voldB <= 0.504081f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_2));
    }
    else if (voldB <= 0.709987f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_1));
    }
    else if (voldB <= 1.0f) {
        ckv.push_back(std::make_pair(VOLUME,LEVEL_0));
    }

    if (ckv.size() == 0) {
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

 exit:
    ctl = NULL;
    if (calConfig)
        free(calConfig);
    ckv.clear();

    return status;
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

    default:
        PAL_ERR(LOG_TAG,"control not supported in this plugin");
        status = -EINVAL;
    }
exit:
    if (voldata) {
        free(voldata);
    }
    PAL_DBG(LOG_TAG,"Exit with status: %d", status);
    return status;
}
}

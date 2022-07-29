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
#include <agm/agm_api.h>
#include <sstream>
#include <system/audio.h>
#include <sys/mman.h>

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
#define BUS_SYS_NOTIFICATION "BUS01_SYS_NOTIFICATION"
#define BUS_NAV_GUIDANCE     "BUS02_NAV_GUIDANCE"
#define BUS_PHONE            "BUS03_PHONE"
#define BUS_FRONT_PASSENGER  "BUS08_FRONT_PASSENGER"
#define BUS_REAR_SEAT        "BUS16_REAR_SEAT"

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
#define MIN_PCM_FRAGMENT_SIZE 512
#define MAX_PCM_FRAGMENT_SIZE (240 * 1024)

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
        voldB = ((voldB > 15.000000) ? 1.0 : (voldB / 15));
        PAL_DBG(LOG_TAG,"Volume brought within range (%f)\n", voldB);
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
    fragment_size = PCM_OFFLOAD_OUTPUT_PERIOD_DURATION *
        sattr.out_media_config.sample_rate * bytes_per_sample * channels;
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
            || (strcmp(streamAttributes_.bus_addr, BUS_FRONT_PASSENGER) == 0)
            || (strcmp(streamAttributes_.bus_addr, BUS_REAR_SEAT) == 0)) {
            return pcm_buffer_size(streamAttributes_);
        } else if(strcmp(streamAttributes_.bus_addr, BUS_PHONE) == 0) {
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

    if (usecase == AUDIO_USECASE_PLAYBACK_LOW_LATENCY)
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
                             (strcmp(streamAttributes_.bus_addr, BUS_PHONE) == 0)) {
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

int shmem_buff_load_parameter(pal_stream_handle_t *stream_handle, pal_shmem_info buff_info)
{
    int status = 0;
    uint8_t *payload = NULL;
    pal_param_payload *pal_payload = NULL;
    effect_pal_payload_t *effect_payload = NULL;
    pal_effect_custom_payload_t *customPayload = NULL;
    pal_load_persist_dummy_t *custom_payload = NULL;
    uint32_t payload_size;

    payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_persist_dummy_t);
    payload = (uint8_t *) calloc (1, payload_size);
    if (!payload) {
        PAL_ERR(LOG_TAG,"%s calloc failed for size %d", __func__, payload_size);
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
    customPayload->paramId = CAPI_V2_PARAM_ID_LOAD_PERSIST_DUMMY;

    custom_payload = (pal_load_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
    custom_payload->size = buff_info.size;
    custom_payload->spf_mem_handle = buff_info.spf_mem_handle;
    custom_payload->spf_mem_addr_lsw = (uint32_t)buff_info.spf_addr;
    custom_payload->spf_mem_addr_msw = (uint32_t)(buff_info.spf_addr >> 32);

    status = pal_stream_set_param(stream_handle, PAL_PARAM_ID_UIEFFECT, pal_payload);
    if (status) {
        PAL_ERR(LOG_TAG, "pal_stream_set_param failed");
        free(payload);
        return status;
    }

    free(payload);
    return 0;
}

int shmem_buff_set_persist_parameter(pal_stream_handle_t *stream_handle, uint8_t *payload)
{
    int status = 0;
    pal_param_payload *pal_payload;
    effect_pal_payload_t *effect_payload = NULL;
    pal_effect_custom_payload_t *customPayload;

    pal_payload = (pal_param_payload *) payload;
    pal_payload->payload_size = sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_set_persist_dummy_t);

    effect_payload = (effect_pal_payload_t *)(payload + sizeof(pal_param_payload));
    effect_payload->isTKV = PARAM_NONTKV;
    effect_payload->tag = TEST_SHARED_MEM_TAG;
    effect_payload->payloadSize = sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_set_persist_dummy_t);

    customPayload = (pal_effect_custom_payload_t *)(payload + sizeof(pal_param_payload) + sizeof(effect_pal_payload_t));
    customPayload->paramId = CAPI_V2_PARAM_ID_LOAD_SET_PERSIST_DUMMY;

    status = pal_stream_set_param(stream_handle, PAL_PARAM_ID_UIEFFECT, pal_payload);
    if (status) {
         PAL_ERR(LOG_TAG, "pal_stream_set_param failed");
         return status;
    }

    return status;
}

int shmem_buff_set_parameter(pal_stream_handle_t *stream_handle, uint8_t *payload)
{
    int status = 0;
    pal_param_payload *pal_payload;
    effect_pal_payload_t *effect_payload = NULL;
    pal_effect_custom_payload_t *customPayload;

    pal_payload = (pal_param_payload *) payload;
    pal_payload->payload_size = sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_get_persist_dummy_t);

    effect_payload = (effect_pal_payload_t *)(payload + sizeof(pal_param_payload));
    effect_payload->isTKV = PARAM_NONTKV;
    effect_payload->tag = TEST_SHARED_MEM_TAG;
    effect_payload->payloadSize = sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_get_persist_dummy_t);

    customPayload = (pal_effect_custom_payload_t *)(payload + sizeof(pal_param_payload) + sizeof(effect_pal_payload_t));
    customPayload->paramId = CAPI_V2_PARAM_ID_LOAD_GET_PERSIST_DUMMY;

    status = pal_stream_set_param(stream_handle, PAL_PARAM_ID_UIEFFECT, pal_payload);
    if (status) {
         PAL_ERR(LOG_TAG, "pal_stream_set_param failed");
         return status;
    }

    return status;
}

int shmem_buff_get_parameter(pal_stream_handle_t *stream_handle, uint8_t *payload, uint32_t size)
{
    int status = 0;
    pal_param_payload *pal_payload;
    effect_pal_payload_t *effect_payload = NULL;
    pal_effect_custom_payload_t *customPayload;

    pal_payload = (pal_param_payload *) payload;
    pal_payload->payload_size = sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + size;

    effect_payload = (effect_pal_payload_t *)(payload + sizeof(pal_param_payload));
    effect_payload->isTKV = PARAM_NONTKV;
    effect_payload->tag = TEST_SHARED_MEM_TAG;
    effect_payload->payloadSize = sizeof(pal_effect_custom_payload_t) + size;

    customPayload = (pal_effect_custom_payload_t *)(payload + sizeof(pal_param_payload) + sizeof(effect_pal_payload_t));
    customPayload->paramId = CAPI_V2_PARAM_ID_LOAD_GET_PERSIST_DUMMY;

    status = pal_stream_get_param(stream_handle, PAL_PARAM_ID_STREAM_SHMEM_GET_PARAM, &pal_payload);
    if (status) {
         PAL_ERR(LOG_TAG, "pal_stream_get_param failed");
         return status;
    }

    return status;
}

/*interface function definition*/

__attribute__ ((visibility ("default")))
int plugin_init(plugin_control_name_t control)
{

    PAL_DBG(LOG_TAG,"Enter with control %d", control);
    struct pal_stream_attributes stream_attr;
    int status;
    int channels = NUMBER_OF_CHANNEL;
    struct pal_shmem_info buff_info;
    struct pal_channel_info ch_info;
    struct pal_device devices[1] = {};
    pal_stream_handle_t *pal_stream_handle;
    Stream *s = NULL;
    Session *sess = nullptr;
    int deviceId;

    switch (control) {

        case PLUGIN_CONTROL_SHMEM_ALLOC:
            ch_info.channels = channels;
            ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
            ch_info.ch_map[1] = PAL_CHMAP_CHANNEL_FR;

            stream_attr.type = PAL_STREAM_LOW_LATENCY;
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

            status = pal_stream_open(&stream_attr,
                    1,
                    devices,
                    0,
                    NULL,
                    NULL,
                    0,
                    &pal_stream_handle);

            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to open pal stream ");
                status = -EINVAL;
            }

            status = pal_stream_start(pal_stream_handle);

            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to start pal stream ");
                status = -EINVAL;
            }

            s = reinterpret_cast<Stream *>(pal_stream_handle);

            status = s->getAssociatedSession(&sess);

            if (status || !sess) {
                PAL_ERR(LOG_TAG,"failed to get session");
            }

            status = sess->getPCMDeviceID(s, &deviceId);

            if (status) {
                PAL_ERR(LOG_TAG,"failed to get device id");
            }

            buff_info.size = 32*1024*1024;
            buff_info.fd = 0;
            buff_info.spf_addr = 0;
            buff_info.spf_mem_handle = 0;
            buff_info.cache = PAL_SHMEM_MAP_UNCACHED;

            status = SessionAlsaUtils::shmem_buff_alloc(&buff_info, deviceId, sizeof(struct pal_shmem_info));
            if (status != 0) {
                PAL_ERR(LOG_TAG, "Shared memory buffer allocation failed");
                status = -EINVAL;
                break;
            }
            PAL_INFO(LOG_TAG, "Recieved values of fd is %d, value of spf handle is %x spf_addr=0x%llx", buff_info.fd, buff_info.spf_mem_handle, buff_info.spf_addr);

            /*
             * Test 1: Client modify uncached shared memory and verify change on spf side using set/get persist
             * Test 2: Client modify uncached shared memory on spf and verify change at HLOS side by mmap of data fd
             * Test 3: Client modify uncached shared memory at HLOS side and verify change on spf side
             */
            {
                uint32_t *p;
                p = (uint32_t *)mmap(NULL, buff_info.size, PROT_READ | PROT_WRITE, MAP_SHARED, buff_info.fd, 0);
                if (p == NULL)
                {
                    PAL_ERR(LOG_TAG,"Error: mmap return NULL");
                    status = -ENOMEM;
                    goto exit;
                }

                {
                    status = shmem_buff_load_parameter(pal_stream_handle, buff_info);
                    if (status) {
                        PAL_ERR(LOG_TAG, "shmem_buff_load_parameter failed");
                        goto exit;
                    }
                    else
                        PAL_INFO(LOG_TAG, "shmem_buff_load_parameter is successful");
                }

                PAL_INFO(LOG_TAG,"%s 1st Test Case set persist get persist", __func__);
                {
                    uint32_t num = 0;
                    uint32_t i = 0;

                    num = 1024;

                    for(i = 0; i < num; i++)
                    {
                        uint8_t *payload = NULL;
                        uint32_t payload_size;
                        pal_load_set_persist_dummy_t *set_shmem = nullptr;
                        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_set_persist_dummy_t);
                        payload = (uint8_t *) calloc (1, payload_size);
                        if (!payload) {
                            PAL_ERR(LOG_TAG,"%s calloc failed for size %d", __func__, payload_size);
                            status = -ENOMEM;
                            goto exit;
                        }
                        set_shmem = (pal_load_set_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
                        set_shmem->effective = 1;
                        set_shmem->index = i;
                        set_shmem->value = i;
                        PAL_INFO(LOG_TAG,"%s effective=%d index=0x%x value[%d]=0x%x", __func__, set_shmem->effective, set_shmem->index, i, set_shmem->value);
                        status = shmem_buff_set_persist_parameter(pal_stream_handle, payload);
                        if (status) {
                            PAL_ERR(LOG_TAG, "shmem_buff_set_persist_parameter failed");
                            free(payload);
                            status = -EINVAL;
                            goto exit;
                        }
                        else
                            PAL_INFO(LOG_TAG, "shmem_buff_set_persist_parameter is successful");

                        free(payload);
                    }

                    {
                        uint8_t *payload = NULL;
                        uint32_t payload_size;
                        pal_load_get_persist_dummy_t *set_shmem1 = nullptr;
                        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_get_persist_dummy_t);
                        payload = (uint8_t *) calloc (1, payload_size);
                        if (!payload) {
                            PAL_ERR(LOG_TAG,"%s calloc failed for size %d", __func__, payload_size);
                            status = -ENOMEM;
                            goto exit;
                        }
                        set_shmem1 = (pal_load_get_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
                        set_shmem1->index = 0;
                        set_shmem1->num = num;
                        status = shmem_buff_set_parameter(pal_stream_handle, payload);
                        if (status) {
                            PAL_ERR(LOG_TAG, "shmem_buff_set_parameter failed");
                            free(payload);
                            status = -EINVAL;
                            goto exit;
                        }
                        else
                            PAL_INFO(LOG_TAG, "shmem_buff_set_parameter is successful");

                        free(payload);
                    }

                    {
                        uint8_t *payload = NULL;
                        uint32_t payload_size;
                        pal_load_get_persist_dummy_t *get_shmem = nullptr;

                        uint32_t gpd_size = sizeof(pal_load_get_persist_dummy_t)+(num*sizeof(uint32_t));

                        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + gpd_size;
                        payload = (uint8_t *) calloc (1, payload_size);
                        if (!payload) {
                            PAL_ERR(LOG_TAG,"%s calloc failed for size %d", __func__, payload_size);
                            status = -ENOMEM;
                            goto exit;
                        }
                        get_shmem = (pal_load_get_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
                        status = shmem_buff_get_parameter(pal_stream_handle, payload, gpd_size);
                        if (status) {
                            PAL_ERR(LOG_TAG, "shmem_buff_get_parameter failed");
                            free(payload);
                            status = -EINVAL;
                            goto exit;
                        }
                        else{
                            for(i = 0; i < get_shmem->num; i++)
                                PAL_INFO(LOG_TAG,"%s index=%d num=0x%x array[%d]=0x%x", __func__, get_shmem->index, get_shmem->num, i, get_shmem->array[i]);
                            PAL_INFO(LOG_TAG, "shmem_buff_get_parameter is successful");
                        }
                        free(payload);
                    }
                }

                PAL_INFO(LOG_TAG,"%s 2nd Test Case set persist Read from virtual", __func__);
                {
                    uint32_t num = 0;
                    uint32_t i = 0, count = 0;
                    num = 1024;

                    for(i = 0; i < num; i++)
                    {
                        uint8_t *payload = NULL;
                        uint32_t payload_size;
                        pal_load_set_persist_dummy_t *set_shmem = nullptr;
                        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_set_persist_dummy_t);
                        payload = (uint8_t *) calloc (1, payload_size);
                        if (!payload) {
                            PAL_ERR(LOG_TAG,"%s calloc failed for size %d", __func__, payload_size);
                            status = -ENOMEM;
                            goto exit;
                        }
                        set_shmem = (pal_load_set_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
                        set_shmem->effective = 1;
                        set_shmem->index = i;
                        set_shmem->value = i * 2;
                        PAL_INFO(LOG_TAG,"%s effective=%d index=0x%x value[%d]=0x%x", __func__, set_shmem->effective, set_shmem->index, i, set_shmem->value);
                        status = shmem_buff_set_persist_parameter(pal_stream_handle, payload);
                        if (status) {
                            PAL_ERR(LOG_TAG, "shmem_buff_set_persist_parameter failed");
                            free(payload);
                            status = -EINVAL;
                            goto exit;
                        }
                        else
                            PAL_INFO(LOG_TAG, "shmem_buff_set_persist_parameter is successful");

                        free(payload);
                    }

                    {
                        /*Use this data as buffer address in userspace*/
                        for(i = 0; i < num; i++ ) {
                            if(p[i] != i * 2) {
                                count++;
                                PAL_ERR(LOG_TAG,"%s set data is not matching with p[%d]=0x%x", __func__, i, p[i]);
                            }
                        }
                        if(count == 0)
                            PAL_INFO(LOG_TAG, "Test case 2 passed");
                    }
                }

                PAL_INFO(LOG_TAG,"%s 3rd Test Case Write to virtual and get param", __func__);
                {
                    uint32_t num = 0;
                    uint32_t i = 0, count = 0;
                    num = 1024;
                    {
                        /*Use this data as buffer address in userspace*/
                        for(i = 0; i < num; i++ ){
                            p[i] = i;
                        }
                    }

                    {
                        uint8_t *payload = NULL;
                        uint32_t payload_size;
                        pal_load_get_persist_dummy_t *set_shmem1 = nullptr;
                        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(pal_load_get_persist_dummy_t);
                        payload = (uint8_t *) calloc (1, payload_size);
                        if (!payload) {
                            PAL_ERR(LOG_TAG,"%s calloc failed for size %d", __func__, payload_size);
                            status = -ENOMEM;
                            goto exit;
                        }
                        set_shmem1 = (pal_load_get_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
                        set_shmem1->index = 0;
                        set_shmem1->num = num;
                        status = shmem_buff_set_parameter(pal_stream_handle, payload);
                        if (status) {
                            PAL_ERR(LOG_TAG, "shmem_buff_set_parameter failed");
                            free(payload);
                            status = -EINVAL;
                            goto exit;
                        }
                        else
                            PAL_INFO(LOG_TAG, "shmem_buff_set_parameter is successful");

                        free(payload);
                    }

                    {
                        uint8_t *payload = NULL;
                        uint32_t payload_size;
                        pal_load_get_persist_dummy_t *get_shmem = nullptr;

                        uint32_t gpd_size = sizeof(pal_load_get_persist_dummy_t)+(num*sizeof(uint32_t));

                        payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + gpd_size;
                        payload = (uint8_t *) calloc (1, payload_size);
                        if (!payload) {
                            PAL_ERR(LOG_TAG,"%s calloc failed for size %d", __func__, payload_size);
                            status = -ENOMEM;
                            goto exit;
                        }
                        get_shmem = (pal_load_get_persist_dummy_t *)(payload + sizeof(pal_param_payload) + sizeof(pal_effect_custom_payload_t) + sizeof(effect_pal_payload_t));
                        status = shmem_buff_get_parameter(pal_stream_handle, payload, gpd_size);
                        if (status) {
                            PAL_ERR(LOG_TAG, "shmem_buff_get_parameter failed");
                            free(payload);
                            status = -EINVAL;
                            goto exit;
                        }
                        else{
                            for(i = 0; i < get_shmem->num; i++){
                                if(p[i] != get_shmem->array[i]) {
                                    count++;
                                    PAL_ERR(LOG_TAG,"%s mismatch in data for index=%d num=0x%x p[%d]=0x%x array[%d]=0x%x", __func__, get_shmem->index, get_shmem->num, i, p[i], i, get_shmem->array[i]);
                                }
                            }
                            if(count == 0)
                                PAL_INFO(LOG_TAG, "Test case 3 passed");
                        }
                        free(payload);
                    }
                }
exit:
                munmap(p, buff_info.size);
            }
            break;
        default:
            PAL_ERR(LOG_TAG,"control not supported in this plugin");
            status = -EINVAL;
            break;
    }
    PAL_DBG(LOG_TAG,"Exit with status: %d", status);
    return status;
}

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

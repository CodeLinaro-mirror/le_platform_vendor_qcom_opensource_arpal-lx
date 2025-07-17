/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ipc_interface.h"

class PalService : public BnPalService {
    public:
        PalService();
        ~PalService();
        virtual int32_t ipc_pal_init() override;
        virtual std::string ipc_pal_get_version() override;
        virtual void ipc_pal_deinit() override;
        virtual int32_t ipc_pal_stream_open(struct pal_stream_attributes *attributes,
                                            uint32_t no_of_devices, struct pal_device *devices,
                                            uint32_t no_of_modifiers, struct modifier_kv *modifiers,
                                            pal_stream_callback cb, uint64_t cookie,
                                            pal_stream_handle_t **stream_handle) override;
        virtual int32_t ipc_pal_stream_close(pal_stream_handle_t *stream_handle) override;
        virtual int32_t ipc_pal_stream_start(pal_stream_handle_t *stream_handle) override;
        virtual int32_t ipc_pal_stream_stop(pal_stream_handle_t *stream_handle) override;
        virtual int32_t ipc_pal_stream_pause(pal_stream_handle_t *stream_handle) override;
        virtual int32_t ipc_pal_stream_resume(pal_stream_handle_t *stream_handle) override;
        virtual int32_t ipc_pal_stream_flush(pal_stream_handle_t *stream_handle) override;
        virtual int32_t ipc_pal_stream_drain(pal_stream_handle_t *stream_handle, pal_drain_type_t type) override;
        virtual int32_t ipc_pal_stream_suspend(pal_stream_handle_t *stream_handle) override;
        virtual int32_t ipc_pal_stream_get_buffer_size(pal_stream_handle_t *stream_handle,
                                                       size_t *in_buffer, size_t *out_buffer) override;
        virtual int32_t ipc_pal_stream_set_buffer_size(pal_stream_handle_t *stream_handle,
                                                       pal_buffer_config_t *in_buff_cfg,
                                                       pal_buffer_config_t *out_buff_cfg) override;
        virtual ssize_t ipc_pal_stream_read(pal_stream_handle_t *stream_handle, struct pal_buffer *buf) override;
        virtual ssize_t ipc_pal_stream_write(pal_stream_handle_t *stream_handle, struct pal_buffer *buf) override;
        virtual int32_t ipc_pal_stream_get_device(pal_stream_handle_t *stream_handle,
                                                  uint32_t no_of_devices, struct pal_device *devices) override;
        virtual int32_t ipc_pal_stream_set_device(pal_stream_handle_t *stream_handle,
                                                  uint32_t no_of_devices, struct pal_device *devices) override;
        virtual int32_t ipc_pal_stream_get_param(pal_stream_handle_t *stream_handle,
                                                 uint32_t param_id, pal_param_payload **param_payload) override;
        virtual int32_t ipc_pal_stream_set_param(pal_stream_handle_t *stream_handle,
                                                 uint32_t param_id, pal_param_payload *param_payload) override;
        virtual int32_t ipc_pal_stream_get_volume(pal_stream_handle_t *stream_handle,
                                                 struct pal_volume_data *volume) override;
        virtual int32_t ipc_pal_stream_set_volume(pal_stream_handle_t *stream_handle,
                                                  struct pal_volume_data *volume) override;
        virtual int32_t ipc_pal_stream_get_mute(pal_stream_handle_t *stream_handle, bool *state) override;
        virtual int32_t ipc_pal_stream_set_mute(pal_stream_handle_t *stream_handle, bool state) override;

        virtual int32_t ipc_pal_get_mic_mute(bool *state) override;
        virtual int32_t ipc_pal_set_mic_mute(bool state) override;
        virtual int32_t ipc_pal_get_timestamp(pal_stream_handle_t *stream_handle, struct pal_session_time *stime) override;
        virtual int32_t ipc_pal_add_remove_effect(pal_stream_handle_t *stream_handle,
                                                  pal_audio_effect_t effect, bool enable) override;
        virtual int32_t ipc_pal_set_param(uint32_t param_id, void *param_payload, size_t payload_size) override;
        virtual int32_t ipc_pal_get_param(uint32_t param_id, void **param_payload,
                                size_t *payload_size, void *query) override;

        virtual int32_t ipc_pal_stream_create_mmap_buffer(pal_stream_handle_t *stream_handle,
                                                int32_t min_size_frames,
                                                struct pal_mmap_buffer *info) override;
        virtual int32_t ipc_pal_stream_get_mmap_position(pal_stream_handle_t *stream_handle,
                                                struct pal_mmap_position *position) override;
        virtual int32_t ipc_pal_register_global_callback(pal_global_callback cb, uint64_t cookie) override;
        virtual int32_t ipc_pal_stream_set_custom_param(pal_stream_handle_t* handle,
              const char param_str[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH],
              void* param_payload, size_t payload_size) override;
        virtual int32_t ipc_pal_stream_get_custom_param(pal_stream_handle_t* handle,
              const char param_str[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH],
              void* param_payload, size_t* payload_size) override;
        virtual int32_t ipc_pal_set_custom_param(custom_payload_uc_info_t* uc_info,
              const char param_str[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH],
              void* param_payload, size_t payload_size) override;
        virtual int32_t ipc_pal_get_custom_param(custom_payload_uc_info_t* uc_info,
              const char param_str[PAL_CUSTOM_PARAM_MAX_STRING_LENGTH],
              void* param_payload, size_t* payload_size) override;

    private:
        bool pal_initialized = false;
};

extern "C" void load_pal_service();

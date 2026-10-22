// ============================================================================
//  platform_api.cpp - every function declared in platform_api.h, for a platform
//  that has nothing
//
//  What a server or a cloud deployment compiles against: the same contract, no
//  capabilities. Zero capabilities and FAM_E_UNSUPPORTED for everything that
//  would need a desktop, so the daemon exercises its degradation paths instead of
//  discovering the difference at runtime.
// ============================================================================

#include "platform_api.h"

namespace
{
    bool Initialised = false;
} // namespace

extern "C"
{
    fam_status_t fam_platform_init(const struct fam_host* host)
    {
        if (host == nullptr || host->size < sizeof(struct fam_host))
        {
            return FAM_E_INVALID;
        }

        if (Initialised)
        {
            return FAM_E_BUSY;
        }

        Initialised = true;
        return FAM_OK;
    }

    void fam_platform_terminate(void)
    {
        Initialised = false;
    }

    uint32_t fam_platform_abi_version(void)
    {
        return FAM_PLATFORM_ABI_VERSION;
    }

    fam_capabilities_t fam_platform_capabilities(void)
    {
        return 0;
    }

    fam_status_t fam_platform_limits(struct fam_limits* out_limits)
    {
        if (!Initialised || out_limits == nullptr)
        {
            return FAM_E_INVALID;
        }

        out_limits->notification_max_actions = 0;
        out_limits->artifact_max_blocks = 64;
        out_limits->frame_max_bytes = 64u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000;

        return FAM_OK;
    }

    fam_status_t fam_platform_paths(enum fam_path_kind kind, char* buf, size_t* inout_len)
    {
        (void)kind;
        (void)buf;
        (void)inout_len;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_hotkey_bind(const char* accelerator, uint32_t* out_id)
    {
        (void)accelerator;
        (void)out_id;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_hotkey_unbind(uint32_t id)
    {
        (void)id;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_secret_set(const char* name, const void* data, size_t size)
    {
        (void)name;
        (void)data;
        (void)size;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_secret_get(const char* name, unsigned char* buf, size_t* inout_len)
    {
        (void)name;
        (void)buf;
        (void)inout_len;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_secret_delete(const char* name)
    {
        (void)name;

        return FAM_E_UNSUPPORTED;
    }

    int fam_platform_can_listen(void)
    {
        return 0;
    }

    fam_status_t fam_platform_listen(struct fam_listener** out_listener)
    {
        (void)out_listener;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_accept(struct fam_listener* listener, struct fam_stream** out_stream)
    {
        (void)listener;
        (void)out_stream;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_connect(uint32_t timeout_ms, struct fam_stream** out_stream)
    {
        (void)timeout_ms;
        (void)out_stream;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_read_line(struct fam_stream* stream, char* buf, size_t* inout_len)
    {
        (void)stream;
        (void)buf;
        (void)inout_len;

        return FAM_E_UNSUPPORTED;
    }

    fam_status_t fam_platform_write_line(struct fam_stream* stream, const char* text, size_t length)
    {
        (void)stream;
        (void)text;
        (void)length;

        return FAM_E_UNSUPPORTED;
    }

    void fam_platform_release_stream(struct fam_stream* stream)
    {
        (void)stream;
    }

    void fam_platform_release_listener(struct fam_listener* listener)
    {
        (void)listener;
    }
}

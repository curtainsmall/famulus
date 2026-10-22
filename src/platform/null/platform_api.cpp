// ============================================================================
//  platform_api.cpp - every function declared in platform_api.h, for a platform
//  that has nothing
//
//  What a server or a cloud deployment compiles against: the same contract, no
//  capabilities. Zero capabilities and FAM_UNSUPPORTED for everything that
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
    FamStatus famInit(const FamHost* host)
    {
        if (host == nullptr || host->size < sizeof(FamHost))
        {
            return FAM_INVALID;
        }

        if (Initialised)
        {
            return FAM_BUSY;
        }

        Initialised = true;
        return FAM_OK;
    }

    void famTerminate(void)
    {
        Initialised = false;
    }

    uint32_t famGetAbiVersion(void)
    {
        return FAM_PLATFORM_ABI_VERSION;
    }

    FamCapabilities famGetCapabilities(void)
    {
        return 0;
    }

    FamStatus famGetLimits(FamLimits* out_limits)
    {
        if (!Initialised || out_limits == nullptr)
        {
            return FAM_INVALID;
        }

        out_limits->notification_max_actions = 0;
        out_limits->artifact_max_blocks = 64;
        out_limits->frame_max_bytes = 64u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000;

        return FAM_OK;
    }

    FamStatus famGetPath(FamPathKind kind, const char** out_path)
    {
        (void)kind;
        (void)out_path;

        return FAM_UNSUPPORTED;
    }

    FamStatus famBindHotkey(const char* accelerator, uint32_t* out_id)
    {
        (void)accelerator;
        (void)out_id;

        return FAM_UNSUPPORTED;
    }

    FamStatus famUnbindHotkey(uint32_t id)
    {
        (void)id;

        return FAM_UNSUPPORTED;
    }

    FamStatus famSetSecret(const char* name, const void* data, size_t size)
    {
        (void)name;
        (void)data;
        (void)size;

        return FAM_UNSUPPORTED;
    }

    FamStatus famGetSecret(const char* name, const unsigned char** out_data, size_t* out_len)
    {
        (void)name;
        (void)out_data;
        (void)out_len;

        return FAM_UNSUPPORTED;
    }

    FamStatus famDeleteSecret(const char* name)
    {
        (void)name;

        return FAM_UNSUPPORTED;
    }

    int famCanListen(void)
    {
        return 0;
    }

    FamStatus famListen(FamListener** out_listener)
    {
        (void)out_listener;

        return FAM_UNSUPPORTED;
    }

    FamStatus famAccept(FamListener* listener, FamStream** out_stream)
    {
        (void)listener;
        (void)out_stream;

        return FAM_UNSUPPORTED;
    }

    FamStatus famConnect(uint32_t timeout_ms, FamStream** out_stream)
    {
        (void)timeout_ms;
        (void)out_stream;

        return FAM_UNSUPPORTED;
    }

    FamStatus famReadLine(FamStream* stream, const char** out_text, size_t* out_len)
    {
        (void)stream;
        (void)out_text;
        (void)out_len;

        return FAM_UNSUPPORTED;
    }

    FamStatus famWriteLine(FamStream* stream, const char* text, size_t length)
    {
        (void)stream;
        (void)text;
        (void)length;

        return FAM_UNSUPPORTED;
    }

    void famReleaseStream(FamStream* stream)
    {
        (void)stream;
    }

    void famReleaseListener(FamListener* listener)
    {
        (void)listener;
    }
}

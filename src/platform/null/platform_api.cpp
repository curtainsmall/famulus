// ============================================================================
//  platform_api.cpp - every function declared in platform_api.h, for a platform
//  that has nothing
//
//  What a server or a cloud deployment compiles against: the same contract, no
//  capabilities. Zero capabilities and FamStatus_Unsupported for everything that
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
            return FamStatus_Invalid;
        }

        if (Initialised)
        {
            return FamStatus_Busy;
        }

        Initialised = true;
        return FamStatus_Ok;
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
            return FamStatus_Invalid;
        }

        out_limits->notification_max_actions = 0;
        out_limits->artifact_max_blocks = 64;
        out_limits->frame_max_bytes = 64u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000;

        return FamStatus_Ok;
    }

    FamStatus famGetPath(FamPathKind kind, const char** out_path)
    {
        (void)kind;
        (void)out_path;

        return FamStatus_Unsupported;
    }

    FamStatus famBindHotkey(const char* accelerator, uint32_t* out_id)
    {
        (void)accelerator;
        (void)out_id;

        return FamStatus_Unsupported;
    }

    FamStatus famUnbindHotkey(uint32_t id)
    {
        (void)id;

        return FamStatus_Unsupported;
    }

    FamStatus famSetSecret(const char* name, const void* data, size_t size)
    {
        (void)name;
        (void)data;
        (void)size;

        return FamStatus_Unsupported;
    }

    FamStatus famGetSecret(const char* name, const unsigned char** out_data, size_t* out_len)
    {
        (void)name;
        (void)out_data;
        (void)out_len;

        return FamStatus_Unsupported;
    }

    FamStatus famDeleteSecret(const char* name)
    {
        (void)name;

        return FamStatus_Unsupported;
    }

    int famCanListen(void)
    {
        return 0;
    }

    FamStatus famListen(FamListener** out_listener)
    {
        (void)out_listener;

        return FamStatus_Unsupported;
    }

    FamStatus famAccept(FamListener* listener, FamStream** out_stream)
    {
        (void)listener;
        (void)out_stream;

        return FamStatus_Unsupported;
    }

    FamStatus famConnect(uint32_t timeout_ms, FamStream** out_stream)
    {
        (void)timeout_ms;
        (void)out_stream;

        return FamStatus_Unsupported;
    }

    FamStatus famReadLine(FamStream* stream, const char** out_text, size_t* out_len)
    {
        (void)stream;
        (void)out_text;
        (void)out_len;

        return FamStatus_Unsupported;
    }

    FamStatus famWriteLine(FamStream* stream, const char* text, size_t length)
    {
        (void)stream;
        (void)text;
        (void)length;

        return FamStatus_Unsupported;
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

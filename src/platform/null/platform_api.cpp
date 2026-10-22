// ============================================================================
//  platform_api.cpp - every function declared in platform_api.h, for a platform
//  that has nothing
//
//  What a server or a cloud deployment compiles against: the same contract, no
//  capabilities. Zero capabilities and FamError_Unsupported for everything that
//  would need a desktop, so the daemon exercises its degradation paths instead of
//  discovering the difference at runtime.
//
//  The error slot lives here as two file statics: with no context to hang it on,
//  this is the whole of the adapter's state besides the initialised flag.
// ============================================================================

#include <string>
#include <string_view>

#include "platform_api.h"

namespace
{
    bool Initialised = false;
    FamError LastError = FamError_None;
    std::string LastErrorText;

    void ClearError()
    {
        LastError = FamError_None;
        LastErrorText.clear();
    }

    void SetError(FamError error, std::string_view text)
    {
        LastError = error;
        LastErrorText.assign(text);
    }

    // Everything this adapter can be asked for needs a desktop it does not have.
    auto NoDesktop() -> bool
    {
        SetError(FamError_Unsupported, "this platform has no local desktop");
        return false;
    }

    auto NotReady() -> bool
    {
        SetError(FamError_NotReady, "famInit has not been called");
        return false;
    }
} // namespace

extern "C"
{
    FamError famGetLastError(const char** description)
    {
        if (description != nullptr)
        {
            *description = LastErrorText.empty() ? nullptr : LastErrorText.c_str();
        }

        return LastError;
    }

    bool famInit(const FamHost* host)
    {
        ClearError();

        if (host == nullptr || host->size < sizeof(FamHost))
        {
            SetError(FamError_Invalid, "the host table is missing or short");
            return false;
        }

        if (Initialised)
        {
            SetError(FamError_Busy, "the platform is already initialised in this process");
            return false;
        }

        Initialised = true;

        return true;
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
        ClearError();

        return 0;
    }

    bool famGetLimits(FamLimits* out_limits)
    {
        ClearError();

        if (!Initialised)
        {
            return NotReady();
        }

        if (out_limits == nullptr)
        {
            SetError(FamError_Invalid, "no FamLimits to write to");
            return false;
        }

        out_limits->notification_max_actions = 0;
        out_limits->artifact_max_blocks = 64;
        out_limits->frame_max_bytes = 64u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000;

        return true;
    }

    const char* famGetPath(FamPathKind kind)
    {
        ClearError();
        (void)kind;

        NoDesktop();

        return nullptr;
    }

    uint32_t famBindHotkey(const char* accelerator)
    {
        ClearError();
        (void)accelerator;

        NoDesktop();

        return 0;
    }

    bool famUnbindHotkey(uint32_t id)
    {
        ClearError();
        (void)id;

        return NoDesktop();
    }

    bool famSetSecret(const char* name, const void* data, size_t size)
    {
        ClearError();
        (void)name;
        (void)data;
        (void)size;

        return NoDesktop();
    }

    const unsigned char* famGetSecret(const char* name, size_t* out_len)
    {
        ClearError();
        (void)name;
        (void)out_len;

        NoDesktop();

        return nullptr;
    }

    bool famDeleteSecret(const char* name)
    {
        ClearError();
        (void)name;

        return NoDesktop();
    }

    bool famCanListen(void)
    {
        ClearError();

        // Not an error: this platform simply has no local endpoint, and the
        // daemon says so rather than reporting a second instance.
        return false;
    }

    FamListener* famListen(void)
    {
        ClearError();

        NoDesktop();

        return nullptr;
    }

    FamStream* famAccept(FamListener* listener)
    {
        ClearError();
        (void)listener;

        NoDesktop();

        return nullptr;
    }

    FamStream* famConnect(uint32_t timeout_ms)
    {
        ClearError();
        (void)timeout_ms;

        NoDesktop();

        return nullptr;
    }

    bool famReadLine(FamStream* stream, const char** out_text, size_t* out_len)
    {
        ClearError();
        (void)stream;
        (void)out_text;
        (void)out_len;

        return NoDesktop();
    }

    bool famWriteLine(FamStream* stream, const char* text, size_t length)
    {
        ClearError();
        (void)stream;
        (void)text;
        (void)length;

        return NoDesktop();
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

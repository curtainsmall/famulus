// ============================================================================
//  platform_api.cpp - every function declared in platform_api.h
//
//  Each function validates its arguments and makes one call on the object that
//  owns the behaviour: PlatformEnvironment, HotkeyWorker, SecretStore, all held
//  by the one context famInit creates.
//
//  The context is a file-static singleton, which is what lets the rest of the
//  header carry no handle at all. It is created by init and freed by terminate,
//  so a caller that forgets a fam_platform_release_* still leaks nothing.
// ============================================================================

#include <chrono>
#include <cstddef>
#include <new>
#include <string>

#include "context.hpp"

namespace
{
    // The one context of the process. Null before init and after terminate.
    fam_context* Context = nullptr;
} // namespace

extern "C"
{
    FamStatus famInit(const FamHost* host)
    {
        if (host == nullptr || host->size < sizeof(FamHost) || host->post_event == nullptr)
        {
            return FamStatus_Invalid;
        }

        if (Context != nullptr)
        {
            return FamStatus_Busy;
        }

        auto* created = new (std::nothrow) fam_context(host);

        if (created == nullptr)
        {
            return FamStatus_NoMem;
        }

        if (!created->Environment.IsReady())
        {
            // A daemon that cannot create its own directories should find out at
            // startup, not at the first write.
            delete created;
            return FamStatus_Io;
        }

        Context = created;
        return FamStatus_Ok;
    }

    void famTerminate(void)
    {
        delete Context;
        Context = nullptr;
    }

    uint32_t famGetAbiVersion(void)
    {
        return FAM_PLATFORM_ABI_VERSION;
    }

    FamCapabilities famGetCapabilities(void)
    {
        if (Context == nullptr)
        {
            return 0;
        }

        return static_cast<FamCapabilities>(FamCapability_Paths | FamCapability_Hotkey | FamCapability_Secret);
    }

    FamStatus famGetLimits(FamLimits* out_limits)
    {
        if (Context == nullptr || out_limits == nullptr)
        {
            return FamStatus_Invalid;
        }

        out_limits->notification_max_actions = 0; /* no notification channel yet */
        out_limits->artifact_max_blocks = 256;
        out_limits->frame_max_bytes = 4u * 1024u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000; /* Task Scheduler granularity */

        return FamStatus_Ok;
    }

    FamStatus famGetPath(FamPathKind kind, const char** out_path)
    {
        if (Context == nullptr || out_path == nullptr)
        {
            return FamStatus_Invalid;
        }

        *out_path = nullptr;

        if (kind != FamPathKind_Config && kind != FamPathKind_Data && kind != FamPathKind_Cache && kind != FamPathKind_State)
        {
            return FamStatus_Invalid;
        }

        if (!Context->Environment.EnsureDirectory(kind))
        {
            return FamStatus_Io;
        }

        *out_path = Context->Environment.GetPathView(kind);

        return *out_path != nullptr ? FamStatus_Ok : FamStatus_Io;
    }

    FamStatus famBindHotkey(const char* accelerator, uint32_t* out_id)
    {
        if (Context == nullptr || accelerator == nullptr || out_id == nullptr)
        {
            return FamStatus_Invalid;
        }

        return Context->Hotkeys.Bind(accelerator, *out_id);
    }

    FamStatus famUnbindHotkey(uint32_t id)
    {
        if (Context == nullptr || id == 0)
        {
            return FamStatus_Invalid;
        }

        return Context->Hotkeys.Unbind(id);
    }

    FamStatus famSetSecret(const char* name, const void* data, size_t size)
    {
        if (Context == nullptr || name == nullptr || (data == nullptr && size != 0))
        {
            return FamStatus_Invalid;
        }

        return Context->Secrets.Set(name, data, size);
    }

    FamStatus famGetSecret(const char* name, const unsigned char** out_data, size_t* out_len)
    {
        if (Context == nullptr || name == nullptr || out_data == nullptr || out_len == nullptr)
        {
            return FamStatus_Invalid;
        }

        return Context->Secrets.GetView(name, out_data, out_len);
    }

    FamStatus famDeleteSecret(const char* name)
    {
        if (Context == nullptr || name == nullptr)
        {
            return FamStatus_Invalid;
        }

        return Context->Secrets.Delete(name);
    }

    int famCanListen(void)
    {
        return Context != nullptr ? 1 : 0;
    }

    FamStatus famListen(FamListener** out_listener)
    {
        if (Context == nullptr || out_listener == nullptr)
        {
            return FamStatus_Invalid;
        }

        auto* listener = new (std::nothrow) FamListener{};

        if (listener == nullptr)
        {
            return FamStatus_NoMem;
        }

        listener->Owner = Context;

        if (!listener->Current.Listen(Famulus::Windows::NamedPipe::DefaultName()))
        {
            delete listener;
            return FamStatus_Busy;
        }

        Context->Track(listener);
        *out_listener = listener;

        return FamStatus_Ok;
    }

    FamStatus famAccept(FamListener* listener, FamStream** out_stream)
    {
        if (listener == nullptr || out_stream == nullptr || listener->Owner == nullptr)
        {
            return FamStatus_Invalid;
        }

        if (!listener->Current.Accept())
        {
            return FamStatus_Io;
        }

        // Put a fresh instance in place before anyone can observe a moment with no
        // endpoint at all: an instance is what makes a second daemon fail.
        Famulus::Windows::NamedPipe served = std::move(listener->Current);
        Famulus::Windows::NamedPipe replacement;

        if (!replacement.Open(served.GetName()))
        {
            return FamStatus_Io;
        }

        listener->Current = std::move(replacement);

        auto* stream = new (std::nothrow) FamStream{listener->Owner, std::move(served)};

        if (stream == nullptr)
        {
            return FamStatus_NoMem;
        }

        listener->Owner->Track(stream);
        *out_stream = stream;

        return FamStatus_Ok;
    }

    FamStatus famConnect(uint32_t timeout_ms, FamStream** out_stream)
    {
        if (Context == nullptr || out_stream == nullptr)
        {
            return FamStatus_Invalid;
        }

        Famulus::Windows::NamedPipe pipe;

        if (!pipe.Connect(Famulus::Windows::NamedPipe::DefaultName(), std::chrono::milliseconds{timeout_ms}))
        {
            return FamStatus_Denied;
        }

        auto* stream = new (std::nothrow) FamStream{Context, std::move(pipe)};

        if (stream == nullptr)
        {
            return FamStatus_NoMem;
        }

        Context->Track(stream);
        *out_stream = stream;

        return FamStatus_Ok;
    }

    FamStatus famReadLine(FamStream* stream, const char** out_text, size_t* out_len)
    {
        if (stream == nullptr || out_text == nullptr || out_len == nullptr || stream->Owner == nullptr)
        {
            return FamStatus_Invalid;
        }

        *out_text = nullptr;
        *out_len = 0;

        if (!stream->Pipe.ReadLine(stream->Owner->LineScratch))
        {
            return FamStatus_Io;
        }

        *out_len = stream->Owner->LineScratch.size();
        *out_text = stream->Owner->LineScratch.data();

        return FamStatus_Ok;
    }

    FamStatus famWriteLine(FamStream* stream, const char* text, size_t length)
    {
        if (stream == nullptr || (text == nullptr && length != 0))
        {
            return FamStatus_Invalid;
        }

        return stream->Pipe.WriteLine(std::string_view(text, length)) ? FamStatus_Ok : FamStatus_Io;
    }

    void famReleaseStream(FamStream* stream)
    {
        if (stream == nullptr)
        {
            return;
        }

        if (stream->Owner != nullptr)
        {
            stream->Owner->Forget(stream);
        }

        delete stream;
    }

    void famReleaseListener(FamListener* listener)
    {
        if (listener == nullptr)
        {
            return;
        }

        if (listener->Owner != nullptr)
        {
            listener->Owner->Forget(listener);
        }

        delete listener;
    }
}

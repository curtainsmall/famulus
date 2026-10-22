// ============================================================================
//  platform_api.cpp - every function declared in platform_api.h
//
//  Each function validates its arguments and makes one call on the object that
//  owns the behaviour: PlatformEnvironment, HotkeyWorker, SecretStore, all held
//  by the one context famInit creates.
//
//  The context is a file-static singleton, which is what lets the rest of the
//  header carry no handle at all. It is created by init and freed by terminate,
//  so a caller that forgets a famRelease* still leaks nothing.
//
//  Every entry point does the same three things in the same order: clear the
//  error slot, check the argument, act. What it returns answers "did it work";
//  the slot answers "why not".
// ============================================================================

#include <chrono>
#include <cstddef>
#include <new>
#include <string>
#include <string_view>

#include "context.hpp"

namespace
{
    // The one context of the process. Null before init and after terminate.
    FamContext* Context = nullptr;

    // An error needs somewhere to live before there is a context - famInit's own
    // failure, and any call made before init or after terminate, would otherwise
    // have nothing to report through.
    FamError OrphanError = FamError_None;
    std::string OrphanText;

    void ClearError()
    {
        if (Context != nullptr)
        {
            Context->ClearError();
            return;
        }

        OrphanError = FamError_None;
        OrphanText.clear();
    }

    void SetError(FamError error, std::string_view text)
    {
        if (Context != nullptr)
        {
            Context->SetError(error, text);
            return;
        }

        OrphanError = error;
        OrphanText.assign(text);
    }

    // The argument checks every entry point starts with. Returns true when the
    // call may continue, and leaves the slot explaining why when it may not.
    auto Ready() -> bool
    {
        if (Context != nullptr)
        {
            return true;
        }

        SetError(FamError_NotReady, "famInit has not been called, or it failed");
        return false;
    }
} // namespace

extern "C"
{
    FamError famGetLastError(const char** description)
    {
        // Deliberately does not clear: it would erase what it is about to report.
        if (Context != nullptr)
        {
            return Context->GetError(description);
        }

        if (description != nullptr)
        {
            *description = OrphanText.empty() ? nullptr : OrphanText.c_str();
        }

        return OrphanError;
    }

    bool famInit(const FamHost* host)
    {
        ClearError();

        if (host == nullptr || host->size < sizeof(FamHost) || host->post_event == nullptr)
        {
            SetError(FamError_Invalid, "the host table is missing, short, or has no post_event");
            return false;
        }

        if (Context != nullptr)
        {
            SetError(FamError_Busy, "the platform is already initialised in this process");
            return false;
        }

        auto* created = new (std::nothrow) FamContext(host);

        if (created == nullptr)
        {
            SetError(FamError_System, "cannot allocate the platform context");
            return false;
        }

        if (!created->Environment.IsReady())
        {
            // A daemon that cannot create its own directories should find out at
            // startup, not at the first write.
            delete created;
            SetError(FamError_System, "cannot create the storage directories");
            return false;
        }

        Context = created;

        return true;
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
        ClearError();

        if (Context == nullptr)
        {
            SetError(FamError_NotReady, "famInit has not been called");
            return 0;
        }

        return static_cast<FamCapabilities>(FamCapability_Paths | FamCapability_Hotkey | FamCapability_Secret);
    }

    bool famGetLimits(FamLimits* out_limits)
    {
        ClearError();

        if (Context == nullptr)
        {
            SetError(FamError_NotReady, "famInit has not been called");
            return false;
        }

        if (out_limits == nullptr)
        {
            SetError(FamError_Invalid, "no FamLimits to write to");
            return false;
        }

        out_limits->notification_max_actions = 0; /* no notification channel yet */
        out_limits->artifact_max_blocks = 256;
        out_limits->frame_max_bytes = 4u * 1024u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000; /* Task Scheduler granularity */

        return true;
    }

    const char* famGetPath(FamPathKind kind)
    {
        ClearError();

        if (!Ready())
        {
            return nullptr;
        }

        if (kind != FamPathKind_Config && kind != FamPathKind_Data && kind != FamPathKind_Cache &&
            kind != FamPathKind_State)
        {
            SetError(FamError_Invalid, "unknown FamPathKind");
            return nullptr;
        }

        if (!Context->Environment.EnsureDirectory(kind))
        {
            SetError(FamError_System, "cannot create the directory for this kind");
            return nullptr;
        }

        const char* path = Context->Environment.GetPathView(kind);

        if (path == nullptr)
        {
            SetError(FamError_System, "the directory has no path");
        }

        return path;
    }

    uint32_t famBindHotkey(const char* accelerator)
    {
        ClearError();

        if (!Ready())
        {
            return 0;
        }

        if (accelerator == nullptr)
        {
            SetError(FamError_Invalid, "no accelerator");
            return 0;
        }

        uint32_t id = 0;
        const FamError result = Context->Hotkeys.Bind(accelerator, id);

        if (result == FamError_Busy)
        {
            SetError(result, "that accelerator is already registered");
            return 0;
        }

        if (result != FamError_None)
        {
            SetError(result, "the accelerator was refused");
            return 0;
        }

        return id;
    }

    bool famUnbindHotkey(uint32_t id)
    {
        ClearError();

        if (!Ready())
        {
            return false;
        }

        if (id == 0)
        {
            SetError(FamError_Invalid, "0 is never a hotkey id");
            return false;
        }

        const FamError result = Context->Hotkeys.Unbind(id);

        if (result != FamError_None)
        {
            SetError(result, "no hotkey is registered under that id");
        }

        return result == FamError_None;
    }

    bool famSetSecret(const char* name, const void* data, size_t size)
    {
        ClearError();

        if (!Ready())
        {
            return false;
        }

        if (name == nullptr || (data == nullptr && size != 0))
        {
            SetError(FamError_Invalid, "no name, or no data with a non-zero size");
            return false;
        }

        const FamError result = Context->Secrets.Set(name, data, size);

        if (result != FamError_None)
        {
            SetError(result, "cannot store the secret");
        }

        return result == FamError_None;
    }

    const unsigned char* famGetSecret(const char* name, size_t* out_len)
    {
        ClearError();

        if (!Ready())
        {
            return nullptr;
        }

        if (name == nullptr || out_len == nullptr)
        {
            SetError(FamError_Invalid, "no name, or nowhere to put the length");
            return nullptr;
        }

        const unsigned char* data = nullptr;
        const FamError result = Context->Secrets.GetView(name, &data, out_len);

        if (result != FamError_None)
        {
            SetError(result, "no secret is stored under that name");
        }

        return data;
    }

    bool famDeleteSecret(const char* name)
    {
        ClearError();

        if (!Ready())
        {
            return false;
        }

        if (name == nullptr)
        {
            SetError(FamError_Invalid, "no name");
            return false;
        }

        const FamError result = Context->Secrets.Delete(name);

        if (result != FamError_None)
        {
            SetError(result, "no secret is stored under that name");
        }

        return result == FamError_None;
    }

    bool famCanListen(void)
    {
        ClearError();

        return Ready();
    }

    FamListener* famListen(void)
    {
        ClearError();

        if (!Ready())
        {
            return nullptr;
        }

        auto* listener = new (std::nothrow) FamListener{};

        if (listener == nullptr)
        {
            SetError(FamError_System, "cannot allocate a listener");
            return nullptr;
        }

        listener->Owner = Context;

        if (!listener->Current.Listen(Famulus::Windows::NamedPipe::DefaultName()))
        {
            delete listener;
            SetError(FamError_Busy, "another daemon already holds this endpoint");
            return nullptr;
        }

        Context->Track(listener);

        return listener;
    }

    FamStream* famAccept(FamListener* listener)
    {
        ClearError();

        if (listener == nullptr || listener->Owner == nullptr)
        {
            SetError(FamError_Invalid, "no listener");
            return nullptr;
        }

        if (!listener->Current.Accept())
        {
            SetError(FamError_System, "the endpoint refused a client");
            return nullptr;
        }

        // Put a fresh instance in place before anyone can observe a moment with no
        // endpoint at all: an instance is what makes a second daemon fail.
        Famulus::Windows::NamedPipe served = std::move(listener->Current);
        Famulus::Windows::NamedPipe replacement;

        if (!replacement.Open(served.GetName()))
        {
            SetError(FamError_System, "cannot open a replacement endpoint instance");
            return nullptr;
        }

        listener->Current = std::move(replacement);

        auto* stream = new (std::nothrow) FamStream{listener->Owner, std::move(served)};

        if (stream == nullptr)
        {
            SetError(FamError_System, "cannot allocate a stream");
            return nullptr;
        }

        listener->Owner->Track(stream);

        return stream;
    }

    FamStream* famConnect(uint32_t timeout_ms)
    {
        ClearError();

        if (!Ready())
        {
            return nullptr;
        }

        Famulus::Windows::NamedPipe pipe;
        const FamError connected =
            pipe.Connect(Famulus::Windows::NamedPipe::DefaultName(), std::chrono::milliseconds{timeout_ms});

        if (connected == FamError_NotFound)
        {
            SetError(connected, "no daemon is listening for this user");
            return nullptr;
        }

        if (connected == FamError_Timeout)
        {
            SetError(connected, "the endpoint is busy and did not answer in time");
            return nullptr;
        }

        if (connected != FamError_None)
        {
            SetError(connected, "cannot connect to the endpoint");
            return nullptr;
        }

        auto* stream = new (std::nothrow) FamStream{Context, std::move(pipe)};

        if (stream == nullptr)
        {
            SetError(FamError_System, "cannot allocate a stream");
            return nullptr;
        }

        Context->Track(stream);

        return stream;
    }

    bool famReadLine(FamStream* stream, const char** out_text, size_t* out_len)
    {
        ClearError();

        if (stream == nullptr || stream->Owner == nullptr)
        {
            SetError(FamError_Invalid, "no stream");
            return false;
        }

        if (out_text == nullptr || out_len == nullptr)
        {
            SetError(FamError_Invalid, "nowhere to put the line");
            return false;
        }

        *out_text = nullptr;
        *out_len = 0;

        if (!stream->Pipe.ReadLine(stream->Owner->LineScratch))
        {
            // A pipe that will not read has lost its peer: that is the ordinary
            // end of a client, not a fault in the platform.
            SetError(FamError_Closed, "the peer closed the connection");
            return false;
        }

        *out_len = stream->Owner->LineScratch.size();
        *out_text = stream->Owner->LineScratch.data();

        return true;
    }

    bool famWriteLine(FamStream* stream, const char* text, size_t length)
    {
        ClearError();

        if (stream == nullptr)
        {
            SetError(FamError_Invalid, "no stream");
            return false;
        }

        if (text == nullptr && length != 0)
        {
            SetError(FamError_Invalid, "no text with a non-zero length");
            return false;
        }

        if (!stream->Pipe.WriteLine(std::string_view(text, length)))
        {
            SetError(FamError_Closed, "the peer closed the connection");
            return false;
        }

        return true;
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

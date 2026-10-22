// ============================================================================
//  platform_api.cpp - every function declared in platform_api.h
//
//  Each function validates its arguments and makes one call on the object that
//  owns the behaviour: PlatformEnvironment, HotkeyWorker, SecretStore, all held
//  by the one context fam_platform_init creates.
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
    fam_status_t fam_platform_init(const struct fam_host* host)
    {
        if (host == nullptr || host->size < sizeof(struct fam_host) || host->post_event == nullptr)
        {
            return FAM_E_INVALID;
        }

        if (Context != nullptr)
        {
            return FAM_E_BUSY;
        }

        auto* created = new (std::nothrow) fam_context(host);

        if (created == nullptr)
        {
            return FAM_E_NOMEM;
        }

        if (!created->Environment.IsReady())
        {
            // A daemon that cannot create its own directories should find out at
            // startup, not at the first write.
            delete created;
            return FAM_E_IO;
        }

        Context = created;
        return FAM_OK;
    }

    void fam_platform_terminate(void)
    {
        delete Context;
        Context = nullptr;
    }

    uint32_t fam_platform_abi_version(void)
    {
        return FAM_PLATFORM_ABI_VERSION;
    }

    fam_capabilities_t fam_platform_capabilities(void)
    {
        if (Context == nullptr)
        {
            return 0;
        }

        return static_cast<fam_capabilities_t>(FAM_CAP_PATHS | FAM_CAP_HOTKEY | FAM_CAP_SECRET);
    }

    fam_status_t fam_platform_limits(struct fam_limits* out_limits)
    {
        if (Context == nullptr || out_limits == nullptr)
        {
            return FAM_E_INVALID;
        }

        out_limits->notification_max_actions = 0; /* no notification channel yet */
        out_limits->artifact_max_blocks = 256;
        out_limits->frame_max_bytes = 4u * 1024u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000; /* Task Scheduler granularity */

        return FAM_OK;
    }

    fam_status_t fam_platform_paths(enum fam_path_kind kind, char* buf, size_t* inout_len)
    {
        if (Context == nullptr || inout_len == nullptr)
        {
            return FAM_E_INVALID;
        }

        if (kind != FAM_PATH_CONFIG && kind != FAM_PATH_DATA && kind != FAM_PATH_CACHE && kind != FAM_PATH_STATE)
        {
            return FAM_E_INVALID;
        }

        if (!Context->Environment.EnsureDirectory(kind))
        {
            return FAM_E_IO;
        }

        const std::string path = Context->Environment.GetPathFor(kind);

        if (path.empty())
        {
            return FAM_E_IO;
        }

        const size_t needed = path.size();

        if (buf == nullptr || *inout_len < needed)
        {
            *inout_len = needed;
            return FAM_E_NOMEM;
        }

        path.copy(buf, needed);
        *inout_len = needed;

        return FAM_OK;
    }

    fam_status_t fam_platform_hotkey_bind(const char* accelerator, uint32_t* out_id)
    {
        if (Context == nullptr || accelerator == nullptr || out_id == nullptr)
        {
            return FAM_E_INVALID;
        }

        return Context->Hotkeys.Bind(accelerator, *out_id);
    }

    fam_status_t fam_platform_hotkey_unbind(uint32_t id)
    {
        if (Context == nullptr || id == 0)
        {
            return FAM_E_INVALID;
        }

        return Context->Hotkeys.Unbind(id);
    }

    fam_status_t fam_platform_secret_set(const char* name, const void* data, size_t size)
    {
        if (Context == nullptr || name == nullptr || (data == nullptr && size != 0))
        {
            return FAM_E_INVALID;
        }

        return Context->Secrets.Set(name, data, size);
    }

    fam_status_t fam_platform_secret_get(const char* name, unsigned char* buf, size_t* inout_len)
    {
        if (Context == nullptr || name == nullptr || inout_len == nullptr)
        {
            return FAM_E_INVALID;
        }

        return Context->Secrets.Get(name, buf, inout_len);
    }

    fam_status_t fam_platform_secret_delete(const char* name)
    {
        if (Context == nullptr || name == nullptr)
        {
            return FAM_E_INVALID;
        }

        return Context->Secrets.Delete(name);
    }

    int fam_platform_can_listen(void)
    {
        return Context != nullptr ? 1 : 0;
    }

    fam_status_t fam_platform_listen(struct fam_listener** out_listener)
    {
        if (Context == nullptr || out_listener == nullptr)
        {
            return FAM_E_INVALID;
        }

        auto* listener = new (std::nothrow) fam_listener{};

        if (listener == nullptr)
        {
            return FAM_E_NOMEM;
        }

        listener->Owner = Context;

        if (!listener->Current.Listen(Famulus::Windows::NamedPipe::DefaultName()))
        {
            delete listener;
            return FAM_E_BUSY;
        }

        Context->Track(listener);
        *out_listener = listener;

        return FAM_OK;
    }

    fam_status_t fam_platform_accept(struct fam_listener* listener, struct fam_stream** out_stream)
    {
        if (listener == nullptr || out_stream == nullptr || listener->Owner == nullptr)
        {
            return FAM_E_INVALID;
        }

        if (!listener->Current.Accept())
        {
            return FAM_E_IO;
        }

        // Put a fresh instance in place before anyone can observe a moment with no
        // endpoint at all: an instance is what makes a second daemon fail.
        Famulus::Windows::NamedPipe served = std::move(listener->Current);
        Famulus::Windows::NamedPipe replacement;

        if (!replacement.Open(served.GetName()))
        {
            return FAM_E_IO;
        }

        listener->Current = std::move(replacement);

        auto* stream = new (std::nothrow) fam_stream{listener->Owner, std::move(served)};

        if (stream == nullptr)
        {
            return FAM_E_NOMEM;
        }

        listener->Owner->Track(stream);
        *out_stream = stream;

        return FAM_OK;
    }

    fam_status_t fam_platform_connect(uint32_t timeout_ms, struct fam_stream** out_stream)
    {
        if (Context == nullptr || out_stream == nullptr)
        {
            return FAM_E_INVALID;
        }

        Famulus::Windows::NamedPipe pipe;

        if (!pipe.Connect(Famulus::Windows::NamedPipe::DefaultName(), std::chrono::milliseconds{timeout_ms}))
        {
            return FAM_E_DENIED;
        }

        auto* stream = new (std::nothrow) fam_stream{Context, std::move(pipe)};

        if (stream == nullptr)
        {
            return FAM_E_NOMEM;
        }

        Context->Track(stream);
        *out_stream = stream;

        return FAM_OK;
    }

    fam_status_t fam_platform_read_line(struct fam_stream* stream, char* buf, size_t* inout_len)
    {
        if (stream == nullptr || inout_len == nullptr)
        {
            return FAM_E_INVALID;
        }

        std::string line;

        if (!stream->Pipe.ReadLine(line))
        {
            return FAM_E_IO;
        }

        const size_t needed = line.size();

        if (buf == nullptr || *inout_len < needed)
        {
            *inout_len = needed;
            return FAM_E_NOMEM;
        }

        line.copy(buf, needed);
        *inout_len = needed;

        return FAM_OK;
    }

    fam_status_t fam_platform_write_line(struct fam_stream* stream, const char* text, size_t length)
    {
        if (stream == nullptr || (text == nullptr && length != 0))
        {
            return FAM_E_INVALID;
        }

        return stream->Pipe.WriteLine(std::string_view(text, length)) ? FAM_OK : FAM_E_IO;
    }

    void fam_platform_release_stream(struct fam_stream* stream)
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

    void fam_platform_release_listener(struct fam_listener* listener)
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

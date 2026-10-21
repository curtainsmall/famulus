// ============================================================================
//  famulus_platform.cpp - everything declared in famulus_platform.h
//
//  One translation unit for the whole ABI, so the mapping from the contract to
//  the implementation is readable in one place: each function validates its
//  arguments and then makes exactly one call on the object that owns the
//  behaviour (PlatformEnvironment, HotkeyWorker, SecretStore).
// ============================================================================

#include <string>

#include "hotkey_worker.hpp"
#include "platform_environment.hpp"
#include "secret_store.hpp"

extern "C"
{
    // --- identity and capabilities ----------------------------------------

    uint32_t fam_platform_abi_version(void)
    {
        return FAM_PLATFORM_ABI_VERSION;
    }

    fam_capabilities_t fam_platform_capabilities(void)
    {
        return static_cast<fam_capabilities_t>(FAM_CAP_PATHS | FAM_CAP_HOTKEY | FAM_CAP_SECRET);
    }

    fam_status_t fam_platform_limits(struct fam_limits* out_limits)
    {
        if (out_limits == nullptr)
        {
            return FAM_E_INVALID;
        }

        out_limits->notification_max_actions = 0; /* no notification channel yet */
        out_limits->artifact_max_blocks = 256;
        out_limits->frame_max_bytes = 4u * 1024u * 1024u;
        out_limits->schedule_min_granularity_ms = 60000; /* Task Scheduler granularity */

        return FAM_OK;
    }

    // --- lifetime ----------------------------------------------------------

    fam_status_t fam_platform_init(const struct fam_host* host)
    {
        if (host == nullptr || host->size < sizeof(struct fam_host) || host->post_event == nullptr)
        {
            return FAM_E_INVALID;
        }

        Famulus::Windows::PlatformEnvironment::GetInstance().SetHost(host);
        return FAM_OK;
    }

    void fam_platform_shutdown(void)
    {
        // The contract promises no event after this returns, so capabilities that
        // own OS resources release them before the host pointer goes away.
        Famulus::Windows::HotkeyWorker::GetInstance().Stop();
        Famulus::Windows::PlatformEnvironment::GetInstance().SetHost(nullptr);
    }

    // --- storage locations -------------------------------------------------

    fam_status_t fam_platform_paths(enum fam_path_kind kind, char* buf, size_t* inout_len)
    {
        if (inout_len == nullptr)
        {
            return FAM_E_INVALID;
        }

        if (kind != FAM_PATH_CONFIG && kind != FAM_PATH_DATA && kind != FAM_PATH_CACHE && kind != FAM_PATH_STATE)
        {
            return FAM_E_INVALID;
        }

        const std::string path = Famulus::Windows::PlatformEnvironment::GetInstance().GetPathFor(kind);

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

    // --- hotkeys -----------------------------------------------------------

    fam_status_t fam_platform_hotkey_bind(const char* accelerator, uint32_t* out_id)
    {
        if (accelerator == nullptr || out_id == nullptr)
        {
            return FAM_E_INVALID;
        }

        return Famulus::Windows::HotkeyWorker::GetInstance().Bind(accelerator, *out_id);
    }

    fam_status_t fam_platform_hotkey_unbind(uint32_t id)
    {
        if (id == 0)
        {
            return FAM_E_INVALID;
        }

        return Famulus::Windows::HotkeyWorker::GetInstance().Unbind(id);
    }

    // --- secrets -----------------------------------------------------------

    fam_status_t fam_platform_secret_set(const char* name, const void* data, size_t size)
    {
        if (name == nullptr || (data == nullptr && size != 0))
        {
            return FAM_E_INVALID;
        }

        return Famulus::Windows::SecretStore::GetInstance().Set(name, data, size);
    }

    fam_status_t fam_platform_secret_get(const char* name, unsigned char* buf, size_t* inout_len)
    {
        if (name == nullptr || inout_len == nullptr)
        {
            return FAM_E_INVALID;
        }

        return Famulus::Windows::SecretStore::GetInstance().Get(name, buf, inout_len);
    }

    fam_status_t fam_platform_secret_delete(const char* name)
    {
        if (name == nullptr)
        {
            return FAM_E_INVALID;
        }

        return Famulus::Windows::SecretStore::GetInstance().Delete(name);
    }
}

// ============================================================================
//  src/platform/windows/platform.cpp - the Windows adapter
//
//  Capabilities only, so far: the ABI, its limits and the storage locations.
//  Hotkey, notification, scheduler, secrets and the named-pipe transport land in
//  later milestones; every one of them will register itself here rather than
//  grow a second file, because they share exactly this state (the host table).
//
//  Style note: the exported functions and the tagged types they use belong to
//  the C ABI; everything else on this side of the boundary follows the
//  project's C#-influenced C++ style.
// ============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstddef>
#include <string>

#include "famulus_platform.h"

namespace
{
    // Held for the first capability that needs to post an event; until then
    // init does nothing but validate it.
    const struct fam_host* Host = nullptr;

    constexpr wchar_t AppDirectoryName[] = L"famulus";

    // Storage roots are platform conventions, never guesses: a sandboxed or
    // roaming build moves them, and the daemon must not care.
    auto RootVariableFor(enum fam_path_kind kind) -> const wchar_t*
    {
        switch (kind)
        {
        case FAM_PATH_CONFIG:
            return L"APPDATA";
        case FAM_PATH_DATA:
        case FAM_PATH_CACHE:
        case FAM_PATH_STATE:
            return L"LOCALAPPDATA";
        }

        return nullptr;
    }

    auto EnvironmentVariable(const wchar_t* name) -> std::wstring
    {
        const DWORD needed = ::GetEnvironmentVariableW(name, nullptr, 0);

        if (needed == 0)
        {
            return {};
        }

        std::wstring value(needed, L'\0');
        const DWORD written = ::GetEnvironmentVariableW(name, value.data(), needed);

        if (written == 0 || written >= needed)
        {
            return {};
        }

        value.resize(written);
        return value;
    }

    auto PathFor(enum fam_path_kind kind, std::wstring& out) -> fam_status_t
    {
        const wchar_t* variable = RootVariableFor(kind);

        if (variable == nullptr)
        {
            return FAM_E_INVALID;
        }

        std::wstring path = EnvironmentVariable(variable);

        if (path.empty())
        {
            return FAM_E_DENIED;
        }

        if (path.back() != L'\\')
        {
            path.push_back(L'\\');
        }

        path.append(AppDirectoryName);

        if (::CreateDirectoryW(path.c_str(), nullptr) == 0)
        {
            const DWORD error = ::GetLastError();

            if (error != ERROR_ALREADY_EXISTS)
            {
                return FAM_E_IO;
            }
        }

        out = path;
        return FAM_OK;
    }

    // Two-pass UTF-8 conversion into a caller buffer: capacity in, required
    // length out, FAM_E_NOMEM when the capacity was short.
    auto CopyUtf8(const std::wstring& text, char* buf, size_t* inout_len) -> fam_status_t
    {
        if (inout_len == nullptr)
        {
            return FAM_E_INVALID;
        }

        const int required = ::WideCharToMultiByte(CP_UTF8,
                                                  0,
                                                  text.c_str(),
                                                  static_cast<int>(text.size()),
                                                  nullptr,
                                                  0,
                                                  nullptr,
                                                  nullptr);

        if (required <= 0)
        {
            return FAM_E_IO;
        }

        const size_t needed = static_cast<size_t>(required);

        if (buf == nullptr || *inout_len < needed)
        {
            *inout_len = needed;
            return FAM_E_NOMEM;
        }

        ::WideCharToMultiByte(CP_UTF8,
                              0,
                              text.c_str(),
                              static_cast<int>(text.size()),
                              buf,
                              required,
                              nullptr,
                              nullptr);

        *inout_len = needed;
        return FAM_OK;
    }
} // namespace

extern "C"
{
    uint32_t fam_platform_abi_version(void)
    {
        return FAM_PLATFORM_ABI_VERSION;
    }

    fam_capabilities_t fam_platform_capabilities(void)
    {
        return FAM_CAP_PATHS;
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

    fam_status_t fam_platform_init(const struct fam_host* host)
    {
        if (host == nullptr || host->size < sizeof(struct fam_host) || host->post_event == nullptr)
        {
            return FAM_E_INVALID;
        }

        Host = host;
        return FAM_OK;
    }

    void fam_platform_shutdown(void)
    {
        Host = nullptr;
    }

    fam_status_t fam_platform_paths(enum fam_path_kind kind, char* buf, size_t* inout_len)
    {
        std::wstring path;

        const fam_status_t status = PathFor(kind, path);

        if (status != FAM_OK)
        {
            return status;
        }

        return CopyUtf8(path, buf, inout_len);
    }
}

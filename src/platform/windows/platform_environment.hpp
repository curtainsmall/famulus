// ============================================================================
//  platform_environment.hpp - what every capability needs from the daemon and
//  from Win32
//
//  Owns the host table the daemon lent at init. The constructor is where the
//  creating happens: it ensures the directories the daemon writes to, and
//  records whether it worked. It cannot report anything - there is no host yet -
//  so fam_platform_init turns that record into a status code.
// ============================================================================

#pragma once

#include <string>
#include <string_view>

#include "famulus_platform.h"

namespace Famulus::Windows
{
    class PlatformEnvironment
    {
    public:
        PlatformEnvironment();
        ~PlatformEnvironment();

        PlatformEnvironment(const PlatformEnvironment&) = delete;
        auto operator=(const PlatformEnvironment&) -> PlatformEnvironment& = delete;

        static auto GetInstance() -> PlatformEnvironment&;

        // The host table, or nullptr before init and after shutdown. Borrowed for
        // the lifetime of the process.
        auto GetHost() const -> const struct fam_host*;
        void SetHost(const struct fam_host* host);

        void Log(int level, const char* category, const char* message) const;

        // Storage root for a path kind, no trailing separator. Empty when the
        // platform refused, which callers treat as "store nothing". GetPathFor
        // returns UTF-8 for the ABI, GetWidePathFor the native form for Win32
        // calls. Neither creates anything: that is EnsureDirectory's job.
        auto GetPathFor(enum fam_path_kind kind) const -> std::string;
        auto GetWidePathFor(enum fam_path_kind kind) const -> std::wstring;

        // Creates the directory if it is missing. True when it exists afterwards.
        auto EnsureDirectory(enum fam_path_kind kind) const -> bool;

        // Whether the constructor managed to create every directory.
        auto IsReady() const -> bool;

        // UTF-8 -> UTF-16 through MultiByteToWideChar.
        static auto ConvertToWide(std::string_view text) -> std::wstring;

    private:
        const struct fam_host* HostTable = nullptr;
        bool Ready = false;
    };
}

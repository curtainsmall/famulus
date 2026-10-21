// ============================================================================
//  PlatformEnvironment.hpp - what every capability needs from the daemon and
//  from Win32
//
//  Owns the host table the daemon lent at init, and the two conversions that
//  would otherwise be repeated in every capability file.
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
        // The host table, or nullptr before init and after shutdown. Borrowed for
        // the lifetime of the process.
        auto Host() const -> const struct fam_host*;

        void Attach(const struct fam_host* host);
        void Detach();

        void Log(int level, const char* category, const char* message) const;

        // Storage root for a path kind, no trailing separator. Empty when the
        // platform refused, which callers treat as "store nothing". PathFor gives
        // UTF-8 for the ABI, WidePathFor the native form for Win32 calls.
        auto PathFor(enum fam_path_kind kind) const -> std::string;
        auto WidePathFor(enum fam_path_kind kind) const -> std::wstring;

        // UTF-8 -> UTF-16 through MultiByteToWideChar.
        static auto ToWide(std::string_view text) -> std::wstring;

    private:
        const struct fam_host* HostTable = nullptr;
    };

    // The adapter has exactly one environment; capabilities reach it through
    // this rather than through a global.
    auto Environment() -> PlatformEnvironment&;
}

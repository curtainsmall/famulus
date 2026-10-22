// ============================================================================
//  platform_environment.hpp - what every capability needs from the daemon and
//  from Win32
//
//  Owns the host table the daemon lent at creation. The constructor is where the
//  creating happens: it ensures the directories the daemon writes to and records
//  whether it worked, because it has no host to report to yet.
// ============================================================================

#pragma once

#include <string>
#include <string_view>

#include "platform_api.h"

namespace Famulus::Windows
{
    class PlatformEnvironment
    {
    public:
        PlatformEnvironment();
        ~PlatformEnvironment();

        PlatformEnvironment(const PlatformEnvironment&) = delete;
        auto operator=(const PlatformEnvironment&) -> PlatformEnvironment& = delete;

        // The host table, or nullptr before it is attached and after detaching.
        auto GetHost() const -> const FamHost*;
        void SetHost(const FamHost* host);

        void Log(int level, const char* category, const char* message) const;

        // Storage root for a path kind, no trailing separator. Empty when the
        // platform refused, which callers treat as "store nothing". GetPathFor
        // returns UTF-8 for the ABI, GetWidePathFor the native form for Win32
        // calls. Neither creates anything: that is EnsureDirectory's job.
        auto GetPathFor(FamPathKind kind) const -> std::string;
        auto GetWidePathFor(FamPathKind kind) const -> std::wstring;

        // The same path, borrowed: valid until the next call, which is what lets
        // the C API hand out a string without asking the caller for a buffer.
        auto GetPathView(FamPathKind kind) -> const char*;

        // Creates the directory if it is missing. True when it exists afterwards.
        auto EnsureDirectory(FamPathKind kind) const -> bool;

        // Whether the constructor managed to create every directory.
        auto IsReady() const -> bool;

        // UTF-8 -> UTF-16 through MultiByteToWideChar.
        static auto ConvertToWide(std::string_view text) -> std::wstring;

    private:
        const FamHost* HostTable = nullptr;
        bool Ready = false;
        std::string PathScratch;
    };
}

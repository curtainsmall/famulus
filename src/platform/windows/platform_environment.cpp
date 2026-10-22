// ============================================================================
//  platform_environment.cpp - host table, logging, storage roots, encoding
//
//  Encoding conversion is MultiByteToWideChar / WideCharToMultiByte: the OS
//  already has a correct, fast implementation that handles invalid input, so
//  there is nothing to hand write here.
// ============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstddef>

#include "platform_environment.hpp"

namespace
{
    constexpr wchar_t AppDirectoryName[] = L"famulus";

    constexpr FamPathKind AllPathKinds[] = {FamPathKind_Data, FamPathKind_Config, FamPathKind_Cache, FamPathKind_State};

    // Storage roots are platform conventions, never guesses: a sandboxed or
    // roaming build moves them, and the daemon must not care.
    auto RootVariableFor(FamPathKind kind) -> const wchar_t*
    {
        switch (kind)
        {
        case FamPathKind_Config:
            return L"APPDATA";
        case FamPathKind_Data:
        case FamPathKind_Cache:
        case FamPathKind_State:
            return L"LOCALAPPDATA";
        }

        return nullptr;
    }

    auto KindName(FamPathKind kind) -> const wchar_t*
    {
        switch (kind)
        {
        case FamPathKind_Config:
            return L"config";
        case FamPathKind_Cache:
            return L"cache";
        case FamPathKind_State:
            return L"state";
        case FamPathKind_Data:
            return L"data";
        }

        return L"data";
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
} // namespace

namespace Famulus::Windows
{
    PlatformEnvironment::PlatformEnvironment()
    {
        // Creating here rather than on first use: a daemon that cannot create its
        // own directory should find out at startup, not halfway through a session.
        // The result is kept, because there is nobody to report to yet.
        bool ready = true;

        for (const FamPathKind kind : AllPathKinds)
        {
            ready = EnsureDirectory(kind) && ready;
        }

        Ready = ready;
    }

    PlatformEnvironment::~PlatformEnvironment() = default;

    auto PlatformEnvironment::GetHost() const -> const FamHost*
    {
        return HostTable;
    }

    void PlatformEnvironment::SetHost(const FamHost* host)
    {
        HostTable = host;
    }

    void PlatformEnvironment::Log(FamLogLevel level, const char* category, const char* message) const
    {
        if (HostTable != nullptr && HostTable->log != nullptr)
        {
            HostTable->log(level, category, message);
        }
    }

    auto PlatformEnvironment::IsReady() const -> bool
    {
        return Ready;
    }

    auto PlatformEnvironment::ConvertToWide(std::string_view text) -> std::wstring
    {
        if (text.empty())
        {
            return {};
        }

        const int needed =
            ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);

        if (needed <= 0)
        {
            return {};
        }

        std::wstring wide(static_cast<size_t>(needed), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), needed);

        return wide;
    }

    auto PlatformEnvironment::EnsureDirectory(FamPathKind kind) const -> bool
    {
        const std::wstring path = GetWidePathFor(kind);

        if (path.empty())
        {
            return false;
        }

        if (::CreateDirectoryW(path.c_str(), nullptr) == 0)
        {
            return ::GetLastError() == ERROR_ALREADY_EXISTS;
        }

        return true;
    }

    auto PlatformEnvironment::GetWidePathFor(FamPathKind kind) const -> std::wstring
    {
        const wchar_t* variable = RootVariableFor(kind);

        if (variable == nullptr)
        {
            return {};
        }

        std::wstring path = EnvironmentVariable(variable);

        if (path.empty())
        {
            return {};
        }

        if (path.back() != L'\\')
        {
            path.push_back(L'\\');
        }

        path.append(AppDirectoryName);

        // One subdirectory per kind, so the four roots the ABI promises are four
        // directories here too instead of all landing in the same place.
        path.push_back(L'\\');
        path.append(KindName(kind));

        return path;
    }

    auto PlatformEnvironment::GetPathFor(FamPathKind kind) const -> std::string
    {
        const std::wstring wide = GetWidePathFor(kind);

        if (wide.empty())
        {
            return {};
        }

        const int needed = ::WideCharToMultiByte(
            CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);

        if (needed <= 0)
        {
            return {};
        }

        std::string narrow(static_cast<size_t>(needed), '\0');
        ::WideCharToMultiByte(
            CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), narrow.data(), needed, nullptr, nullptr);

        return narrow;
    }

    auto PlatformEnvironment::GetPathView(FamPathKind kind) -> const char*
    {
        PathScratch = GetPathFor(kind);

        return PathScratch.empty() ? nullptr : PathScratch.data();
    }
} // namespace Famulus::Windows

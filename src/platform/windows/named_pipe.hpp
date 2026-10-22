// ============================================================================
//  named_pipe.hpp - the Windows endpoint behind base/local_transport.hpp
//
//  A file handle is a resource with a lifetime, so it lives in a class: Listen
//  and Open attach the handle, the destructor closes it, and moving transfers
//  ownership of that handle.
//
//  The exclusive instance created by Listen is never connected. It is the
//  single-instance lock, and it must outlive the connections that follow:
//  otherwise the lock would disappear between two clients.
// ============================================================================

#pragma once

#include <chrono>
#include <string>
#include <string_view>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "platform_api.h"

namespace Famulus::Windows
{
    class NamedPipe
    {
    public:
        NamedPipe() = default;
        ~NamedPipe();

        NamedPipe(const NamedPipe&) = delete;
        auto operator=(const NamedPipe&) -> NamedPipe& = delete;

        NamedPipe(NamedPipe&& other) noexcept;
        auto operator=(NamedPipe&& other) noexcept -> NamedPipe&;

        // \\.\pipe\famulus-<user>. Derived from the account, not the machine, so
        // two users on one box do not share a daemon.
        static auto DefaultName() -> std::wstring;

        // Server side: the exclusive instance, held for the process lifetime.
        // False when another daemon already holds it.
        auto Listen(const std::wstring& name) -> bool;

        // Server side: one additional instance for the next client.
        auto Open(const std::wstring& name) -> bool;

        // Waits for a client to connect to the current instance.
        auto Accept() -> bool;

        // Client side.
        // Why the connection attempt failed: NotFound when nothing ever listened
        // under that name, Timeout when something holds it but none would serve
        // within the deadline, System for anything else.
        auto Connect(const std::wstring& name, std::chrono::milliseconds timeout) -> FamError;

        // One message, without its terminator. Read returns false on disconnect.
        auto ReadLine(std::string& line) -> bool;
        auto WriteLine(std::string_view line) -> bool;

        auto GetName() const -> const std::wstring&;
        auto Close() -> void;

    private:
        auto CreateInstance(unsigned long flags) -> bool;

        HANDLE Handle = INVALID_HANDLE_VALUE;
        std::wstring Name;
    };
}

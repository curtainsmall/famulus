// ============================================================================
//  named_pipe.cpp - the local transport between famulusd and its clients
//
//  Blocking I/O on the calling thread: one daemon serves one client at a time,
//  which is enough for a status line and keeps the code small. Overlapped I/O
//  becomes worth it when a client can be slow without blocking others.
// ============================================================================

#include "named_pipe.hpp"

#include <vector>

namespace
{
    constexpr DWORD BufferSize = 4096;
    constexpr DWORD ConnectTimeoutMs = 1000;

    auto UserName() -> std::wstring
    {
        wchar_t buffer[256]{};
        DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));

        if (::GetUserNameW(buffer, &size) == 0)
        {
            return L"user";
        }

        std::wstring name(buffer, size > 0 ? size - 1 : 0);

        for (wchar_t& character : name)
        {
            const bool allowed = (character >= L'a' && character <= L'z') ||
                                 (character >= L'A' && character <= L'Z') ||
                                 (character >= L'0' && character <= L'9') || character == L'.' || character == L'_' ||
                                 character == L'-';

            if (!allowed)
            {
                character = L'_';
            }
        }

        return name;
    }
} // namespace

namespace Famulus::Windows
{
    NamedPipe::~NamedPipe()
    {
        Close();
    }

    NamedPipe::NamedPipe(NamedPipe&& other) noexcept
        : Handle(other.Handle),
          Name(std::move(other.Name))
    {
        other.Handle = INVALID_HANDLE_VALUE;
    }

    auto NamedPipe::operator=(NamedPipe&& other) noexcept -> NamedPipe&
    {
        if (this != &other)
        {
            Close();
            Handle = other.Handle;
            Name = std::move(other.Name);
            other.Handle = INVALID_HANDLE_VALUE;
        }

        return *this;
    }

    auto NamedPipe::DefaultName() -> std::wstring
    {
        return L"\\\\.\\pipe\\famulus-" + UserName();
    }

    auto NamedPipe::GetName() const -> const std::wstring&
    {
        return Name;
    }

    auto NamedPipe::CreateInstance(unsigned long flags) -> bool
    {
        Close();

        Handle = ::CreateNamedPipeW(Name.c_str(),
                                    PIPE_ACCESS_DUPLEX | static_cast<DWORD>(flags),
                                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                    PIPE_UNLIMITED_INSTANCES,
                                    BufferSize,
                                    BufferSize,
                                    ConnectTimeoutMs,
                                    nullptr);

        return Handle != INVALID_HANDLE_VALUE;
    }

    auto NamedPipe::Listen(const std::wstring& name) -> bool
    {
        Close();
        Name = name;

        // Never connected: this instance is the lock, not a channel.
        return CreateInstance(FILE_FLAG_FIRST_PIPE_INSTANCE);
    }

    auto NamedPipe::Open(const std::wstring& name) -> bool
    {
        Close();
        Name = name;

        return CreateInstance(0);
    }

    auto NamedPipe::Accept() -> bool
    {
        if (Handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        if (::ConnectNamedPipe(Handle, nullptr) != 0)
        {
            return true;
        }

        return ::GetLastError() == ERROR_PIPE_CONNECTED;
    }

    auto NamedPipe::Connect(const std::wstring& name, std::chrono::milliseconds timeout) -> FamError
    {
        Close();
        Name = name;

        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (true)
        {
            Handle = ::CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

            if (Handle != INVALID_HANDLE_VALUE)
            {
                return FamError_None;
            }

            const DWORD error = ::GetLastError();

            // Nothing of that name exists: no daemon has ever listened here, and
            // waiting would only delay the answer.
            if (error == ERROR_FILE_NOT_FOUND)
            {
                return FamError_NotFound;
            }

            if (error != ERROR_PIPE_BUSY)
            {
                return FamError_System;
            }

            if (std::chrono::steady_clock::now() >= deadline)
            {
                return FamError_Timeout;
            }

            ::WaitNamedPipeW(name.c_str(), ConnectTimeoutMs);
        }
    }

    auto NamedPipe::ReadLine(std::string& line) -> bool
    {
        line.clear();

        if (Handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        char buffer[256]{};

        while (true)
        {
            DWORD read = 0;

            if (::ReadFile(Handle, buffer, sizeof(buffer), &read, nullptr) == 0 || read == 0)
            {
                return false; // client disconnected
            }

            for (DWORD i = 0; i < read; ++i)
            {
                if (buffer[i] == '\n')
                {
                    line.append(buffer, i);
                    return true;
                }
            }

            line.append(buffer, read);

            if (line.size() > BufferSize * 16)
            {
                return false; // a message this long is not a request
            }
        }
    }

    auto NamedPipe::WriteLine(std::string_view line) -> bool
    {
        if (Handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        std::string message{line};
        message.push_back('\n');

        DWORD written = 0;

        return ::WriteFile(Handle, message.data(), static_cast<DWORD>(message.size()), &written, nullptr) != 0 &&
               written == message.size();
    }

    void NamedPipe::Close()
    {
        if (Handle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        ::FlushFileBuffers(Handle);
        ::DisconnectNamedPipe(Handle);
        ::CloseHandle(Handle);
        Handle = INVALID_HANDLE_VALUE;
    }
}

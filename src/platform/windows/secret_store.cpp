// ============================================================================
//  SecretStore.cpp - DPAPI sealed files
//
//  A stolen file without the user's logon context is not readable, and the
//  daemon never sees a path.
// ============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <dpapi.h>
#include <wincrypt.h>

#include <cstdint>
#include <cstring>

#include "platform_environment.hpp"
#include "secret_store.hpp"

namespace
{
    constexpr wchar_t SecretsFolder[] = L"secrets";
    constexpr size_t MaxNameLength = 64;
    constexpr size_t MaxSecretBytes = 64 * 1024;
} // namespace

namespace Famulus::Windows
{
    SecretStore::SecretStore(PlatformEnvironment& environment)
        : Environment(environment)
    {
        // Nothing to create yet: the data directory belongs to the environment,
        // and the secrets subdirectory is ensured on first use, where a failure
        // can be reported to the host.
    }

    auto SecretStore::Set(std::string_view name, const void* data, size_t size) -> FamStatus
    {
        const std::wstring path = GetFileFor(name);

        if (path.empty())
        {
            return FamStatus_Invalid;
        }

        std::vector<unsigned char> sealed;

        if (Protect(static_cast<const unsigned char*>(data), size, sealed) != FamStatus_Ok)
        {
            return FamStatus_Io;
        }

        return WriteAll(path, sealed.data(), sealed.size());
    }

    auto SecretStore::Get(std::string_view name, unsigned char* buf, size_t* inout_len) -> FamStatus
    {
        if (inout_len == nullptr)
        {
            return FamStatus_Invalid;
        }

        const std::wstring path = GetFileFor(name);

        if (path.empty())
        {
            return FamStatus_Invalid;
        }

        std::vector<unsigned char> sealed;
        const FamStatus read = ReadAll(path, sealed);

        if (read != FamStatus_Ok)
        {
            return read;
        }

        std::vector<unsigned char> plain;

        if (Unprotect(sealed, plain) != FamStatus_Ok)
        {
            return FamStatus_Io;
        }

        if (buf == nullptr || *inout_len < plain.size())
        {
            *inout_len = plain.size();
            return FamStatus_NoMem;
        }

        if (!plain.empty())
        {
            ::memcpy(buf, plain.data(), plain.size());
        }

        *inout_len = plain.size();

        return FamStatus_Ok;
    }

    auto SecretStore::GetView(std::string_view name, const unsigned char** out_data, size_t* out_len) -> FamStatus
    {
        if (out_data == nullptr || out_len == nullptr)
        {
            return FamStatus_Invalid;
        }

        size_t capacity = 0;
        const FamStatus probe = Get(name, nullptr, &capacity);

        if (probe != FamStatus_NoMem && probe != FamStatus_Ok)
        {
            return probe;
        }

        Scratch.assign(capacity, 0);

        if (capacity != 0 && Get(name, Scratch.data(), &capacity) != FamStatus_Ok)
        {
            return FamStatus_Io;
        }

        *out_len = capacity;
        *out_data = Scratch.empty() ? nullptr : Scratch.data();

        return FamStatus_Ok;
    }

    auto SecretStore::Delete(std::string_view name) -> FamStatus
    {
        const std::wstring path = GetFileFor(name);

        if (path.empty())
        {
            return FamStatus_Invalid;
        }

        if (::DeleteFileW(path.c_str()) == 0)
        {
            return ::GetLastError() == ERROR_FILE_NOT_FOUND ? FamStatus_Denied : FamStatus_Io;
        }

        return FamStatus_Ok;
    }

    auto SecretStore::IsValidName(std::string_view name) -> bool
    {
        if (name.empty() || name.size() > MaxNameLength || name == "." || name == "..")
        {
            return false;
        }

        for (const char character : name)
        {
            const bool allowed = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                                 (character >= '0' && character <= '9') || character == '.' || character == '_' ||
                                 character == '-';

            if (!allowed)
            {
                return false;
            }
        }

        return true;
    }

    auto SecretStore::EnsureSecretsDirectory() const -> std::wstring
    {
        if (!Environment.EnsureDirectory(FamPathKind_Data))
        {
            Environment.Log(FamLogLevel_Error, "secret", "cannot create the data directory");
            return {};
        }

        const std::wstring data_directory = Environment.GetWidePathFor(FamPathKind_Data);

        if (data_directory.empty())
        {
            return {};
        }

        std::wstring directory = data_directory;
        directory.push_back(L'\\');
        directory.append(SecretsFolder);

        if (::CreateDirectoryW(directory.c_str(), nullptr) == 0 && ::GetLastError() != ERROR_ALREADY_EXISTS)
        {
            Environment.Log(FamLogLevel_Error, "secret", "cannot create the secrets directory");
            return {};
        }

        return directory;
    }

    auto SecretStore::GetFileFor(std::string_view name) const -> std::wstring
    {
        if (!IsValidName(name))
        {
            return {};
        }

        const std::wstring directory = EnsureSecretsDirectory();

        if (directory.empty())
        {
            return {};
        }

        std::wstring file = directory;
        file.push_back(L'\\');
        file.append(PlatformEnvironment::ConvertToWide(name));
        file.append(L".bin");

        return file;
    }

    auto SecretStore::ReadAll(const std::wstring& path, std::vector<unsigned char>& out) -> FamStatus
    {
        const HANDLE handle =
            ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            return ::GetLastError() == ERROR_FILE_NOT_FOUND ? FamStatus_Denied : FamStatus_Io;
        }

        LARGE_INTEGER size{};

        if (::GetFileSizeEx(handle, &size) == 0 || size.QuadPart < 0 ||
            size.QuadPart > static_cast<LONGLONG>(MaxSecretBytes))
        {
            ::CloseHandle(handle);
            return FamStatus_Io;
        }

        out.resize(static_cast<size_t>(size.QuadPart));

        if (out.empty())
        {
            ::CloseHandle(handle);
            return FamStatus_Ok;
        }

        DWORD read = 0;
        const BOOL ok = ::ReadFile(handle, out.data(), static_cast<DWORD>(out.size()), &read, nullptr);
        ::CloseHandle(handle);

        if (ok == 0 || read != out.size())
        {
            return FamStatus_Io;
        }

        return FamStatus_Ok;
    }

    auto SecretStore::WriteAll(const std::wstring& path, const unsigned char* data, size_t size) -> FamStatus
    {
        const HANDLE handle =
            ::CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            return FamStatus_Io;
        }

        DWORD written = 0;
        const BOOL ok = size == 0 || ::WriteFile(handle, data, static_cast<DWORD>(size), &written, nullptr) != 0;
        ::CloseHandle(handle);

        if (ok == 0 || written != size)
        {
            return FamStatus_Io;
        }

        return FamStatus_Ok;
    }

    auto SecretStore::Protect(const unsigned char* data, size_t size, std::vector<unsigned char>& out) -> FamStatus
    {
        if (size > 0xFFFFFFFFu)
        {
            return FamStatus_Invalid;
        }

        DATA_BLOB input{static_cast<DWORD>(size), const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data))};
        DATA_BLOB output{};

        if (::CryptProtectData(&input, L"famulus", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output) == 0)
        {
            return FamStatus_Io;
        }

        out.assign(output.pbData, output.pbData + output.cbData);
        ::LocalFree(output.pbData);

        return FamStatus_Ok;
    }

    auto SecretStore::Unprotect(const std::vector<unsigned char>& data, std::vector<unsigned char>& out) -> FamStatus
    {
        if (data.empty() || data.size() > 0xFFFFFFFFu)
        {
            return FamStatus_Io;
        }

        DATA_BLOB input{static_cast<DWORD>(data.size()), const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data.data()))};
        DATA_BLOB output{};

        if (::CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output) == 0)
        {
            return FamStatus_Io;
        }

        out.assign(output.pbData, output.pbData + output.cbData);
        ::LocalFree(output.pbData);

        return FamStatus_Ok;
    }
} // namespace Famulus::Windows

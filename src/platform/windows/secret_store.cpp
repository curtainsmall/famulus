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

    auto SecretStore::Set(std::string_view name, const void* data, size_t size) -> fam_status_t
    {
        const std::wstring path = GetFileFor(name);

        if (path.empty())
        {
            return FAM_E_INVALID;
        }

        std::vector<unsigned char> sealed;

        if (Protect(static_cast<const unsigned char*>(data), size, sealed) != FAM_OK)
        {
            return FAM_E_IO;
        }

        return WriteAll(path, sealed.data(), sealed.size());
    }

    auto SecretStore::Get(std::string_view name, unsigned char* buf, size_t* inout_len) -> fam_status_t
    {
        if (inout_len == nullptr)
        {
            return FAM_E_INVALID;
        }

        const std::wstring path = GetFileFor(name);

        if (path.empty())
        {
            return FAM_E_INVALID;
        }

        std::vector<unsigned char> sealed;
        const fam_status_t read = ReadAll(path, sealed);

        if (read != FAM_OK)
        {
            return read;
        }

        std::vector<unsigned char> plain;

        if (Unprotect(sealed, plain) != FAM_OK)
        {
            return FAM_E_IO;
        }

        if (buf == nullptr || *inout_len < plain.size())
        {
            *inout_len = plain.size();
            return FAM_E_NOMEM;
        }

        if (!plain.empty())
        {
            ::memcpy(buf, plain.data(), plain.size());
        }

        *inout_len = plain.size();

        return FAM_OK;
    }

    auto SecretStore::Delete(std::string_view name) -> fam_status_t
    {
        const std::wstring path = GetFileFor(name);

        if (path.empty())
        {
            return FAM_E_INVALID;
        }

        if (::DeleteFileW(path.c_str()) == 0)
        {
            return ::GetLastError() == ERROR_FILE_NOT_FOUND ? FAM_E_DENIED : FAM_E_IO;
        }

        return FAM_OK;
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
        if (!Environment.EnsureDirectory(FAM_PATH_DATA))
        {
            Environment.Log(3, "secret", "cannot create the data directory");
            return {};
        }

        const std::wstring data_directory = Environment.GetWidePathFor(FAM_PATH_DATA);

        if (data_directory.empty())
        {
            return {};
        }

        std::wstring directory = data_directory;
        directory.push_back(L'\\');
        directory.append(SecretsFolder);

        if (::CreateDirectoryW(directory.c_str(), nullptr) == 0 && ::GetLastError() != ERROR_ALREADY_EXISTS)
        {
            Environment.Log(3, "secret", "cannot create the secrets directory");
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

    auto SecretStore::ReadAll(const std::wstring& path, std::vector<unsigned char>& out) -> fam_status_t
    {
        const HANDLE handle =
            ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            return ::GetLastError() == ERROR_FILE_NOT_FOUND ? FAM_E_DENIED : FAM_E_IO;
        }

        LARGE_INTEGER size{};

        if (::GetFileSizeEx(handle, &size) == 0 || size.QuadPart < 0 ||
            size.QuadPart > static_cast<LONGLONG>(MaxSecretBytes))
        {
            ::CloseHandle(handle);
            return FAM_E_IO;
        }

        out.resize(static_cast<size_t>(size.QuadPart));

        if (out.empty())
        {
            ::CloseHandle(handle);
            return FAM_OK;
        }

        DWORD read = 0;
        const BOOL ok = ::ReadFile(handle, out.data(), static_cast<DWORD>(out.size()), &read, nullptr);
        ::CloseHandle(handle);

        if (ok == 0 || read != out.size())
        {
            return FAM_E_IO;
        }

        return FAM_OK;
    }

    auto SecretStore::WriteAll(const std::wstring& path, const unsigned char* data, size_t size) -> fam_status_t
    {
        const HANDLE handle =
            ::CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            return FAM_E_IO;
        }

        DWORD written = 0;
        const BOOL ok = size == 0 || ::WriteFile(handle, data, static_cast<DWORD>(size), &written, nullptr) != 0;
        ::CloseHandle(handle);

        if (ok == 0 || written != size)
        {
            return FAM_E_IO;
        }

        return FAM_OK;
    }

    auto SecretStore::Protect(const unsigned char* data, size_t size, std::vector<unsigned char>& out) -> fam_status_t
    {
        if (size > 0xFFFFFFFFu)
        {
            return FAM_E_INVALID;
        }

        DATA_BLOB input{static_cast<DWORD>(size), const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data))};
        DATA_BLOB output{};

        if (::CryptProtectData(&input, L"famulus", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output) == 0)
        {
            return FAM_E_IO;
        }

        out.assign(output.pbData, output.pbData + output.cbData);
        ::LocalFree(output.pbData);

        return FAM_OK;
    }

    auto SecretStore::Unprotect(const std::vector<unsigned char>& data, std::vector<unsigned char>& out) -> fam_status_t
    {
        if (data.empty() || data.size() > 0xFFFFFFFFu)
        {
            return FAM_E_IO;
        }

        DATA_BLOB input{static_cast<DWORD>(data.size()), const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data.data()))};
        DATA_BLOB output{};

        if (::CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output) == 0)
        {
            return FAM_E_IO;
        }

        out.assign(output.pbData, output.pbData + output.cbData);
        ::LocalFree(output.pbData);

        return FAM_OK;
    }
} // namespace Famulus::Windows

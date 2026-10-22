// ============================================================================
//  secret_store.hpp - secrets encrypted at rest under the current user
//
//  One file per secret under the data directory, sealed with DPAPI. The daemon
//  hands over opaque bytes and a name; it never learns where they live.
// ============================================================================

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "platform_api.h"

namespace Famulus::Windows
{
    class PlatformEnvironment;

    class SecretStore
    {
    public:
        explicit SecretStore(PlatformEnvironment& environment);

        // A name is 1..64 characters of [A-Za-z0-9._-]. It is validated here
        // because in this implementation a name is a path component: the
        // boundary's job is to refuse the locator and accept only the data.
        auto Set(std::string_view name, const void* data, size_t size) -> fam_status_t;
        auto Get(std::string_view name, unsigned char* buf, size_t* inout_len) -> fam_status_t;
        auto Delete(std::string_view name) -> fam_status_t;

    private:
        static auto IsValidName(std::string_view name) -> bool;
        auto EnsureSecretsDirectory() const -> std::wstring;
        auto GetFileFor(std::string_view name) const -> std::wstring;
        static auto ReadAll(const std::wstring& path, std::vector<unsigned char>& out) -> fam_status_t;
        static auto WriteAll(const std::wstring& path, const unsigned char* data, size_t size) -> fam_status_t;
        static auto Protect(const unsigned char* data, size_t size, std::vector<unsigned char>& out) -> fam_status_t;
        static auto Unprotect(const std::vector<unsigned char>& data, std::vector<unsigned char>& out) -> fam_status_t;

        PlatformEnvironment& Environment;
    };
}

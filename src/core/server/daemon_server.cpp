// ============================================================================
//  daemon_server.cpp - the request loop
// ============================================================================

#include "daemon_server.hpp"

#include <format>

#include "platform_api.h"

namespace
{

} // namespace

namespace Famulus
{
    DaemonServer::DaemonServer(struct fam_listener* listener)
        : Listener(listener)
    {
    }

    auto DaemonServer::Run() -> int
    {
        while (true)
        {
            struct fam_stream* client = nullptr;

            if (fam_platform_accept(Listener, &client) != FAM_OK || client == nullptr)
            {
                return 1;
            }

            bool shutdown = false;
            bool done = false;

            while (!done)
            {
                char buffer[512]{};
                size_t length = sizeof(buffer);

                if (fam_platform_read_line(client, buffer, &length) != FAM_OK)
                {
                    break; // client went away, or the line did not fit
                }

                const std::string reply = HandleRequest(std::string_view(buffer, length), shutdown);
                fam_platform_write_line(client, reply.data(), reply.size());

                done = shutdown;
            }

            fam_platform_release_stream(client);

            if (shutdown)
            {
                return 0;
            }
        }
    }

    auto DaemonServer::HandleRequest(std::string_view request, bool& shutdown) -> std::string
    {
        if (request == "status")
        {
            const unsigned long caps = static_cast<unsigned long>(fam_platform_capabilities());

            return std::format("ok protocol=1 daemon=running platform=windows caps=0x{:08x}", caps);
        }

        if (request == "shutdown")
        {
            shutdown = true;
            return std::string{"ok shutting-down"};
        }

        if (request == "quit")
        {
            return std::string{"ok bye"};
        }

        return std::format("err unknown-command {}", request);
    }
} // namespace Famulus

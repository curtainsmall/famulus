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
    DaemonServer::DaemonServer(FamListener* listener)
        : Listener(listener)
    {
    }

    auto DaemonServer::Run() -> StopReason
    {
        while (true)
        {
            FamStream* client = famAccept(Listener);

            if (client == nullptr)
            {
                return StopReason::AcceptFailed;
            }

            bool shutdown = false;
            bool done = false;

            while (!done)
            {
                const char* line = nullptr;
                size_t length = 0;

                if (!famReadLine(client, &line, &length))
                {
                    break; // client went away
                }

                const std::string reply = HandleRequest(std::string_view(line, length), shutdown);
                famWriteLine(client, reply.data(), reply.size());

                done = shutdown;
            }

            famReleaseStream(client);

            if (shutdown)
            {
                return StopReason::Requested;
            }
        }
    }

    auto DaemonServer::HandleRequest(std::string_view request, bool& shutdown) -> std::string
    {
        if (request == "status")
        {
            const unsigned long caps = static_cast<unsigned long>(famGetCapabilities());

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

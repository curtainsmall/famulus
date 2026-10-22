// ============================================================================
//  daemon_server.hpp - what famulusd does with a connected client
//
//  Owns the listening pipe and the loop that reads one request at a time. The
//  wire format is deliberately small: one command per line, one reply line.
//
//      request   status | shutdown | quit
//      reply     ok <fields> | err <message>
//
//  This is not the protocol the shells will speak: that one is newline delimited
//  JSON with sequence numbers and artifact streams. Until the JSON dependency can
//  be fetched, the daemon answers plain text so that a real client can be built
//  and debugged against it.
// ============================================================================

#pragma once

#include <string>
#include <string_view>

#include "platform_api.h"


namespace Famulus
{
    class DaemonServer
    {
    public:
        // The listener is borrowed: the platform owns it, and
        // fam_platform_terminate frees it when the daemon stops.
        explicit DaemonServer(struct fam_listener* listener);
        ~DaemonServer() = default;

        DaemonServer(const DaemonServer&) = delete;
        auto operator=(const DaemonServer&) -> DaemonServer& = delete;

        // Runs until a client asks for shutdown. One client at a time: a status
        // line does not need concurrency, and the daemon keeps one owner thread.
        auto Run() -> int;

    private:
        // Returns the reply line, and sets `shutdown` when the daemon should stop.
        auto HandleRequest(std::string_view request, bool& shutdown) -> std::string;

        struct fam_listener* Listener;
    };
}

// ============================================================================
//  src/core/main.cpp - famulusd
//
//  Process bootstrap, not the brain: parse the arguments, install the host
//  bridge, take the user's pipe (which is also the single-instance lock), run
//  the request loop, then let the platform release what it owns.
// ============================================================================

#include <chrono>
#include <cstdio>
#include <format>
#include <string>
#include <string_view>

#include "server/daemon_server.hpp"
#include "platform_api.h"

namespace
{
    constexpr std::string_view ProgramName = "famulusd";
    constexpr std::string_view DaemonVersion = "0.1.0";

    constexpr std::string_view UsageText = R"(famulusd - the famulus resident daemon

usage:
  famulusd [--foreground] [--help] [--version]

  --foreground   keep diagnostics on stderr instead of handing them to a supervisor
  --help         print this text and exit
  --version      print the daemon version and exit

The daemon listens on a per-user named pipe; famulusctl talks to it.
)";

    void PrintUsage()
    {
        std::fwrite(UsageText.data(), 1, UsageText.size(), stdout);
    }

    void PrintError(std::string_view message)
    {
        const std::string line = std::format("{}: {}\n", ProgramName, message);
        std::fwrite(line.data(), 1, line.size(), stderr);
    }

    // What the platform adapter may call back into. These are the only three
    // entry points core offers it, and they are all the adapter ever gets.
    void HostLog(int level, const char* category, const char* message)
    {
        std::fprintf(stderr, "[%d %s] %s\n", level, category, message);
    }

    void HostPostEvent(const struct fam_event* event)
    {
        std::fprintf(stderr,
                     "event %s %.*s\n",
                     event->type,
                     static_cast<int>(event->payload_len),
                     event->payload != nullptr ? event->payload : "");
    }

    int64_t HostMonotonicMs()
    {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    }
} // namespace

int main(int argc, char** argv)
{
    bool foreground = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view argument(argv[i]);

        if (argument == "--help")
        {
            PrintUsage();
            return 0;
        }

        if (argument == "--version")
        {
            std::printf("%.*s %.*s\n",
                        static_cast<int>(ProgramName.size()),
                        ProgramName.data(),
                        static_cast<int>(DaemonVersion.size()),
                        DaemonVersion.data());
            return 0;
        }

        if (argument == "--foreground")
        {
            foreground = true;
            continue;
        }

        PrintError(std::format("unknown argument '{}'", argument));
        PrintUsage();
        return 1;
    }

    static_cast<void>(foreground);

    const struct fam_host host{sizeof(struct fam_host), HostLog, HostPostEvent, HostMonotonicMs};

    if (fam_platform_init(&host) != FAM_OK)
    {
        PrintError("the platform adapter refused to start");
        return 1;
    }

    if (fam_platform_can_listen() == 0)
    {
        fam_platform_terminate();
        PrintError("this platform has no local endpoint; there is nobody to serve");
        return 1;
    }

    struct fam_listener* listener = nullptr;

    if (fam_platform_listen(&listener) != FAM_OK || listener == nullptr)
    {
        fam_platform_terminate();
        PrintError("another famulusd is already running for this user");
        return 1;
    }

    PrintError("listening on the per-user endpoint, protocol 1");

    Famulus::DaemonServer server(listener);
    const int status = server.Run();

    // One call frees the listener and anything else the platform allocated.
    fam_platform_terminate();

    return status;
}

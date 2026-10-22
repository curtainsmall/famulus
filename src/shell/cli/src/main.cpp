// ============================================================================
//  src/shell/cli/src/main.cpp - famulusctl
//
//  The headless shell: connect to the daemon's pipe, send one request, print the
//  reply. Exit codes are the interface a script sees, so they are fixed here and
//  not guessed per command:
//
//      0   the daemon answered ok
//      1   the daemon answered with an error, or the request was invalid
//      2   the daemon is not running
// ============================================================================

#include <chrono>
#include <cstdio>
#include <format>
#include <string>
#include <string_view>

#include "platform_api.h"


namespace
{
    constexpr std::string_view ProgramName = "famulusctl";
    constexpr auto ConnectTimeout = std::chrono::milliseconds{1000};

    constexpr int ExitOk = 0;
    constexpr int ExitError = 1;
    constexpr int ExitNoDaemon = 2;

    constexpr std::string_view UsageText = R"(famulusctl - talk to the famulus daemon

usage:
  famulusctl status      print protocol version, platform and capabilities
  famulusctl shutdown    ask the daemon to exit cleanly
  famulusctl --help      print this text and exit
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

    // The client installs a host because the ABI asks for one; it never receives
    // platform events and never needs a clock.
    void NoEvent(const FamEvent* event)
    {
        (void)event;
    }

    int64_t NoClock()
    {
        return 0;
    }

    auto Send(FamStream* stream, std::string_view request) -> int
    {
        if (famWriteLine(stream, request.data(), request.size()) != FAM_OK)
        {
            PrintError("could not send the request");
            return ExitError;
        }

        const char* reply = nullptr;
        size_t length = 0;

        if (famReadLine(stream, &reply, &length) != FAM_OK)
        {
            PrintError("the daemon closed the connection without answering");
            return ExitError;
        }

        std::printf("%.*s\n", static_cast<int>(length), reply);

        return length >= 2 && reply[0] == 'o' && reply[1] == 'k' ? ExitOk : ExitError;
    }
} // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        PrintUsage();
        return ExitError;
    }

    const std::string_view command(argv[1]);

    if (command == "--help")
    {
        PrintUsage();
        return ExitOk;
    }

    if (command != "status" && command != "shutdown")
    {
        PrintError(std::format("unknown command '{}'", command));
        PrintUsage();
        return ExitError;
    }

    // The client installs a host because the ABI asks for one; it never receives
    // platform events and never needs a clock.
    const FamHost host{sizeof(FamHost), nullptr, NoEvent, NoClock};

    if (famInit(&host) != FAM_OK)
    {
        PrintError("the platform adapter refused to start");
        return ExitError;
    }

    FamStream* stream = nullptr;

    if (famConnect(static_cast<uint32_t>(ConnectTimeout.count()), &stream) != FAM_OK || stream == nullptr)
    {
        famTerminate();
        PrintError("the daemon is not running");
        return ExitNoDaemon;
    }

    const int status = Send(stream, command);

    // One call frees the stream too, so no path can leak it.
    famTerminate();

    return status;
}

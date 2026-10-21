// ============================================================================
//  src/daemon/main.cpp - famulusd entry point
//
//  Process bootstrap, not the brain. Argument parsing, single-instance
//  acquisition, log initialisation, the supervisor handshake and signal
//  handling all belong here. famulus_core stays a library with no entry point:
//  this directory is a sibling of src/core/, so the library cannot pick this
//  file up at all.
//
//  This is the only file that knows the daemon exists as a process. Nothing
//  here reaches into the platform adapter directly: the daemon asks it through the
//  bridge in the core, never from its entry point.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <format>
#include <string_view>

namespace Famulus
{
    namespace
    {
        constexpr std::string_view ProgramName = "famulusd";

        constexpr std::string_view UsageText = R"(famulusd - the famulus resident daemon

usage:
  famulusd [--foreground] [--help]

  --foreground   keep diagnostics on stderr instead of handing them to a supervisor
  --help         print this text and exit
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
    } // namespace
} // namespace Famulus

int main(int argc, char** argv)
{
    bool foreground = false;

    for(int i = 1; i < argc; ++i)
    {
        const std::string_view argument(argv[i]);

        if(argument == "--help")
        {
            Famulus::PrintUsage();
            return EXIT_SUCCESS;
        }

        if(argument == "--foreground")
        {
            foreground = true;
            continue;
        }

        Famulus::PrintError("unknown argument");
        Famulus::PrintUsage();
        return EXIT_FAILURE;
    }

    static_cast<void>(foreground);

    Famulus::PrintError("not implemented yet: no session server, no adapter, no shell");
    return EXIT_FAILURE;
}

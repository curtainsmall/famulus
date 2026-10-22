// ============================================================================
//  src/base/include/logger.hpp - the file behind host->log
//
//  Platform independent on purpose: it takes a directory the caller obtained
//  from the platform and writes lines into it, so nothing here knows what a
//  platform is.
//
//  spdlog does the work (sinks, rotation, formatting) and stays inside
//  logger.cpp: the interface the rest of the tree sees is three integers and a
//  string, so replacing the logger later does not touch a caller.
// ============================================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace spdlog
{
    class logger;
}

namespace Famulus
{
    // The three levels a log line can have. The numbers are fixed because the
    // platform ABI numbers its own levels the same way; the two enums stay
    // separate, since base may not include a platform header.
    enum class LogLevel
    {
        Info = 0,
        Warning = 1,
        Error = 2,
    };

    class Logger
    {
    public:
        Logger(std::string directory, std::string file_name, std::size_t max_bytes);
        ~Logger();

        Logger(const Logger&) = delete;
        auto operator=(const Logger&) -> Logger& = delete;

        // Appends one line to the file and echoes it to stderr, rotating when the
        // file passes the limit.
        void Write(LogLevel level, std::string_view category, std::string_view message);

        // Whether the file could be opened; false leaves logging on stderr only.
        auto IsOpen() const -> bool;

    private:
        std::shared_ptr<spdlog::logger> Sink;
    };
}

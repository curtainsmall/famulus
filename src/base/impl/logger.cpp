// ============================================================================
//  src/base/src/logger.cpp - spdlog behind a three-argument interface
//
//  One logger with two sinks: the rotating file the daemon reads when something
//  went wrong at three in the morning, and stderr so a foreground run shows the
//  same lines in the terminal.
// ============================================================================

#include "logger.hpp"

#include <cstdio>
#include <filesystem>
#include <system_error>
#include <utility>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace
{
    constexpr std::size_t DefaultMaxBytes = 4u * 1024u * 1024u;

    auto LevelFrom(int level) -> spdlog::level::level_enum
    {
        switch (level)
        {
        case 1:
            return spdlog::level::warn;
        case 2:
            return spdlog::level::err;
        default:
            return spdlog::level::info;
        }
    }

    // Wall clock with level and message. spdlog expands the timestamp, so the
    // format lives here and nowhere else.
    auto Pattern() -> const char*
    {
        return "%Y-%m-%d %H:%M:%S.%e [%l] %v";
    }
} // namespace

namespace Famulus
{
    Logger::Logger(std::string directory, std::string file_name, std::size_t max_bytes)
    {
        if (directory.empty())
        {
            return;
        }

        std::error_code error;

        std::filesystem::create_directories(directory, error);

        const std::filesystem::path path = std::filesystem::path(directory) / file_name;

        try
        {
            // Two sinks, one logger: the file rotates, stderr does not need to.
            auto stderr_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                path.string(), max_bytes == 0 ? DefaultMaxBytes : max_bytes, 1);

            auto made = std::make_shared<spdlog::logger>("famulus",
                                                         spdlog::sinks_init_list{stderr_sink, file_sink});

            made->set_pattern(Pattern());
            made->set_level(spdlog::level::info);
            made->flush_on(spdlog::level::info);

            Sink = std::move(made);
        }
        catch (const spdlog::spdlog_ex&)
        {
            // No file, no logger: Write still reports through stderr only, and
            // IsOpen tells the caller that the file is missing.
            Sink = nullptr;
        }
    }

    Logger::~Logger() = default;

    auto Logger::IsOpen() const -> bool
    {
        return Sink != nullptr;
    }

    void Logger::Write(int level, std::string_view category, std::string_view message)
    {
        const std::string line = std::string(category) + ": " + std::string(message);

        if (Sink != nullptr)
        {
            Sink->log(LevelFrom(level), line);
            return;
        }

        std::fprintf(stderr, "%s\n", line.c_str());
    }
} // namespace Famulus

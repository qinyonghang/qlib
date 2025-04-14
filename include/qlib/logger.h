#pragma once

#ifdef _WIN32
#include "spdlog/sinks/wincolor_sink.h"
#else
#include "spdlog/sinks/ansicolor_sink.h"
#endif
#include "spdlog/fmt/chrono.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

template <class T, class Char>
struct fmt::formatter<T, Char, std::void_t<decltype(std::decay_t<T>().str())>>
        : public fmt::formatter<std::string> {
    auto format(T const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(value.str(), ctx);
    }
};

template <class T, class Char>
struct fmt::formatter<T, Char, std::void_t<decltype(std::decay_t<T>().string())>>
        : public fmt::formatter<std::string> {
    auto format(T const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(value.string(), ctx);
    }
};

template <class T, class Char>
struct fmt::formatter<T, Char, std::void_t<decltype(std::decay_t<T>().time_since_epoch())>>
        : public fmt::formatter<int64_t> {
    auto format(T const& value, format_context& ctx) const {
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch());
        return fmt::formatter<int64_t>::format(time.count(), ctx);
    }
};

#define qTrace(fmt, ...) spdlog::trace("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qDebug(fmt, ...) spdlog::debug("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qInfo(fmt, ...) spdlog::info("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qWarn(fmt, ...) spdlog::warn("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qError(fmt, ...) spdlog::error("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qCritical(fmt, ...) spdlog::critical("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)

namespace qlib {

class logger_register final {
protected:
    logger_register() {
#ifdef _WIN32
        auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
        auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            fmt::format("logs/{:%Y-%m-%d_%H-%M-%S}.log", fmt::localtime(time)), true);

        spdlog::sinks_init_list sinks{color_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("End2End", sinks.begin(), sinks.end());
        spdlog::set_default_logger(logger);

        auto log_level = std::getenv("END2END_LOG_LEVEL");
        if (log_level != nullptr) {
            char* end = nullptr;
            auto log_level_int = std::strtol(log_level, &end, 10);
            if (*end == '\0' && log_level_int >= 0 && log_level_int < spdlog::level::n_levels) {
                qInfo("Log level: {}", log_level);
                spdlog::set_level(static_cast<spdlog::level::level_enum>(log_level_int));
            } else {
                qError("Invalid log level: {}", log_level);
            }
        }
    }

public:
    static auto& get_instance() {
        static logger_register impl;
        return impl;
    }
};

};  // namespace qlib
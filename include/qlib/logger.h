#pragma once

#include <filesystem>
#include <vector>

#include "qlib/object.h"
#include "spdlog/fmt/chrono.h"
#include "spdlog/sinks/basic_file_sink.h"
#ifdef _WIN32
#include "spdlog/sinks/wincolor_sink.h"
#else
#include "spdlog/sinks/ansicolor_sink.h"
#endif
#include "spdlog/spdlog.h"

#ifdef ARGPARSE_IMPLEMENTATION
template <>
spdlog::level::level_enum argparse::ArgumentParser::get<spdlog::level::level_enum>(
    std::string_view arg_name) const {
    THROW_EXCEPTION(m_is_parsed, "Nothing parsed, no arguments are available.");

    spdlog::level::level_enum value{spdlog::level::n_levels};
    try {
        value = (*this)[arg_name].get<spdlog::level::level_enum>();
    } catch (std::bad_any_cast const&) {
        value = static_cast<spdlog::level::level_enum>((*this)[arg_name].get<size_t>());
    }
    THROW_EXCEPTION(value <= spdlog::level::n_levels, "value({}) must be less than {}!", value,
                    spdlog::level::n_levels);

    return value;
}
#endif

#ifdef DJI_TYPEDEF_H
template <>
struct fmt::formatter<T_DjiVector3d> : public fmt::formatter<std::string> {
    auto format(T_DjiVector3d const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("[{},{},{}]", value.x, value.y, value.z), ctx);
    }
};

template <>
struct fmt::formatter<T_DjiVector3f> : public fmt::formatter<std::string> {
    auto format(T_DjiVector3f const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("[{:.2f},{:.2f},{:.2f}]", value.x, value.y, value.z), ctx);
    }
};

template <>
struct fmt::formatter<T_DjiDataTimestamp> : public fmt::formatter<std::string> {
    auto format(T_DjiDataTimestamp const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("[{},{}]", value.millisecond, value.microsecond), ctx);
    }
};
#endif

#ifdef DJI_FC_SUBSCRIPTION_H
template <>
struct fmt::formatter<T_DjiFcSubscriptionVelocity> : public fmt::formatter<std::string> {
    auto format(T_DjiFcSubscriptionVelocity const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(fmt::format("[{},{}]", value.data, value.health),
                                                   ctx);
    }
};

template <>
struct fmt::formatter<T_DjiFcSubscriptionRtkPosition> : public fmt::formatter<std::string> {
    auto format(T_DjiFcSubscriptionRtkPosition const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("[{:.4f},{:.4f},{:.2f}]", value.longitude, value.latitude, value.hfsl),
            ctx);
    }
};
#endif

#ifdef DJI_PERCEPTION_H
template <>
struct fmt::formatter<T_DjiPerceptionRawImageInfo> : public fmt::formatter<std::string> {
    auto format(T_DjiPerceptionRawImageInfo const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("[{:#x},{},{},{},{}]", value.index, value.direction, value.bpp, value.width,
                        value.height),
            ctx);
    }
};

template <>
struct fmt::formatter<T_DjiPerceptionImageInfo> : public fmt::formatter<std::string> {
    auto format(T_DjiPerceptionImageInfo const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("[{},{},{},{},{}]", value.rawInfo, value.dataId, value.sequence,
                        value.dataType, value.timeStamp),
            ctx);
    }
};
#endif

template <>
struct fmt::formatter<spdlog::level::level_enum> : public fmt::formatter<int32_t> {
    auto format(spdlog::level::level_enum const& value, format_context& ctx) const {
        return fmt::formatter<int32_t>::format(static_cast<int32_t>(value), ctx);
    }
};

template <>
struct fmt::formatter<std::filesystem::path> : public fmt::formatter<std::string> {
    auto format(std::filesystem::path const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(value.string(), ctx);
    }
};

template <class T, class Char>
struct fmt::formatter<T, Char, std::void_t<decltype(std::decay_t<T>().to_string())>>
        : public fmt::formatter<std::string> {
    auto format(T const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(value.to_string(), ctx);
    }
};

template <class T>
struct fmt::formatter<std::vector<T>> : public fmt::formatter<std::string> {
    auto format(std::vector<T> const& vector, format_context& ctx) const {
        std::stringstream out;
        out << "[";
        for (auto i = 0u; i < vector.size() - 1; ++i) {
            out << fmt::format("{},", vector[i]);
        }
        if (vector.size() > 0) {
            out << fmt::format("{}", vector.back());
        }
        out << "]";
        return fmt::formatter<std::string>::format(out.str(), ctx);
    }
};

#ifdef _GLIBCXX_ARRAY
template <class T, size_t N>
struct fmt::formatter<std::array<T, N>> : fmt::formatter<std::string> {
    auto format(std::array<T, N> const& vector, format_context& ctx) const {
        std::stringstream out;
        out << "[";
        for (auto i = 0u; i < vector.size() - 1; ++i) {
            out << fmt::format("{},", vector[i]);
        }
        if (vector.size() > 0) {
            out << fmt::format("{}", vector.back());
        }
        out << "]";
        return fmt::formatter<std::string>::format(out.str(), ctx);
    }
};
#endif

#ifdef _STL_PAIR_H
template <class T1, class T2>
struct fmt::formatter<std::pair<T1, T2>> : fmt::formatter<std::string> {
    auto format(std::pair<T1, T2> const& value, format_context& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("[{},{}]", value.first, value.second), ctx);
    }
};
#endif

#ifdef _GLIBCXX_ARRAY
template <class T, size_t N>
struct fmt::formatter<std::array<T, N>> : fmt::formatter<std::string> {
    auto format(std::array<T, N> const& vector, format_context& ctx) const {
        std::stringstream out;
        out << "[";
        for (auto i = 0u; i < vector.size() - 1; ++i) {
            out << fmt::format("{},", vector[i]);
        }
        if (vector.size() > 0) {
            out << fmt::format("{}", vector.back());
        }
        out << "]";
        return fmt::formatter<std::string>::format(out.str(), ctx);
    }
};
#endif

// template <class T, class Char>
// struct fmt::formatter<T, Char, std::void_t<decltype(std::decay_t<T>().string())>>
//         : public fmt::formatter<std::string> {
//     auto format(T const& value, format_context& ctx) const {
//         return fmt::formatter<std::string>::format(value.string(), ctx);
//     }
// };

// template <class T, class Char>
// struct fmt::formatter<T, Char, std::void_t<decltype(std::decay_t<T>().time_since_epoch())>>
//         : public fmt::formatter<int64_t> {
//     auto format(T const& value, format_context& ctx) const {
//         auto time = std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch());
//         return fmt::formatter<int64_t>::format(time.count(), ctx);
//     }
// };

#define qTrace(fmt, ...) spdlog::trace("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qDebug(fmt, ...) spdlog::debug("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qInfo(fmt, ...) spdlog::info("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qWarn(fmt, ...) spdlog::warn("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qError(fmt, ...) spdlog::error("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qCritical(fmt, ...) spdlog::critical("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)

namespace qlib {

namespace logger {
using namespace spdlog;
using level = spdlog::level::level_enum;

class factory final : public object {
public:
    using self = factory;
    using ptr = std::shared_ptr<self>;
    using base = object;

    factory() = default;

    self& name(string_t const& name) {
        self::_name = name;
        return *this;
    }

    self& console(bool console = true, level level = level::info) {
        self::_console = console;
        self::_console_level = level;
        return *this;
    }

    self& file(std::filesystem::path const& prefix = "logs", level level = level::trace) {
        self::_prefix = prefix;
        self::_file_level = level;
        return *this;
    }

    auto build() const {
        std::vector<spdlog::sink_ptr> sinks;

        if (self::_console) {
#ifdef _WIN32
            auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
            auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
            color_sink->set_level(self::_console_level);
            sinks.emplace_back(color_sink);
        }

        if (self::_prefix != std::filesystem::path{}) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                fmt::format("{}/{:%Y-%m-%d_%H-%M-%S}.log", self::_prefix, fmt::localtime(time)));
            file_sink->set_level(_file_level);
            sinks.emplace_back(file_sink);
        }

        return std::make_shared<spdlog::logger>(self::_name, sinks.begin(), sinks.end());
    }

protected:
    string_t _name{"default"};
    bool _console{false};
    level _console_level;
    std::filesystem::path _prefix;
    level _file_level;
};

class register2 final : public object {
public:
    static auto build(string_t const& name,
                      bool console = true,
                      bool file = true,
                      std::filesystem::path const& log_file_prefix = "logs") {
        std::vector<spdlog::sink_ptr> sinks;

        if (console) {
#ifdef _WIN32
            auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
            auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
            sinks.emplace_back(color_sink);
        }

        if (file) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format(
                "{}/{:%Y-%m-%d_%H-%M-%S}.log", log_file_prefix.string(), fmt::localtime(time)));

            sinks.emplace_back(file_sink);
        }

        auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
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
};

};  // namespace logger
};  // namespace qlib
#pragma once

#include <filesystem>

#include "qlib/object.h"
#include "qlib/string.h"

#include "spdlog/fmt/chrono.h"
#include "spdlog/sinks/basic_file_sink.h"
#ifdef _WIN32
#include "spdlog/sinks/wincolor_sink.h"
#else
#include "spdlog/sinks/ansicolor_sink.h"
#endif
#include "spdlog/spdlog.h"

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

    self& name(string_view_t name) {
        self::_name = name;
        return *this;
    }

    self& console(bool console = true, level level = level::info) {
        self::_console = console;
        self::_console_level = level;
        return *this;
    }

    self& console(level level = level::info) {
        self::_console = (level != level::off);
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
        sinks.reserve(2u);

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

        return std::make_shared<spdlog::logger>(self::_name.data(), sinks.begin(), sinks.end());
    }

protected:
    string_t _name{"default"};
    bool _console{false};
    level _console_level;
    std::filesystem::path _prefix;
    level _file_level;
};

};  // namespace logger

// #ifdef CONVERTER_IMPLEMENTATION

// template <>
// struct converter<logger::level> : public object {
//     static logger::level decode(string_view_t s) {
//         logger::level value{logger::level::n_levels};
//         value = static_cast<logger::level>(converter<size_t>::decode(s));
//         THROW_EXCEPTION(value <= logger::level::n_levels, "value({}) must be less than {}!", value,
//                         logger::level::n_levels);

//         return value;
//     }
// };

// #endif

};  // namespace qlib

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

template <class Char>
struct fmt::formatter<qlib::string::value<Char>, Char>
        : public fmt::formatter<fmt::basic_string_view<Char>, Char> {
    template <class FormatContext>
    auto format(qlib::string::value<Char, Policy> value, FormatContext& ctx) const {
        using Base = fmt::basic_string_view<Char>;
        return fmt::formatter<Base, Char>::format(Base{value.data(), value.size()}, ctx);
    }
};

template <class Char>
struct fmt::formatter<qlib::string::view<Char>, Char>
        : public fmt::formatter<fmt::basic_string_view<Char>, Char> {
    template <class FormatContext>
    auto format(qlib::string::view<Char, Policy> value, FormatContext& ctx) const {
        using Base = fmt::basic_string_view<Char>;
        return fmt::formatter<Base, Char>::format(Base{value.data(), value.size()}, ctx);
    }
};

template <class T, class Char>
struct fmt::formatter<T, Char, std::void_t<decltype(std::decay_t<T>().to_string())>>
        : public fmt::formatter<std::decay_t<decltype(std::declval<T>().to_string())>, Char> {
    auto format(T const& value, format_context& ctx) const {
        using Base = std::decay_t<decltype(value.to_string())>;
        return fmt::formatter<Base, Char>::format(value.to_string(), ctx);
    }
};

template <class T>
struct fmt::formatter<std::vector<T>> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const std::vector<T>& vector, FormatContext& ctx) const {
        auto out = ctx.out();

        *out++ = '[';

        if (!vector.empty()) {
            out = fmt::format_to(out, "{}", vector[0]);

            for (size_t i = 1; i < vector.size(); ++i) {
                *out++ = ',';
                out = fmt::format_to(out, "{}", vector[i]);
            }
        }

        *out++ = ']';
        return out;
    }
};

#ifdef _GLIBCXX_ARRAY
template <class T, size_t N>
struct fmt::formatter<std::array<T, N>> : fmt::formatter<std::string> {
    auto format(std::array<T, N> const& vector, format_context& ctx) const {
        auto out = ctx.out();
        *out++ = '[';
        if (!vector.empty()) {
            out = fmt::format_to(out, "{}", vector[0]);

            for (size_t i = 1; i < vector.size(); ++i) {
                *out++ = ',';
                out = fmt::format_to(out, "{}", vector[i]);
            }
        }
        *out++ = ']';
        return out;
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

#ifndef QLIB_LOGGER_H
#define QLIB_LOGGER_H

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

namespace qlib {

class logger final : public object {
public:
    using base = object;
    using self = logger;
    using level_type = spdlog::level::level_enum;

protected:
    std::shared_ptr<spdlog::logger> _impl;

public:
    logger(self const&) = delete;
    logger(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    logger() = default;

    template <class... _Args>
    logger(_Args&&... __args) {
        auto result = init(forward<_Args>(__args)...);
        throw_if(result != 0u, "logger exception");
    }

    template <class _Map>
    auto init(_Map const& __map) {
        int32_t result{0};

        do {
            std::vector<spdlog::sink_ptr> __sinks;

            auto& __console = __map["console"];
            if (__console["enable"] && __console["enable"].template get<bool_t>()) {
                auto __level = __console["level"].template get<uint32_t>();
#ifdef _WIN32
                auto __sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
                auto __sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
                __sink->set_level(level_type(__level));
                __sinks.emplace_back(__sink);
            }
            auto& __file = __map["file"];
            if (__file["enable"] && __file["enable"].template get<bool_t>()) {
                auto __path = __file["path"].template get<std::string>();
                auto __level = __file["level"].template get<uint32_t>();
                if (__path == "auto") {
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    __path = fmt::format("logs/{:%Y-%m-%d_%H-%M-%S}.log", fmt::localtime(time));
                }
                auto __sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(__path.c_str());
                __sink->set_level(level_type(__level));
                __sinks.emplace_back(__sink);
            }
            auto __name = __map["name"].template get<std::string>();
            _impl = std::make_shared<spdlog::logger>(__name, __sinks.begin(), __sinks.end());
            _impl->set_level(level_type(__map["level"].template get<uint32_t>()));

            _impl->trace("logger init!");
        } while (false);

        return result;
    }

    template <class... _Args>
    void trace(_Args&&... __args) {
        _impl->trace(forward<_Args>(__args)...);
    }

    template <class... _Args>
    void debug(_Args&&... __args) {
        _impl->debug(forward<_Args>(__args)...);
    }

    template <class... _Args>
    void info(_Args&&... __args) {
        _impl->info(forward<_Args>(__args)...);
    }

    template <class... _Args>
    void warn(_Args&&... __args) {
        _impl->warn(forward<_Args>(__args)...);
    }

    template <class... _Args>
    void error(_Args&&... __args) {
        _impl->error(forward<_Args>(__args)...);
    }

    template <class... _Args>
    void critical(_Args&&... __args) {
        _impl->critical(forward<_Args>(__args)...);
    }
};

};  // namespace qlib

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

template <class Char>
struct fmt::formatter<qlib::string::value<Char>, Char>
        : public fmt::formatter<fmt::basic_string_view<Char>, Char> {
    template <class FormatContext>
    auto format(qlib::string::value<Char> value, FormatContext& ctx) const {
        using Base = fmt::basic_string_view<Char>;
        return fmt::formatter<Base, Char>::format(Base{value.data(), value.size()}, ctx);
    }
};

template <class Char>
struct fmt::formatter<qlib::string::view<Char>, Char>
        : public fmt::formatter<fmt::basic_string_view<Char>, Char> {
    template <class FormatContext>
    auto format(qlib::string::view<Char> value, FormatContext& ctx) const {
        using Base = fmt::basic_string_view<Char>;
        return fmt::formatter<Base, Char>::format(Base{value.data(), value.size()}, ctx);
    }
};

#endif

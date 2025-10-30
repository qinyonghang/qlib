#ifndef QLIB_LOGGER_H
#define QLIB_LOGGER_H

#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

#include "qlib/logger.h"

namespace qlib {

class slogger final : public object {
public:
    using base = object;
    using self = slogger;
    using level_type = logger::level;
    using sink_ptr = std::shared_ptr<logger::sink>;
    using logger_type = logger::value<string_view_t, vector_t<sink_ptr>>;

protected:
    std::shared_ptr<logger_type> _impl;

public:
    slogger(self const&) = delete;
    slogger(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    slogger() = default;

    template <class... _Args>
    slogger(_Args&&... __args) {
        auto result = init(qlib::forward<_Args>(__args)...);
        throw_if(result != 0u, "logger exception");
    }

    template <class _Map>
    auto init(_Map const& __map) {
        int32_t result{0};

        do {
            vector_t<sink_ptr> __sinks;

            auto& __console = __map["console"];
            if (__console["enable"] && __console["enable"].template get<bool_t>()) {
                auto __level = __console["level"].template get<uint32_t>();
                auto __sink = std::make_shared<logger::ansicolor_stdout_sink_mt>();
                __sink->set_level(level_type(__level));
                __sinks.emplace_back(__sink);
            }
            auto& __file = __map["file"];
            if (__file["enable"] && __file["enable"].template get<bool_t>()) {
                auto __path = __file["path"].template get<string_t>();
                auto __level = __file["level"].template get<uint32_t>();
                if (__path == "auto") {
                    auto now = std::chrono::system_clock::now();
                    __path = fmt::format("logs/{:%Y-%m-%d_%H-%M-%S}.log", now);
                }
                auto __sink = std::make_shared<logger::file_sink_mt>(__path.c_str());
                __sink->set_level(level_type(__level));
                __sinks.emplace_back(__sink);
            }
            auto __name = __map["name"].template get<string_view_t>();
            _impl = std::make_shared<logger_type>(__name, qlib::move(__sinks));
            // _impl->set_level(level_type(__map["level"].template get<uint32_t>()));

            _impl->trace("logger: init!");
        } while (false);

        return result;
    }

    template <class... _Args>
    void trace(_Args&&... __args) {
        _impl->trace(qlib::forward<_Args>(__args)...);
    }

    template <class... _Args>
    void debug(_Args&&... __args) {
        _impl->debug(qlib::forward<_Args>(__args)...);
    }

    template <class... _Args>
    void info(_Args&&... __args) {
        _impl->info(qlib::forward<_Args>(__args)...);
    }

    template <class... _Args>
    void warn(_Args&&... __args) {
        _impl->warn(qlib::forward<_Args>(__args)...);
    }

    template <class... _Args>
    void error(_Args&&... __args) {
        _impl->error(qlib::forward<_Args>(__args)...);
    }

    template <class... _Args>
    void critical(_Args&&... __args) {
        _impl->critical(qlib::forward<_Args>(__args)...);
    }
};

};  // namespace qlib

#endif

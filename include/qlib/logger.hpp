#ifndef QLIB_LOGGER_HPP
#define QLIB_LOGGER_HPP

#include "qlib/format.h"
#include "qlib/string.h"
#include "qlib/vector.h"

namespace qlib {

namespace log {

enum level : uint8_t { trace, debug, info, warn, err, critical, off, n_levels };

#ifndef _QLOGGER_LEVEL_
constexpr static level static_level = level::trace;
#else
constexpr static level static_level = _QLOGGER_LEVEL_;
#endif

class null_mutex final : public object {
public:
    ALWAYS_INLINE CONSTEXPR void lock() {}
    ALWAYS_INLINE CONSTEXPR void unlock() {}
    static null_mutex& instance() {
        static null_mutex __instance;
        return __instance;
    }
};

#ifdef _GLIBCXX_MUTEX_H
class mutex {
protected:
    std::mutex _mutex;

public:
    ALWAYS_INLINE void lock() { _mutex.lock(); }
    ALWAYS_INLINE void unlock() { _mutex.unlock(); }

    static mutex& instance() {
        static mutex __instance;
        return __instance;
    }
};  // namespace mutex
#endif

struct msg : public object {
    level lvl;
#ifdef _GLIBCXX_CHRONO
    string_view_t tp;
#endif
    string_view_t name;
    string_view_t data;
};

class sink : public object {
public:
    using base = object;
    using self = sink;

protected:
    level _level{info};

public:
    virtual ~sink() = default;
    virtual void log(msg const&) = 0;
    // virtual void flush() = 0;
    // virtual void set_pattern(const std::string& pattern) = 0;
    // virtual void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) = 0;

    ALWAYS_INLINE CONSTEXPR void set_level(level __level) { _level = __level; }
};

template <class _OStream, class _Mutex = null_mutex>
class ansicolor_sink : public sink {
public:
    using base = sink;
    using self = ansicolor_sink;
    using msg_type = msg;
    using stream_type = _OStream;
    using mutex_type = _Mutex;

protected:
    stream_type& _stream;
    mutex_type& _mutex;

    using level_pair = pair<string_view_t, string_view_t>;

    constexpr static level_pair __map[n_levels] = {
        level_pair("trace", "\033[37m"),            // white
        level_pair("debug", "\033[36m"),            // cyan
        level_pair("info", "\033[32m"),             // green
        level_pair("warn", "\033[33m\033[1m"),      // yellow_bold
        level_pair("error", "\033[31m\033[1m"),     // red_bold
        level_pair("critical", "\033[1m\033[41m"),  // bold_on_red
        level_pair("off", "\033[m")                 // reset
    };

    ALWAYS_INLINE CONSTEXPR static auto& _level_(level __l) { return __map[__l]; }

public:
    ansicolor_sink(self const& other) = delete;
    self& operator=(self const& other) = delete;

    ansicolor_sink(self&&) = default;
    ~ansicolor_sink() override = default;
    self& operator=(self&& other) = default;

    ALWAYS_INLINE CONSTEXPR ansicolor_sink(stream_type& __s)
            : _stream(__s), _mutex(mutex_type::instance()) {}

    ALWAYS_INLINE CONSTEXPR void log(msg_type const& msg) override {
        if (msg.lvl >= base::_level) {
            _mutex.lock();
            auto& l = _level_(msg.lvl);
            auto& off = _level_(level::off);
#ifdef _GLIBCXX_CHRONO
            _stream << "[" << msg.tp << "] ";
#endif
            _stream << "[" << msg.name << "] [" << l.value << l.key << off.value << "] " << msg.data
                    << "\n";
            _stream.flush();
            _mutex.unlock();
        }
    }
    // void set_color(level::level_enum color_level, string_view_t color);
    // void set_color_mode(color_mode mode);
    // bool should_color();
    // void flush() override;
    // void set_pattern(const std::string& pattern) final override;
    // void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override;
};

#ifdef _GLIBCXX_IOSFWD
template <class _Mutex>
class ansicolor_stdout_sink : public ansicolor_sink<std::ostream, _Mutex> {
public:
    ansicolor_stdout_sink() : ansicolor_sink<std::ostream, _Mutex>(std::cout) {}
};

template <class _Mutex>
class ansicolor_stderr_sink : public ansicolor_sink<std::ostream, _Mutex> {
public:
    ansicolor_stderr_sink() : ansicolor_sink<std::ostream, _Mutex>(std::cerr) {}
};

using ansicolor_stdout_sink_st = ansicolor_stdout_sink<null_mutex>;
using ansicolor_stderr_sink_st = ansicolor_stderr_sink<null_mutex>;
#ifdef _GLIBCXX_MUTEX_H
using ansicolor_stdout_sink_mt = ansicolor_stdout_sink<mutex>;
using ansicolor_stderr_sink_mt = ansicolor_stderr_sink<mutex>;
#endif
#endif

template <class _FStream, class _Mutex = null_mutex>
class file_sink : public sink {
public:
    using base = sink;
    using self = file_sink;
    using stream_type = _FStream;
    using msg_type = msg;
    using mutex_type = _Mutex;

protected:
    stream_type _stream;
    mutex_type _mutex;

    constexpr static string_view_t map[n_levels] = {"trace", "debug",    "info", "warn",
                                                    "error", "critical", "off"};

public:
    file_sink(self const&) = delete;
    self& operator=(self const&) = delete;

    file_sink(self&&) = default;
    ~file_sink() override = default;
    self& operator=(self&&) = default;

    template <class _S>
    ALWAYS_INLINE CONSTEXPR file_sink(_S&& __path) : _stream(__path) {
        throw_if(!_stream.is_open(), "cannot open file");
    }

    ALWAYS_INLINE CONSTEXPR void log(msg_type const& msg) override {
        if (msg.lvl >= base::_level) {
            _mutex.lock();
#ifdef _GLIBCXX_CHRONO
            _stream << "[" << msg.tp << "] ";
#endif
            _stream << "[" << msg.name << "] [" << map[msg.lvl] << "] " << msg.data << "\n";
            _stream.flush();
            _mutex.unlock();
        }
    }
};

#ifdef _GLIBCXX_FSTREAM
using file_sink_st = file_sink<std::ofstream, null_mutex>;
#ifdef _GLIBCXX_MUTEX_H
using file_sink_mt = file_sink<std::ofstream, mutex>;
#endif
#endif

template <class _Name, class _Sinks>
class value final : public object {
public:
    using base = object;
    using self = value;
    using name_type = _Name;
    using sinks_type = _Sinks;
    using msg_type = msg;
    using string_view = string_view_t;
    using string_type = string_t;

protected:
    name_type _name;
    sinks_type _sinks;

    struct formatter final : public object {
#ifdef _GLIBCXX_CHRONO
        fmt::array<char, 32u> _time_str;
#endif
        string_type _data;
        template <class... _Args>
        ALWAYS_INLINE CONSTEXPR msg_type operator()(string_view name, level lvl, _Args&&... args) {
#ifdef _GLIBCXX_CHRONO
            _time_str =
                fmt::bformat<32u>("{:%Y-%m-%d %H:%M:%S.%3f}", std::chrono::system_clock::now());
#endif
            _data = fmt::format(forward<_Args>(args)...);
            msg_type msg{.lvl = lvl,
#ifdef _GLIBCXX_CHRONO
                         .tp = string_view_t(_time_str.begin(), _time_str.end()),
#endif
                         .name = name,
                         .data = _data};
            return msg;
        }
    };

    template <class _Sink, class... _Args>
    ALWAYS_INLINE CONSTEXPR static enable_if_t<is_pointer_v<_Sink>, void> _log_(_Sink& __sink,
                                                                                _Args&&... __args) {
        __sink->log(forward<_Args>(__args)...);
    }

    template <class _Sink, class... _Args>
    ALWAYS_INLINE CONSTEXPR static enable_if_t<!is_pointer_v<_Sink>, void> _log_(
        _Sink& __sink, _Args&&... __args) {
        __sink.log(forward<_Args>(__args)...);
    }

    template <class _uSinks = sinks_type, class... _Args>
    ALWAYS_INLINE CONSTEXPR enable_if_t<is_container_v<_uSinks>, void> _log_(level __level,
                                                                             _Args&&... __args) {
        formatter _f;
        auto msg = _f(_name, __level, forward<_Args>(__args)...);
        for (auto& sink : _sinks) {
            _log_(sink, msg);
        }
    }

    template <class _uSinks = sinks_type, class... _Args>
    ALWAYS_INLINE CONSTEXPR enable_if_t<!is_container_v<_uSinks>, void> _log_(level __level,
                                                                              _Args&&... __args) {
        formatter _f;
        auto msg = _f(_name, __level, forward<_Args>(__args)...);
        _log_(_sinks, msg);
    }

public:
    value() = delete;
    value(self const&) = delete;
    self& operator=(self const&) = delete;

    ALWAYS_INLINE CONSTEXPR value(self&& __o) : _name(move(__o._name)), _sinks(move(__o._sinks)) {}

    template <class _uName, class _uSinks>
    ALWAYS_INLINE CONSTEXPR value(_uName&& __n, _uSinks&& __sinks)
            : _name(forward<_uName>(__n)), _sinks(forward<_uSinks>(__sinks)) {}

    ALWAYS_INLINE CONSTEXPR self& operator=(self&& __o) {
        if (this != &__o) {
            _name = move(__o._name);
            _sinks = move(__o._sinks);
        }
        return *this;
    }

    template <class... _Args>
    ALWAYS_INLINE CONSTEXPR void trace(_Args&&... __args) {
        if CONSTEXPR (static_level <= level::trace) {
            _log_(level::trace, forward<_Args>(__args)...);
        }
    }

    template <class... _Args>
    ALWAYS_INLINE CONSTEXPR void debug(_Args&&... __args) {
        if CONSTEXPR (static_level <= level::debug) {
            _log_(level::debug, forward<_Args>(__args)...);
        }
    }

    template <class... _Args>
    ALWAYS_INLINE CONSTEXPR void info(_Args&&... __args) {
        if CONSTEXPR (static_level <= level::info) {
            _log_(level::info, forward<_Args>(__args)...);
        }
    }

    template <class... _Args>
    ALWAYS_INLINE CONSTEXPR void warn(_Args&&... __args) {
        if CONSTEXPR (static_level <= level::warn) {
            _log_(level::warn, forward<_Args>(__args)...);
        }
    }

    template <class... _Args>
    ALWAYS_INLINE CONSTEXPR void error(_Args&&... __args) {
        if CONSTEXPR (static_level <= level::err) {
            _log_(level::err, forward<_Args>(__args)...);
        }
    }

    template <class... _Args>
    ALWAYS_INLINE CONSTEXPR void critical(_Args&&... __args) {
        if CONSTEXPR (static_level <= level::critical) {
            _log_(level::critical, forward<_Args>(__args)...);
        }
    }
};

};  // namespace log

};  // namespace qlib

#endif

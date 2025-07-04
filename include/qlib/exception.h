#pragma once

#ifdef NO_EXCEPTION

#define THROW_EXCEPTION(ok, fmt, ...)

#else

#include <exception>

#include "qlib/object.h"
#include "spdlog/fmt/fmt.h"

namespace qlib {
class exception final : public std::exception {
protected:
    char _impl[512u];

public:
    exception(char const* file, int line, char const* message) {
        snprintf(_impl, sizeof(_impl), "[%s:%d]%s", file, line, message);
    }

    char const* what() const noexcept override { return _impl; }
};
};  // namespace qlib

#define THROW_EXCEPTION(ok, format_str, ...)                                                       \
    do {                                                                                           \
        if (!(qlib::likely(ok))) {                                                                 \
            throw qlib::exception(__FILE__, __LINE__,                                              \
                                  fmt::format(format_str, ##__VA_ARGS__).c_str());                 \
        }                                                                                          \
    } while (false)

#endif

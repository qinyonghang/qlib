#pragma once

#ifdef NO_EXCEPTION

#define THROW_EXCEPTION(ok, fmt, ...)

#else

#include <exception>

#include "spdlog/fmt/fmt.h"

namespace qlib {
class exception final : public std::exception {
protected:
    constexpr static auto max_length = 512u;
    char __impl[max_length];

public:
    exception(char const* file, int line, char const* message) {
        snprintf(__impl, sizeof(__impl), "[%s:%d]%s", file, line, message);
    }

    char const* what() const noexcept override { return __impl; }
};
};  // namespace qlib

#define THROW_EXCEPTION(ok, format_str, ...)                                                       \
    do {                                                                                           \
        if (!(ok)) {                                                                               \
            throw qlib::exception(__FILE__, __LINE__,                                              \
                                  fmt::format(format_str, ##__VA_ARGS__).c_str());                 \
        }                                                                                          \
    } while (false)

#endif

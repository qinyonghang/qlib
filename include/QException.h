#pragma once

#ifdef QNOEXCEPTION

#define QTHROW_EXCEPTION(fmt, ...)
#define QMTHROW_EXCEPTION(ok, fmt, ...)
#define QCTHROW_EXCEPTION(ok, fmt, ...)
#define QCMTHROW_EXCEPTION(ok, fmt, ...)

#else

#include <cstdio>
#include <exception>
#include <string>
#include <typeinfo>

#include "spdlog/spdlog.h"

namespace qlib {

#define QMESSAGE_MAX_LENGTH (512)

class QException final : public std::exception {
protected:
    char __what[QMESSAGE_MAX_LENGTH];

public:
    QException(char const* file, int line, char const* message) {
        snprintf(__what, sizeof(__what), "[%s:%d]%s", file, line, message);
    }

    char const* what() const noexcept override { return __what; }
};

template <class format_string_t, class... Args>
void qthrow_exception(char const* file, int line, format_string_t fmt, Args&&... args) {
    throw QException(file, line, fmt::format(fmt, std::forward<Args>(args)...).c_str());
}

};  // namespace qlib

#define QTHROW_EXCEPTION(ok, fmt, ...)                                                             \
    do {                                                                                           \
        if (!(ok)) {                                                                               \
            qlib::qthrow_exception(__FILE__, __LINE__, fmt, ##__VA_ARGS__);                        \
        }                                                                                          \
    } while (false)

#define QMTHROW_EXCEPTION(ok, fmt, ...) QTHROW_EXCEPTION(ok, "{}: " fmt, __func__, ##__VA_ARGS__)

#define QCTHROW_EXCEPTION(ok, fmt, ...)                                                            \
    QTHROW_EXCEPTION(ok, "{}: " fmt, typeid(*this).name(), ##__VA_ARGS__)

#define QCMTHROW_EXCEPTION(ok, fmt, ...)                                                           \
    QTHROW_EXCEPTION(ok, "{}::{}: " fmt, typeid(*this).name(), __func__, ##__VA_ARGS__)

#endif

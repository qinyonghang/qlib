#pragma once

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {
template <class T, class Enable = void>
struct converter : public object {
    static T decode(std::string_view s) { return T{s}; }
};

template <>
struct converter<bool_t> : public object {
    static bool_t decode(std::string_view s) {
        bool_t result{False};

        if (s == "true" || s == "True") {
            result = True;
        } else if (s == "false" || s == "False") {
            result = False;
        } else {
            THROW_EXCEPTION(False, "Value({}) is Invalid Bool!", s.data());
        }

        return result;
    }
};

template <class T>
struct converter<T,
                 std::enable_if_t<is_one_of_v<T,
                                              int8_t,
                                              int16_t,
                                              int32_t,
                                              int64_t,
                                              ssize_t,
                                              uint8_t,
                                              uint16_t,
                                              uint32_t,
                                              uint64_t,
                                              size_t>>> : public object {
    static T decode(std::string_view s) {
        T result;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result);
        THROW_EXCEPTION(ec == std::errc{} && ptr != nullptr && *ptr == '\0',
                        "Value({}) is Invalid {}! Err={}!", s.data(), typeid(T).name(),
                        static_cast<int32_t>(ec));
        return result;
    }
};

};  // namespace qlib

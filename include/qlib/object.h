#pragma once

#include <memory>
#include <string>

namespace qlib {
using string = std::string;
using byte = std::uint8_t;
using int8_t = std::int8_t;
using uint8_t = std::uint8_t;
using int16_t = std::int16_t;
using uint16_t = std::uint16_t;
using int32_t = std::int32_t;
using uint32_t = std::uint32_t;
using int64_t = std::int64_t;
using uint64_t = std::uint64_t;
using float32_t = float;
using float64_t = double;

template <typename T>
class object {
public:
    enum : int32_t {
        OK = 0,
        UNKNOWN_ERROR = -1,
        IMPL_NULLPTR = -2,
        PARAM_INVALID = -3,
        FILE_NOT_FOUND = -4,
        FILE_NOT_SUPPORT = -5,
        FILE_INVALID = -6,
        YAML_PARSE_ERROR = -7,
        ERRORCODE_MAX = -8
    };

    using self = object<T>;
    using ptr = std::shared_ptr<self>;

protected:
    struct impl {};
    using impl_ptr = std::shared_ptr<impl>;

    impl_ptr __impl;

public:
    T& derived() { return static_cast<T&>(*this); }

    explicit object() = default;

    template <class... Args>
    int32_t init(Args&&... args) {
        return derived().init(std::forward<Args>(args)...);
    }
};

template <class T>
using object_ptr = typename object<T>::ptr;

};  // namespace qlib

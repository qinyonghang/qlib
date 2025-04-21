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

struct position {
    float64_t longitude;
    float64_t latitude;
    float64_t relative_height; /*! relative to takeoff height*/
};

enum error_code : int32_t {
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

template <typename T>
class object {
public:
    using string = qlib::string;
    using int8_t = qlib::int8_t;
    using uint8_t = qlib::uint8_t;
    using int16_t = qlib::int16_t;
    using uint16_t = qlib::uint16_t;
    using int32_t = qlib::int32_t;
    using uint32_t = qlib::uint32_t;
    using int64_t = qlib::int64_t;
    using uint64_t = qlib::uint64_t;
    using float32_t = qlib::float32_t;
    using float64_t = qlib::float64_t;

    using self = object<void>;
    using ptr = std::shared_ptr<self>;
};

template <class T>
using object_ptr = typename object<T>::ptr;

};  // namespace qlib

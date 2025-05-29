#pragma once

#define OBJECT_IMPLEMENTATION

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

enum class error : int32_t {
    unknown = -1,
    impl_nullptr = -2,
    param_invalid = -3,
    file_not_found = -4,
    file_not_support = -5,
    file_invalid = -6
};

class object {
protected:
    object() = default;

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

    struct parameter {};

    using self = object;
    using ptr = std::shared_ptr<self>;
};

static inline bool likely(bool ok) {
#ifdef __glibc_likely
    return __glibc_likely(ok);
#else
    return ok;
#endif
}

static inline bool unlikely(bool ok) {
#ifdef __glibc_unlikely
    return __glibc_unlikely(ok);
#else
    return ok;
#endif
}

};  // namespace qlib

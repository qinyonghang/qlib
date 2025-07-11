#pragma once

#define OBJECT_IMPLEMENTATION

#include <memory>
#include <string>
#include <type_traits>

namespace qlib {
using string_t = std::string;
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

using bool_t = bool;
constexpr bool_t True = true;
constexpr bool_t False = false;

template <class T>
using sptr = std::shared_ptr<T>;

template <class T>
using uptr = std::unique_ptr<T>;

// struct position {
//     float64_t longitude;
//     float64_t latitude;
//     float64_t relative_height; /*! relative to takeoff height*/
// };

// enum : int32_t {
//     OK = 0,
//     UNKNOWN_ERROR = -1,
//     IMPL_NULLPTR = -2,
//     PARAM_INVALID = -3,
//     FILE_NOT_FOUND = -4,
//     FILE_NOT_SUPPORT = -5,
//     FILE_INVALID = -6,
//     YAML_PARSE_ERROR = -7,
//     ERRORCODE_MAX = -8
// };

enum class error : int32_t {
    unknown = -1,
    impl_nullptr = -2,
    param_invalid = -3,
    file_not_found = -4,
    file_not_support = -5,
    file_invalid = -6
};

class object {
public:
    using string_t = qlib::string_t;
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

    using bool_t = qlib::bool_t;
    constexpr static inline bool_t True = qlib::True;
    constexpr static inline bool_t False = qlib::False;

    struct parameter {
        using self = parameter;
        using ptr = std::shared_ptr<self>;
    };

    using self = object;
    using ptr = qlib::sptr<self>;

    template <class T>
    using sptr = qlib::sptr<T>;

    template <class T>
    using uptr = qlib::uptr<T>;

    ~object() noexcept = default;

protected:
    constexpr object() noexcept = default;
};

constexpr static inline bool likely(bool ok) {
#ifdef __glibc_likely
    return __glibc_likely(ok);
#else
    return ok;
#endif
}

constexpr static inline bool unlikely(bool ok) {
#ifdef __glibc_unlikely
    return __glibc_unlikely(ok);
#else
    return ok;
#endif
}

template <class T, class... Ts>
struct is_one_of final : std::disjunction<std::is_same<T, Ts>...> {};

template <class T, class... Ts>
inline constexpr bool is_one_of_v = is_one_of<T, Ts...>::value;

};  // namespace qlib

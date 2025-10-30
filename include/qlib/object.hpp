#ifndef QLIB_OBJECT_HPP
#define QLIB_OBJECT_HPP

#if (defined(__GNUC__) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define _cpp17_
#endif

#define NODISCARD [[nodiscard]]
#if defined(__GNUC__) || defined(__clang__)
#define ATTR_UNUSED __attribute__((__unused__))
#else
#define ATTR_UNUSED
#endif
#if defined(_cpp17_)
#if defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define CONSTEXPR constexpr
#else
#define ALWAYS_INLINE inline
#define CONSTEXPR
#endif
#else
#define ALWAYS_INLINE inline
#define CONSTEXPR
#endif

namespace qlib {

using int8_t = char;
using uint8_t = unsigned char;
using int16_t = short;
using uint16_t = unsigned short;
using int32_t = int;
using uint32_t = unsigned int;
using int64_t = long long;
using uint64_t = unsigned long long;
using float32_t = float;
using float64_t = double;

using nullptr_t = decltype(nullptr);
using ptrdiff_t = decltype((uint64_t*)0 - (uint64_t*)0);
using size_t = decltype(sizeof(0));

using bool_t = bool;
constexpr bool_t True = true;
constexpr bool_t False = false;

ALWAYS_INLINE CONSTEXPR int32_t _memcmp_(void const* __s1, void const* __s2, size_t __l) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_memcmp(__s1, __s2, __l);
#elif defined(_MSC_VER) && (defined(_STRING_H) || defined(_CSTRING_))
    return memcmp(__s1, __s2, __l);
#else
    auto __u1 = (uint8_t*)__s1;
    auto __u2 = (uint8_t*)__s2;
    int32_t result{0u};
    for (size_t i = 0u; i < __l; ++i) {
        if (__u1[i] != __u2[i]) {
            result = __u1[i] < __u2[i] ? -1 : 1;
            break;
        }
    }
    return result;
#endif
}

ALWAYS_INLINE CONSTEXPR void* _memmove_(void* __dst, void const* __src, size_t __l) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_memmove(__dst, __src, __l);
#elif defined(_MSC_VER) && (defined(_STRING_H) || defined(_CSTRING_))
    return memmove(__dst, __src, __l);
#else
    if (__dst <= __src || (uint8_t*)__dst >= (uint8_t*)__src + __l) {
        uint8_t* __d = (uint8_t*)__dst;
        uint8_t const* __s = (uint8_t const*)__src;
        for (size_t i = 0; i < __l; ++i) {
            __d[i] = __s[i];
        }
    } else {
        uint8_t* __d = (uint8_t*)__dst + __l;
        uint8_t const* __s = (uint8_t const*)__src + __l;
        for (size_t i = 0; i < __l; ++i) {
            --__d;
            --__s;
            *__d = *__s;
        }
    }
    return __dst;
#endif
}

ALWAYS_INLINE CONSTEXPR void* _memcpy_(void* __dst, void const* __src, size_t __l) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_memcpy(__dst, __src, __l);
#elif defined(_MSC_VER) && (defined(_STRING_H) || defined(_CSTRING_))
    return memcpy(__dst, __src, __l);
#else
    for (size_t i = 0; i < __l; ++i) {
        ((uint8_t*)__dst)[i] = ((uint8_t const*)__src)[i];
    }
    return __dst;
#endif
}

ALWAYS_INLINE CONSTEXPR bool_t _isnan_(float64_t __v) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_isnan(__v);
#elif defined(_MSC_VER) && (defined(_CMATH_))
    return isnan(__v);
#else
    union {
        float64_t f;
        uint64_t bits;
    } u = {__v};

    uint64_t const exp_mask = 0x7FF0000000000000ULL;
    uint64_t exp_bits = u.bits & exp_mask;

    uint64_t const mantissa_mask = 0x000FFFFFFFFFFFFFULL;
    uint64_t mantissa_bits = u.bits & mantissa_mask;

    return (exp_bits == exp_mask) && (mantissa_bits != 0ULL);
#endif
}

ALWAYS_INLINE CONSTEXPR bool_t _isinf_(float64_t __v) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_isinf(__v);
#elif defined(_MSC_VER) && (defined(_CMATH_))
    return isinf(__v);
#else
    union {
        float64_t f;
        uint64_t bits;
    } u = {__v};

    uint64_t const exp_mask = 0x7FF0000000000000ULL;
    uint64_t exp_bits = u.bits & exp_mask;

    uint64_t const mantissa_mask = 0x000FFFFFFFFFFFFFULL;
    uint64_t mantissa_bits = u.bits & mantissa_mask;

    return (exp_bits == exp_mask) && (mantissa_bits == 0ULL);
#endif
}

class object {
public:
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

    using nullptr_t = qlib::nullptr_t;
    using ptrdiff_t = qlib::ptrdiff_t;
    using size_t = qlib::size_t;

    using bool_t = qlib::bool_t;
    constexpr static bool_t True{qlib::True};
    constexpr static bool_t False{qlib::False};

protected:
    ALWAYS_INLINE CONSTEXPR object() noexcept = default;
};

template <class T>
class reference : public object {
public:
    using base = object;
    using self = reference;
    using value_type = T;

protected:
    value_type* _impl;

public:
    reference() noexcept = delete;

    ALWAYS_INLINE CONSTEXPR reference(reference const&) noexcept = default;
    ALWAYS_INLINE CONSTEXPR reference(reference&&) noexcept = default;
    ALWAYS_INLINE CONSTEXPR reference& operator=(reference const&) noexcept = default;
    ALWAYS_INLINE CONSTEXPR reference& operator=(reference&&) noexcept = default;

    ALWAYS_INLINE CONSTEXPR reference(value_type& allocator) noexcept : _impl(&allocator) {}
    ALWAYS_INLINE CONSTEXPR reference(value_type* allocator) noexcept : _impl(allocator) {}
    NODISCARD ALWAYS_INLINE CONSTEXPR operator value_type&() noexcept { return *_impl; }
};

template <class _Tp>
struct traits : public object {
    using reference = _Tp&;
};

#ifdef _GLIBCXX_STD_FUNCTION_H
template <class _Res, class... _Args>
struct traits<std::function<_Res(_Args...)>> : public object {
    using reference = std::function<_Res(_Args...)>;
};
#endif

NODISCARD ALWAYS_INLINE CONSTEXPR auto likely(bool_t ok) noexcept {
#ifdef __GNUC__
    return __builtin_expect(ok, 1);
#else
    return ok;
#endif
}

NODISCARD ALWAYS_INLINE CONSTEXPR auto unlikely(bool_t ok) noexcept {
#ifdef __GNUC__
    return __builtin_expect(ok, 0);
#else
    return ok;
#endif
}

class exception : public object {
public:
    virtual char const* what() const noexcept { return nullptr; }
};

template <typename _Tp, typename _Up = _Tp&&>
_Up __declval(int);

template <typename _Tp>
_Tp __declval(long);

template <typename _Tp>
auto declval() noexcept -> decltype(__declval<_Tp>(0));

// template <typename _Tp>
// auto declval() noexcept -> decltype(__declval<_Tp>(0)) {
//     return __declval<_Tp>(0);
// }

template <class T, T _value>
struct constant {
    constexpr static T value = _value;
    using value_type = T;
    using type = constant<T, _value>;
};

template <bool_t _value>
struct bool_constant : public constant<bool_t, _value> {};

struct true_type : public bool_constant<True> {};

struct false_type : public bool_constant<False> {};

/// remove_cv
template <typename _Tp>
struct remove_cv {
    using type = _Tp;
};

template <typename _Tp>
struct remove_cv<const _Tp> {
    using type = _Tp;
};

template <typename _Tp>
struct remove_cv<volatile _Tp> {
    using type = _Tp;
};

template <typename _Tp>
struct remove_cv<const volatile _Tp> {
    using type = _Tp;
};

template <typename _Tp>
using remove_cv_t = typename remove_cv<_Tp>::type;

template <bool_t, class T = void>
struct enable_if {};

template <class T>
struct enable_if<True, T> {
    using type = T;
};

template <bool_t Enable, class T = void>
using enable_if_t = typename enable_if<Enable, T>::type;

template <class T>
struct _is_signed_helper : public false_type {};
template <>
struct _is_signed_helper<int8_t> : public true_type {};
template <>
struct _is_signed_helper<int16_t> : public true_type {};
template <>
struct _is_signed_helper<int32_t> : public true_type {};
template <>
struct _is_signed_helper<int64_t> : public true_type {};

template <class T>
struct is_signed : public _is_signed_helper<remove_cv_t<T>> {};
template <class T>
constexpr static bool_t is_signed_v = is_signed<T>::value;

template <class T>
struct _is_unsigned_helper : public false_type {};
template <>
struct _is_unsigned_helper<uint8_t> : public true_type {};
template <>
struct _is_unsigned_helper<uint16_t> : public true_type {};
template <>
struct _is_unsigned_helper<uint32_t> : public true_type {};
template <>
struct _is_unsigned_helper<uint64_t> : public true_type {};
#ifdef __GNUC__
template <>
struct _is_unsigned_helper<size_t> : public true_type {};
#endif

template <class T>
struct is_unsigned : public _is_unsigned_helper<remove_cv_t<T>> {};
template <class T>
constexpr static bool_t is_unsigned_v = is_unsigned<T>::value;

template <class T>
struct _is_integral_helper : public false_type {};
// template <>
// struct _is_integral_helper<bool_t> : public true_type {};
template <>
struct _is_integral_helper<int8_t> : public true_type {};
template <>
struct _is_integral_helper<uint8_t> : public true_type {};
template <>
struct _is_integral_helper<int16_t> : public true_type {};
template <>
struct _is_integral_helper<uint16_t> : public true_type {};
template <>
struct _is_integral_helper<int32_t> : public true_type {};
template <>
struct _is_integral_helper<uint32_t> : public true_type {};
template <>
struct _is_integral_helper<int64_t> : public true_type {};
template <>
struct _is_integral_helper<uint64_t> : public true_type {};

template <class T>
struct is_integral : public _is_integral_helper<remove_cv_t<T>> {};
template <class T>
constexpr static bool_t is_integral_v = is_integral<T>::value;

template <class T>
struct _is_floating_point_helper : public false_type {};
template <>
struct _is_floating_point_helper<float32_t> : public true_type {};
template <>
struct _is_floating_point_helper<float64_t> : public true_type {};

template <class T>
struct is_floating_point : public _is_floating_point_helper<remove_cv_t<T>> {};
template <class T>
constexpr static bool_t is_floating_point_v = is_floating_point<T>::value;

template <class, class>
struct is_same {
    constexpr static bool_t value{False};
};

template <class T>
struct is_same<T, T> {
    constexpr static bool_t value{True};
};

template <class T, class U>
constexpr static bool_t is_same_v = is_same<T, U>::value;

template <class _Tp, class... _Args>
using __is_nothrow_constructible_impl = bool_constant<__is_nothrow_constructible(_Tp, _Args...)>;
/// @endcond

template <class _Tp, class... _Args>
struct is_nothrow_constructible : public __is_nothrow_constructible_impl<_Tp, _Args...>::type {};

template <class T, class... Args>
constexpr static bool_t is_nothrow_constructible_v = is_nothrow_constructible<T, Args...>::value;

template <bool_t, class, class>
struct conditional;

template <class true_type, class false_type>
struct conditional<True, true_type, false_type> {
    using type = true_type;
};

template <class true_type, class false_type>
struct conditional<False, true_type, false_type> {
    using type = false_type;
};

template <bool_t _B, class true_type, class false_type>
using conditional_t = typename conditional<_B, true_type, false_type>::type;

template <class...>
struct __or_;

template <>
struct __or_<> : public false_type {};

template <class _B1>
struct __or_<_B1> : public _B1 {};

template <class _B1, class _B2>
struct __or_<_B1, _B2> : public conditional<_B1::value, _B1, _B2>::type {};

template <class _B1, class _B2, class... _Bn>
struct __or_<_B1, _B2, _Bn...> : public conditional<_B1::value, _B1, __or_<_B2, _Bn...>>::type {};

template <class...>
struct __and_;

template <>
struct __and_<> : public true_type {};

template <class _B1>
struct __and_<_B1> : public _B1 {};

template <class _B1, class _B2>
struct __and_<_B1, _B2> : public conditional<_B1::value, _B2, _B1>::type {};

template <class _B1, class _B2, class... _Bn>
struct __and_<_B1, _B2, _Bn...> : public conditional<_B1::value, __and_<_B2, _Bn...>, _B1>::type {};

template <class _Pp>
struct __not_ : public bool_constant<!bool(_Pp::value)> {};

/// extent
template <typename, unsigned = 0>
struct extent;

template <typename, unsigned _Uint>
struct extent : public constant<uint64_t, 0> {};

template <typename _Tp, unsigned _Uint, uint64_t _Size>
struct extent<_Tp[_Size], _Uint>
        : public constant<uint64_t, _Uint == 0 ? _Size : extent<_Tp, _Uint - 1>::value> {};

template <typename _Tp, unsigned _Uint>
struct extent<_Tp[], _Uint>
        : public constant<uint64_t, _Uint == 0 ? 0 : extent<_Tp, _Uint - 1>::value> {};

template <typename _Tp>
struct remove_all_extents {
    typedef _Tp type;
};

template <typename _Tp, uint64_t _Size>
struct remove_all_extents<_Tp[_Size]> {
    typedef typename remove_all_extents<_Tp>::type type;
};

template <typename _Tp>
struct remove_all_extents<_Tp[]> {
    typedef typename remove_all_extents<_Tp>::type type;
};

template <class _Tp>
using remove_all_extents_t = typename remove_all_extents<_Tp>::type;

/// is_void
template <typename>
struct is_void : public false_type {};

template <>
struct is_void<void> : public true_type {};

/// is_array
template <typename>
struct is_array : public false_type {};

template <typename _Tp, uint64_t _Size>
struct is_array<_Tp[_Size]> : public true_type {};

template <typename _Tp>
struct is_array<_Tp[]> : public true_type {};

/// is_const
template <typename>
struct is_const : public false_type {};

template <typename _Tp>
struct is_const<_Tp const> : public true_type {};

/// is_function
template <typename _Tp>
struct is_function : public bool_constant<!is_const<const _Tp>::value> {};

template <typename _Tp>
struct is_function<_Tp&> : public false_type {};

template <typename _Tp>
struct is_function<_Tp&&> : public false_type {};

template <typename _Tp>
struct __is_array_unknown_bounds : public __and_<is_array<_Tp>, __not_<extent<_Tp>>> {};

/// is_lvalue_reference
template <typename>
struct is_lvalue_reference : public false_type {};

template <typename _Tp>
struct is_lvalue_reference<_Tp&> : public true_type {};

/// is_rvalue_reference
template <typename>
struct is_rvalue_reference : public false_type {};

template <typename _Tp>
struct is_rvalue_reference<_Tp&&> : public true_type {};

/// is_reference
template <typename _Tp>
struct is_reference : public __or_<is_lvalue_reference<_Tp>, is_rvalue_reference<_Tp>>::type {};

template <typename _Tp>
struct is_arithmetic : public __or_<is_integral<_Tp>, is_floating_point<_Tp>>::type {};

template <class _Tp>
constexpr static bool_t is_arithmetic_v = is_arithmetic<_Tp>::value;

template <class _Tp>
struct _is_pointer_helper : public false_type {};

template <typename _Tp>
struct _is_pointer_helper<_Tp*> : public true_type {};

#if (defined(__GNUC__) && defined(_UNIQUE_PTR_H)) || (defined(_MSC_VER) && defined(_MEMORY_))
template <class _Tp, class _Dp>
struct _is_pointer_helper<std::unique_ptr<_Tp, _Dp>> : public true_type {};
#endif

#if (defined(__GNUC__) && defined(_SHARED_PTR_H)) || (defined(_MSC_VER) && defined(_MEMORY_))
template <class _Tp>
struct _is_pointer_helper<std::shared_ptr<_Tp>> : public true_type {};
#endif

template <class _Tp>
struct is_pointer : public _is_pointer_helper<remove_cv_t<_Tp>>::type {};

template <class _Tp>
constexpr static bool_t is_pointer_v = is_pointer<_Tp>::value;

template <typename _Tp>
struct __is_member_pointer_helper : public false_type {};

template <typename _Tp, typename _Cp>
struct __is_member_pointer_helper<_Tp _Cp::*> : public true_type {};

template <typename _Tp>
struct is_member_pointer : public __is_member_pointer_helper<remove_cv_t<_Tp>>::type {};

template <typename>
struct __is_null_pointer_helper : public false_type {};

template <>
struct __is_null_pointer_helper<decltype(nullptr)> : public true_type {};

template <typename _Tp>
struct is_null_pointer : public __is_null_pointer_helper<remove_cv_t<_Tp>>::type {};

template <typename _Tp>
struct is_scalar : public __or_<is_arithmetic<_Tp>,
                                // is_enum<_Tp>,
                                is_pointer<_Tp>,
                                is_member_pointer<_Tp>,
                                is_null_pointer<_Tp>>::type {};

template <typename _Tp>
struct __is_nt_destructible_impl {
    using type = bool_constant<noexcept(declval<_Tp&>().~_Tp())>;
};

template <typename _Tp,
          bool = __or_<is_void<_Tp>, __is_array_unknown_bounds<_Tp>, is_function<_Tp>>::value,
          bool = __or_<is_reference<_Tp>, is_scalar<_Tp>>::value>
struct __is_nt_destructible_safe;

template <typename _Tp>
struct __is_nt_destructible_safe<_Tp, False, False>
        : public __is_nt_destructible_impl<remove_all_extents_t<_Tp>>::type {};

template <typename _Tp>
struct __is_nt_destructible_safe<_Tp, True, False> : public false_type {};

template <typename _Tp>
struct __is_nt_destructible_safe<_Tp, False, True> : public true_type {};

template <typename _Tp>
struct is_nothrow_destructible : public __is_nt_destructible_safe<_Tp>::type {};

template <class T>
constexpr static bool_t is_nothrow_destructible_v = is_nothrow_destructible<T>::value;

// template <class T>
// struct is_nothrow_default_constructible {
//     constexpr static bool_t value{noexcept(T())};
// };

// template <class T>
// constexpr static bool_t is_nothrow_default_constructible_v =
//     is_nothrow_default_constructible<T>::value;

template <class T>
struct remove_reference {
    using type = T;
};

template <class T>
struct remove_reference<T&> {
    using type = T;
};

template <class T>
struct remove_reference<T&&> {
    using type = T;
};

template <class T>
using remove_reference_t = typename remove_reference<T>::type;

template <class _Tp>
using remove_cvref_t = remove_cv_t<remove_reference_t<_Tp>>;

template <class...>
using void_t = void;

template <typename _Iterator, typename = void_t<>>
struct _iterator_traits_helper {};

template <typename _Iterator>
struct _iterator_traits_helper<_Iterator,
                               void_t<typename _Iterator::value_type,
                                      typename _Iterator::difference_type,
                                      typename _Iterator::iterator_category,
                                      typename _Iterator::pointer,
                                      typename _Iterator::reference>> {
    typedef typename _Iterator::iterator_category iterator_category;
    typedef typename _Iterator::value_type value_type;
    typedef typename _Iterator::difference_type difference_type;
    typedef typename _Iterator::pointer pointer;
    typedef typename _Iterator::reference reference;
};

template <typename _Iterator>
struct iterator_traits : public _iterator_traits_helper<_Iterator> {};

struct input_iterator_tag {};

struct output_iterator_tag {};

struct forward_iterator_tag : public input_iterator_tag {};

struct bidirectional_iterator_tag : public forward_iterator_tag {};

struct random_access_iterator_tag : public bidirectional_iterator_tag {};

template <typename _Tp>
struct iterator_traits<_Tp*> {
    typedef random_access_iterator_tag iterator_category;
    typedef _Tp value_type;
    typedef ptrdiff_t difference_type;
    typedef _Tp* pointer;
    typedef _Tp& reference;
};

template <typename _Tp>
struct iterator_traits<const _Tp*> {
    typedef random_access_iterator_tag iterator_category;
    typedef _Tp value_type;
    typedef ptrdiff_t difference_type;
    typedef const _Tp* pointer;
    typedef const _Tp& reference;
};

template <class _Tp>
using iterator_difference_t = typename iterator_traits<_Tp>::difference_type;

template <class _Tp>
using iterator_category_t = typename iterator_traits<_Tp>::iterator_category;

template <class T>
struct is_trivially_copyable : public bool_constant<__is_trivially_copyable(T)> {};

template <class T>
constexpr static bool_t is_trivially_copyable_v = is_trivially_copyable<T>::value;

template <class _Base, class _Derived>
struct is_base_of : public bool_constant<__is_base_of(_Base, _Derived)> {};

template <class _Base, class _Derived>
constexpr static bool_t is_base_of_v = is_base_of<_Base, _Derived>::value;

template <class _From,
          class _To,
          bool = __or_<is_void<_From>, is_function<_To>, is_array<_To>>::value>
struct __is_convertible_helper {
    typedef class is_void<_To>::type type;
};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#endif
template <class _From, class _To>
class __is_convertible_helper<_From, _To, false> {
    template <class _To1>
    static void __test_aux(_To1) noexcept;

    template <class _From1, class _To1, class = decltype(__test_aux<_To1>(declval<_From1>()))>
    static true_type __test(int);

    template <class, class>
    static false_type __test(...);

public:
    typedef decltype(__test<_From, _To>(0)) type;
};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

template <class _From, class _To>
struct is_convertible : public __is_convertible_helper<_From, _To>::type {};

template <class _From, class _To>
constexpr static bool_t is_convertible_v = is_convertible<_From, _To>::value;

template <class _Tp>
struct _is_container_helper_ : public false_type {};

#ifdef _GLIBCXX_VECTOR
template <class _Tp>
struct _is_container_helper_<std::vector<_Tp>> : public true_type {};
#endif

#ifdef _GLIBCXX_ARRAY
template <class _Tp, size_t _Nm>
struct _is_container_helper_<std::array<_Tp, _Nm>> : public true_type {};
#endif

#ifdef _GLIBCXX_LIST
template <class _Tp>
struct _is_container_helper_<std::list<_Tp>> : public true_type {};
#endif

#ifdef _GLIBCXX_QUEUE
template <class _Tp>
struct _is_container_helper_<std::queue<_Tp>> : public true_type {};
#endif

template <class _Tp>
struct is_container : public _is_container_helper_<remove_cv_t<_Tp>>::type {};

template <class _Tp>
constexpr static bool_t is_container_v = is_container<_Tp>::value;

template <class T>
struct unsigned_traits;

template <>
struct unsigned_traits<int8_t> {
    using type = uint8_t;
};

template <>
struct unsigned_traits<int16_t> {
    using type = uint16_t;
};

template <>
struct unsigned_traits<int32_t> {
    using type = uint32_t;
};

template <>
struct unsigned_traits<int64_t> {
    using type = uint64_t;
};

ALWAYS_INLINE constexpr bool_t _is_constant_evaluated_(int) noexcept {
    return True;
}

ALWAYS_INLINE bool_t _is_constant_evaluated_(...) noexcept {
    return False;
}

ALWAYS_INLINE constexpr auto is_constant_evaluated() {
    return _is_constant_evaluated_(0);
}

template <class _Tp>
ALWAYS_INLINE constexpr _Tp&& forward(remove_reference_t<_Tp>& t) noexcept {
    return static_cast<_Tp&&>(t);
}

template <class _Tp>
ALWAYS_INLINE constexpr _Tp&& forward(remove_reference_t<_Tp>&& t) noexcept {
    return static_cast<_Tp&&>(t);
}

template <class _Tp>
NODISCARD CONSTEXPR remove_reference_t<_Tp>&& move(_Tp&& __t) noexcept {
    return static_cast<remove_reference_t<_Tp>&&>(__t);
}

template <class _Key, class _Value>
struct pair : public object {
    using self = pair;
    using key_type = _Key;
    using value_type = _Value;

    key_type key;
    value_type value;

    pair() = default;
    pair(const pair&) = default;
    pair(pair&&) = default;
    pair& operator=(const pair&) = default;
    pair& operator=(pair&&) = default;

    template <class _uKey, class _uValue>
    ALWAYS_INLINE CONSTEXPR pair(_uKey&& __key, _uValue&& __value)
            : key(qlib::forward<_uKey>(__key)), value(qlib::forward<_uValue>(__value)) {}
};

template <class Tag>
struct _distance_helper {
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE static CONSTEXPR auto call(Iter1 __first, Iter2 __last) noexcept {
        typename iterator_traits<Iter1>::difference_type __n = 0;
        while (likely(__first != __last)) {
            ++__n;
            ++__first;
        }
        return __n;
    }
};

template <>
struct _distance_helper<random_access_iterator_tag> {
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE static constexpr auto call(Iter1 __first, Iter2 __last) noexcept {
        return __last - __first;
    }
};

template <class Iter1, class Iter2>
NODISCARD ALWAYS_INLINE constexpr auto distance(Iter1 __first, Iter2 __last) noexcept {
    return _distance_helper<iterator_category_t<Iter1>>::call(__first, __last);
}

template <bool_t simple>
struct _equal_helper;

template <>
struct _equal_helper<False> {
    template <class Iter1, class Iter2, class Iter3>
    NODISCARD ALWAYS_INLINE static constexpr bool_t call(Iter1 begin1, Iter2 end1, Iter3 begin2) {
        if (likely(begin1 != begin2)) {
            while ((begin1 != end1)) {
                if (unlikely(*begin1 != *begin2)) {
                    return False;
                }
                ++begin1;
                ++begin2;
            }
        }

        return True;
    }
};

template <>
struct _equal_helper<True> {
    template <class T>
    NODISCARD ALWAYS_INLINE static constexpr bool_t call(T const* begin1,
                                                         T const* end1,
                                                         T const* begin2) {
        if (is_constant_evaluated()) {
            return _equal_helper<False>::call(begin1, end1, begin2);
        }

        const auto size = distance(begin1, end1);
        if (likely(begin1 != begin2 && size > 0)) {
            return _memcmp_(begin1, begin2, sizeof(T) * size) == 0;
        }
        return True;
    }
};

template <class Iter1, class Iter2, class Iter3>
NODISCARD ALWAYS_INLINE CONSTEXPR bool_t equal(Iter1 begin1, Iter2 end1, Iter3 begin2) {
    using value_type1 = typename iterator_traits<Iter1>::value_type;
    using value_type2 = typename iterator_traits<Iter2>::value_type;
    using value_type3 = typename iterator_traits<Iter3>::value_type;
    constexpr auto simple = is_trivially_copyable_v<value_type1> &&
        is_trivially_copyable_v<value_type2> && is_trivially_copyable_v<value_type3>;
    return _equal_helper<simple>::call(begin1, end1, begin2);
}

template <bool_t simple>
struct _copy_helper;

template <>
struct _copy_helper<False> {
    template <class Iter1, class Iter2, class Iter3>
    ALWAYS_INLINE CONSTEXPR static Iter3 call(Iter1 begin1, Iter2 end1, Iter3 begin2) noexcept {
        if (likely(begin1 != begin2)) {
            while (likely(begin1 != end1)) {
                *begin2 = *begin1;
                ++begin1;
                ++begin2;
            }
        }
        return begin2;
    }
};

template <>
struct _copy_helper<True> {
    template <class T>
    ALWAYS_INLINE static CONSTEXPR T* call(T const* begin1, T const* end1, T* begin2) noexcept {
        if (is_constant_evaluated()) {
            return _copy_helper<False>::call(begin1, end1, begin2);
        }
        const auto size = distance(begin1, end1);
        if (likely(begin1 != begin2 && size > 0)) {
            _memmove_(begin2, begin1, sizeof(T) * size);
        }
        return begin2 + size;
    }
};

template <class Iter1, class Iter2, class Iter3>
ALWAYS_INLINE CONSTEXPR Iter3 copy(Iter1 begin1, Iter2 end1, Iter3 begin2) noexcept {
    using value_type1 = typename iterator_traits<Iter1>::value_type;
    using value_type2 = typename iterator_traits<Iter2>::value_type;
    using value_type3 = typename iterator_traits<Iter3>::value_type;
    constexpr auto simple = is_trivially_copyable_v<value_type1> &&
        is_trivially_copyable_v<value_type2> && is_trivially_copyable_v<value_type3>;
    return _copy_helper<simple>::call(begin1, end1, begin2);
}

template <class Char>
NODISCARD ALWAYS_INLINE CONSTEXPR auto is_digit(Char c) -> bool_t {
    return c >= '0' && c <= '9';
}

// template <class T, class Enable = enable_if_t<is_unsigned_v<T>>>
// ALWAYS_INLINE CONSTEXPR auto len(T value) -> size_t {
//     size_t __n = 1;
//     constexpr T __b1 = 10u;
//     constexpr T __b2 = 100u;
//     constexpr T __b3 = 1000u;
//     constexpr T __b4 = 10000u;
//     while (True) {
//         if (value < __b1) {
//             break;
//         }
//         if (value < __b2) {
//             __n += 1;
//             break;
//         }
//         if (value < __b3) {
//             __n += 2;
//             break;
//         }
//         if (value < __b4) {
//             __n += 3;
//             break;
//         }
//         value /= __b4;
//         __n += 4;
//     }
//     return __n;
// }

NODISCARD ALWAYS_INLINE CONSTEXPR size_t len(uint8_t __value) noexcept {
    return __value < 10u ? 1 : (__value < 100u ? 2u : 3u);
}

NODISCARD ALWAYS_INLINE CONSTEXPR size_t len(int8_t __v) noexcept {
    return __v >= 0 ? len(uint8_t(__v)) : len(uint8_t(uint8_t(~__v) + 1u)) + 1u;
}

NODISCARD ALWAYS_INLINE CONSTEXPR size_t len(uint16_t __value) noexcept {
    return __value < 10u
        ? 1
        : (__value < 100u ? 2u : (__value < 1000u ? 3u : (__value < 10000u ? 4u : 5u)));
}

NODISCARD ALWAYS_INLINE CONSTEXPR size_t len(int16_t __v) noexcept {
    return __v >= 0 ? len(uint16_t(__v)) : len(uint16_t(uint16_t(~__v) + 1u)) + 1u;
}

NODISCARD ALWAYS_INLINE CONSTEXPR auto len(uint32_t value) -> size_t {
    if (value < 10u)
        return 1;
    if (value < 100u)
        return 2;
    if (value < 1000u)
        return 3;
    if (value < 10000u)
        return 4;
    if (value < 100000u)
        return 5;
    if (value < 1000000u)
        return 6;
    if (value < 10000000u)
        return 7;
    if (value < 100000000u)
        return 8;
    if (value < 1000000000u)
        return 9;
    return 10;
}

NODISCARD ALWAYS_INLINE CONSTEXPR auto len(int32_t __v) -> size_t {
    return __v >= 0 ? len(uint32_t(__v)) : len(uint32_t(uint32_t(~__v) + 1u)) + 1u;
}

NODISCARD ALWAYS_INLINE CONSTEXPR auto len(uint64_t value) -> size_t {
    if (value < 10u)
        return 1;
    if (value < 100u)
        return 2;
    if (value < 1000u)
        return 3;
    if (value < 10000u)
        return 4;
    if (value < 100000u)
        return 5;
    if (value < 1000000u)
        return 6;
    if (value < 10000000u)
        return 7;
    if (value < 100000000u)
        return 8;
    if (value < 1000000000u)
        return 9;
    if (value < 10000000000u)
        return 10;
    if (value < 100000000000u)
        return 11;
    if (value < 1000000000000u)
        return 12;
    if (value < 10000000000000u)
        return 13;
    if (value < 100000000000000u)
        return 14;
    if (value < 1000000000000000u)
        return 15;
    if (value < 10000000000000000u)
        return 16;
    if (value < 100000000000000000u)
        return 17;
    if (value < 1000000000000000000u)
        return 18;
    if (value < 10000000000000000000u)
        return 19;
    return 20;
}

NODISCARD ALWAYS_INLINE CONSTEXPR auto len(int64_t __v) -> size_t {
    return __v >= 0 ? len(uint64_t(__v)) : len(uint64_t(uint64_t(~__v) + 1u)) + 1u;
}

#ifdef __GNUC__
NODISCARD ALWAYS_INLINE CONSTEXPR auto len(size_t __v) -> size_t {
    return len(uint64_t(__v));
}
#endif

template <class Char>
NODISCARD ALWAYS_INLINE constexpr size_t len(Char const* str) noexcept {
    size_t size{0u};
    while (str[size] != '\0') {
        ++size;
    }
    return size;
}

template <class _Tp>
[[noreturn]] ALWAYS_INLINE CONSTEXPR auto __throw(_Tp&& __e) {
    throw qlib::forward<_Tp>(__e);
}

// template <class _Tp>
// auto __throw(_Tp&& __e ATTR_UNUSED) {}

template <class _Tp>
ALWAYS_INLINE CONSTEXPR auto throw_if(bool_t __ok, _Tp&& __e) {
    if (unlikely(__ok)) {
        __throw(qlib::forward<_Tp>(__e));
    }
}

template <class _Tp>
ALWAYS_INLINE CONSTEXPR auto throw_if_not(bool_t __ok, _Tp&& __e) {
    throw_if(!__ok, qlib::forward<_Tp>(__e));
}

ALWAYS_INLINE CONSTEXPR auto throw_if(bool_t __ok) {
    throw_if(__ok, exception{});
}

ALWAYS_INLINE CONSTEXPR auto throw_if_not(bool_t __ok) {
    throw_if(!__ok);
}

class _str_exception_ : public exception {
protected:
    char const* _impl;

public:
    ALWAYS_INLINE CONSTEXPR _str_exception_(char const* __str) : _impl(__str) {}
    const char* what() const noexcept override { return _impl; }
};

[[noreturn]] ALWAYS_INLINE CONSTEXPR auto __throw(char const* __str) {
    __throw(_str_exception_{__str});
}

ALWAYS_INLINE CONSTEXPR auto throw_if(bool_t __ok, char const* __str) {
    throw_if(__ok, _str_exception_{__str});
}

ALWAYS_INLINE CONSTEXPR auto throw_if_not(bool_t __ok, char const* __str) {
    throw_if(!__ok, _str_exception_{__str});
}

template <size_t _Len, size_t _Align>
struct aligned_storage {
    struct type {
        alignas(_Align) uint8_t __data[_Len];
    };
};

template <class... Args>
struct union_storage final : public object {
    template <class... Ts>
    constexpr static size_t max_of() {
        size_t sizes[] = {sizeof(Ts)...};
        size_t max_size = sizes[0];
        for (size_t i = 1; i < sizeof...(Ts); ++i) {
            if (sizes[i] > max_size)
                max_size = sizes[i];
        }
        return max_size;
    }

    template <class... Ts>
    constexpr static size_t max_align_of() {
        size_t aligns[] = {alignof(Ts)...};
        size_t max_align = aligns[0];
        for (size_t i = 1; i < sizeof...(Ts); ++i) {
            if (aligns[i] > max_align)
                max_align = aligns[i];
        }
        return max_align;
    }

    enum : size_t { max_size = max_of<Args...>(), max_align = max_align_of<Args...>() };

    typename aligned_storage<max_size, max_align>::type _data;
};

template <class _Tp, class _Iter1, class _Iter2>
NODISCARD ALWAYS_INLINE CONSTEXPR auto find(_Iter1 __first, _Iter2 __last, _Tp const& __value) {
    _Iter1 __i{__first};

    while (__i != __last && *__i != __value) {
        ++__i;
    }

    return __i;
}

template <class _Tp, class _Iter1, class _Iter2, class _Equal>
NODISCARD ALWAYS_INLINE CONSTEXPR auto find(_Iter1 __first,
                                            _Iter2 __last,
                                            _Tp const& __value,
                                            _Equal __equal) {
    _Iter1 __i{__first};

    while (__i != __last && !__equal(*__i, __value)) {
        ++__i;
    }

    return __i;
}

#if (defined(__GNUC__) && defined(_TIME_H)) || (defined(_MSC_VER) && defined(_INC_TIME))
NODISCARD ALWAYS_INLINE auto localtime(time_t __t) {
    struct tm local_tm;
#ifdef _MSC_VER
    localtime_s(&local_tm, &__t);
#else
    localtime_r(&__t, &local_tm);
#endif
    return local_tm;
}
#endif

};  // namespace qlib

#endif

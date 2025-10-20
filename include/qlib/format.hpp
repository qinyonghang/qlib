#ifndef QLIB_FORMAT_HPP
#define QLIB_FORMAT_HPP

#include <charconv>
#include <tuple>

#include "qlib/string.h"

namespace qlib {

namespace fmt {

template <class _Char>
class _str;

template <class _Char, class _Enable = enable_if_t<is_trivially_copyable_v<_Char>>>
class buffer final : public object {
public:
    using base = object;
    using self = buffer;
    using char_type = _Char;
    using size_type = uint32_t;
    using allocator_type = memory::new_allocator;

    friend class _str<_Char>;

protected:
    char_type* _impl{nullptr};
    size_type _size{0u};
    size_type _capacity{0u};

    ALWAYS_INLINE CONSTEXPR allocator_type _allocator_() const noexcept { return allocator_type{}; }

    ALWAYS_INLINE CONSTEXPR void _reserve_(size_type __n) {
        if (__n > _capacity) {
            size_type __new{1u};
            while (__new < __n) {
                __new <<= 1u;
            }
            char_type* __p = _allocator_().template allocate<char_type>(__new);
            if (_impl != nullptr) {
                __builtin_memcpy(__p, _impl, _size);
            }
            auto __t = _impl;
            auto __c = _capacity;
            _impl = __p;
            _capacity = __new;
            _allocator_().template deallocate<char_type>(__t, __c);
        }
    }

public:
    buffer(self const&) = delete;
    self& operator=(self const&) = delete;

    ALWAYS_INLINE CONSTEXPR buffer(size_type __n = 512u)
            : _impl(_allocator_().template allocate<char_type>(__n)), _size(0u), _capacity(__n) {}

    ALWAYS_INLINE CONSTEXPR buffer(self&& __o)
            : _impl(__o._impl), _size(__o._size), _capacity(__o._capacity) {
        __o._impl = nullptr;
        __o._size = 0u;
        __o._capacity = 0u;
    }

    ALWAYS_INLINE ~buffer() {
        if (_impl != nullptr) {
            _allocator_().deallocate(_impl, _capacity);
            _impl = nullptr;
            _size = 0u;
            _capacity = 0u;
        }
    }

    ALWAYS_INLINE CONSTEXPR self& operator=(self&& __o) {
        if (this != &__o) {
            auto __t = _impl;
            auto __c = _capacity;
            _impl = __o._impl;
            _size = __o._size;
            _capacity = __o._capacity;
            __o._impl = nullptr;
            __o._size = 0u;
            __o._capacity = 0u;
            _allocator_().template deallocate<char_type>(__t, __c);
        }
        return *this;
    }

    ALWAYS_INLINE CONSTEXPR void emplace(char_type __val) {
        _reserve_(_size + 1u);
        _impl[_size] = __val;
        ++_size;
    }

    template <class _Iter1, class _Iter2>
    ALWAYS_INLINE CONSTEXPR void emplace(_Iter1 __first, _Iter2 __last) {
        auto __n = distance(__first, __last);
        _reserve_(_size + __n);
        __builtin_memcpy(_impl + _size, __first, __n);
        _size += __n;
    }
};

#if __cplusplus >= 201703L
template <class _Char,
          size_t _capacity,
          class _Enable = enable_if_t<is_trivially_copyable_v<_Char>>>
class array final : public object {
public:
    using base = object;
    using self = array;
    using char_type = _Char;
    using size_type = uint32_t;
    using pointer = char_type*;
    using const_pointer = char_type const*;
    using iterator = char_type*;
    using const_iterator = char_type const*;

protected:
    char_type _impl[_capacity]{};
    size_type _size{0u};

public:
    ALWAYS_INLINE CONSTEXPR void emplace(char_type __val) {
        throw_if(_size + 1 >= _capacity, "overflow");
        _impl[_size] = __val;
        ++_size;
    }

    template <class _Iter1, class _Iter2>
    ALWAYS_INLINE CONSTEXPR void emplace(_Iter1 __first, _Iter2 __last) {
        auto __n = distance(__first, __last);
        throw_if(size_type(__n) + _size >= _capacity, "overflow");
        if constexpr (is_constant_evaluated()) {
            for (size_type __i = 0u; __i < __n; ++__i) {
                _impl[_size + __i] = __first[__i];
            }
        } else {
            __builtin_memcpy(_impl + _size, __first, __n);
        }
        _size += __n;
    }

    ALWAYS_INLINE CONSTEXPR size_type size() const noexcept { return _size; }
    ALWAYS_INLINE CONSTEXPR size_type capacity() const noexcept { return _capacity; }
    ALWAYS_INLINE CONSTEXPR pointer data() noexcept { return _impl; }
    ALWAYS_INLINE CONSTEXPR const_pointer data() const noexcept { return _impl; }
    ALWAYS_INLINE CONSTEXPR iterator begin() noexcept { return _impl; }
    ALWAYS_INLINE CONSTEXPR iterator end() noexcept { return _impl + _size; }
    ALWAYS_INLINE CONSTEXPR const_iterator begin() const noexcept { return _impl; }
    ALWAYS_INLINE CONSTEXPR const_iterator end() const noexcept { return _impl + _size; }

    ALWAYS_INLINE CONSTEXPR operator string::view<char_type>() const noexcept {
        return string::view<char_type>(begin(), end());
    }

    ALWAYS_INLINE CONSTEXPR operator string::value<char_type>() const noexcept {
        return string::value<char_type>(begin(), end());
    }
};
#endif

template <class _Char>
class _str final : public string::value<_Char> {
public:
    using base = string::value<_Char>;

    ALWAYS_INLINE CONSTEXPR explicit _str(buffer<_Char>&& __b) {
        if (__b._capacity > __b._size) {
            base::_impl = __b._impl;
            base::_size = __b._size;
            base::_capacity = __b._capacity;
            base::_impl[base::_size] = '\0';
            __b._impl = nullptr;
        } else {
            base::_init(__b._impl, __b._impl + __b._size);
        }
    }
};

template <class _Tp, class _Char, class _Enable = void>
struct formatter;

template <class _Char>
struct formatter<bool_t, _Char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            bool_t value,
                                            string::view<_Char> fmt ATTR_UNUSED) {
        string::view<_Char> str;
        if (value) {
            str = string::view<_Char>::true_str;
        } else {
            str = string::view<_Char>::false_str;
        }
        out.emplace(str.begin(), str.end());
    }
};

template <class _Tp, class _Char>
struct formatter<_Tp, _Char, enable_if_t<is_unsigned_v<_Tp>>> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out, _Tp value, string::view<_Char> fmt) {
        char str[32];
        auto l = len(value);
        string::__to_chars(str + l, value);
        out.emplace(str, str + l);
    }
};

template <class _Tp, class _Char>
struct formatter<_Tp, _Char, enable_if_t<is_signed_v<_Tp>>> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out, _Tp value, string::view<_Char> fmt) {
        char str[32]{'-', '\0'};
        auto l = len(value);
        using _Up = typename unsigned_traits<_Tp>::type;
        _Up val = value < 0 ? _Up(~value) + 1u : _Up(value);
        string::__to_chars(str + l, val);
        out.emplace(str, str + l);
    }
};

template <class _Tp, class _Char>
struct formatter<_Tp, _Char, enable_if_t<is_floating_point_v<_Tp>>> final : public object {
    struct specs {
        size_t precision{6u};
        ALWAYS_INLINE CONSTEXPR specs(string::view<_Char> fmt) {
            if (fmt.size() > 3) {
                throw_if(fmt[0] != '.', "missing '.'");
                throw_if(fmt[1] < '0' || fmt[1] > '9', "invalid number");
                throw_if(fmt[2] != 'f', "missing 'f'");
                precision = fmt[1] - '0';
            }
        }
    };

    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out, _Tp value, specs const& specs) {
        if (__builtin_isnan(value)) {
            auto str = string::view<_Char>::nan_str;
            out.emplace(str.begin(), str.end());
        } else if (__builtin_isinf(value)) {
            auto str = string::view<_Char>::inf_str;
            out.emplace(str.begin(), str.end());
        } else {
            formatter<int64_t, _Char> f;
            f(out, int64_t(value), string::view<_Char>{});
            char str[16u]{'\0'};
            string::__to_chars_for_frac(str, value - int64_t(value), specs.precision);
            out.emplace(str, str + specs.precision + 1u);
        }
    }
};

template <>
struct formatter<char, char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            char value,
                                            string::view<char> fmt ATTR_UNUSED) {
        out.emplace(value);
    }
};

template <class _Char>
struct formatter<string::view<_Char>, _Char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            string::view<_Char> value,
                                            string::view<_Char> fmt ATTR_UNUSED) {
        out.emplace(value.begin(), value.end());
    }
};

template <class _Char, class _Alloc>
struct formatter<string::value<_Char, _Alloc>, _Char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            string::value<_Char> value,
                                            string::view<_Char> fmt ATTR_UNUSED) {
        out.emplace(value.begin(), value.end());
    }
};

template <class _Char>
struct formatter<_Char const*, _Char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            _Char const* const value,
                                            string::view<_Char> fmt ATTR_UNUSED) {
        out.emplace(value, value + len(value));
    }
};

template <class _Char, size_t _N>
struct formatter<_Char[_N], _Char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            _Char const (&value)[_N],
                                            string::view<_Char> fmt ATTR_UNUSED) {
        out.emplace(value, value + _N - 1);
    }
};

#if defined(_BASIC_STRING_H)
template <class _Char>
struct formatter<std::basic_string<_Char>, _Char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            std::basic_string<_Char> const& value,
                                            string::view<_Char> fmt ATTR_UNUSED) {
        out.emplace(value.data(), value.data() + value.size());
    }
};
#endif

#if defined(_GLIBCXX_CHRONO)
template <class _Char>
struct formatter<std::chrono::system_clock::time_point, _Char> final : public object {
    template <class _Stream>
    ALWAYS_INLINE CONSTEXPR void operator()(_Stream& out,
                                            std::chrono::system_clock::time_point tp,
                                            string::view<_Char> specs) {
        constexpr char digits[201] = "0001020304050607080910111213141516171819"
                                     "2021222324252627282930313233343536373839"
                                     "4041424344454647484950515253545556575859"
                                     "6061626364656667686970717273747576777879"
                                     "8081828384858687888990919293949596979899";
        auto time_t_val = std::chrono::system_clock::to_time_t(tp);
        auto tm_val = localtime(time_t_val);

        for (auto it = specs.begin(); it != specs.end(); ++it) {
            if (*it == '%' && it + 1 != specs.end()) {
                ++it;
                switch (*it) {
                    case 'Y': {
                        auto value = tm_val.tm_year + 1900u;
                        auto n = (value % 100u) * 2u;
                        char str[5];
                        *(str + 3) = digits[n + 1];
                        *(str + 2) = digits[n];
                        auto n2 = ((value / 100u) % 100u) * 2u;
                        *(str + 1) = digits[n2 + 1];
                        *(str + 0) = digits[n2];
                        out.emplace(str, str + 4);
                        break;
                    }
                    case 'm': {
                        auto value = tm_val.tm_mon + 1;
                        auto n = (value % 100u) * 2u;
                        char str[3];
                        *(str + 1) = digits[n + 1];
                        *(str + 0) = digits[n];
                        out.emplace(str, str + 2);
                        break;
                    }
                    case 'd': {
                        auto value = tm_val.tm_mday;
                        auto n = (value % 100u) * 2u;
                        char str[3];
                        *(str + 1) = digits[n + 1];
                        *(str + 0) = digits[n];
                        out.emplace(str, str + 2);
                        break;
                    }
                    case 'H': {
                        auto value = tm_val.tm_hour;
                        auto n = (value % 100u) * 2u;
                        char str[3];
                        *(str + 1) = digits[n + 1];
                        *(str + 0) = digits[n];
                        out.emplace(str, str + 2);
                        break;
                    }
                    case 'M': {
                        auto value = tm_val.tm_min;
                        auto n = (value % 100u) * 2u;
                        char str[3];
                        *(str + 1) = digits[n + 1];
                        *(str + 0) = digits[n];
                        out.emplace(str, str + 2);
                        break;
                    }
                    case 'S': {
                        auto value = tm_val.tm_sec;
                        auto n = (value % 100u) * 2u;
                        char str[3];
                        *(str + 1) = digits[n + 1];
                        *(str + 0) = digits[n];
                        out.emplace(str, str + 2);
                        break;
                    }
                    case '3': {
                        ++it;
                        if (*it == 'f') {
                            auto value = std::chrono::duration_cast<std::chrono::milliseconds>(
                                             tp.time_since_epoch())
                                             .count() %
                                1000;
                            auto n = (value % 100u) * 2u;
                            char str[4];
                            *(str + 2) = digits[n + 1];
                            *(str + 1) = digits[n];
                            auto n2 = value / 100u;
                            *(str + 0) = n2 + '0';
                            out.emplace(str, str + 3);
                        } else {
                            out.emplace(it - 2, it + 1);
                        }
                        break;
                    }
                    case '6': {
                        ++it;
                        if (*it == 'f') {
                            auto value = std::chrono::duration_cast<std::chrono::microseconds>(
                                             tp.time_since_epoch())
                                             .count() %
                                1000;
                            char str[7];
                            auto n1 = (value % 100u) * 2u;
                            *(str + 5) = digits[n1 + 1];
                            *(str + 4) = digits[n1];
                            auto n2 = ((value / 100u) % 100u) * 2u;
                            *(str + 3) = digits[n2 + 1];
                            *(str + 2) = digits[n2];
                            auto n3 = ((value / 10000u) % 100u) * 2u;
                            *(str + 1) = digits[n3 + 1];
                            *(str + 0) = digits[n3];
                            out.emplace(str, str + 6);
                        } else {
                            out.emplace(it - 2, it + 1);
                        }
                    }
                    default: {
                        out.emplace(it - 1, it + 1);
                    }
                }
            } else {
                out.emplace(*it);
            }
        }
    }
};
#endif

template <size_t Idx, class _Stream, class _ArgsT, class _Char>
ALWAYS_INLINE CONSTEXPR void _format_s_(_Stream& out,
                                        _ArgsT const& args,
                                        string::view<_Char> spec) {
    using ArgType = std::tuple_element_t<Idx, _ArgsT>;
    using _ArgType = remove_cvref_t<ArgType>;
    formatter<_ArgType, _Char>{}(out, std::get<Idx>(args), spec);
}

template <class _Stream, class _Char, class _ArgsT, size_t... _Idx>
ALWAYS_INLINE CONSTEXPR void _format_iter_(_Stream& out,
                                           string::view<_Char> fmt,
                                           _ArgsT const& args,
                                           std::index_sequence<_Idx...>) {
    using handler_type = void (*)(_Stream&, _ArgsT const&, string::view<_Char>);
    constexpr handler_type arg_handlers[] = {&_format_s_<_Idx, _Stream, _ArgsT, _Char>...};

    size_t idx{0u};
    for (auto it = fmt.begin(); it != fmt.end(); ++it) {
        if (*it == '{') {
            ++it;

            auto i = idx;
            auto begin = it;
            while (it != fmt.end() && *it != '}') {
                if (*it == ':') {
                    if (distance(begin, it) > 0) {
                        auto r = std::from_chars(begin, it, i);
                        throw_if(r.ec != std::errc{}, "invalid index");
                    }
                    ++it;
                    begin = it;
                    break;
                }
                ++it;
            }
            while (it != fmt.end() && *it != '}') {
                ++it;
            }
            throw_if(it == fmt.end(), "missing right brace");
            throw_if(i >= sizeof...(_Idx), "index out of range");

            arg_handlers[i](out, args, string::view<_Char>{begin, it});
            ++idx;
        } else {
            out.emplace(*it);
        }
    }
}

template <class _Stream, class _Char, class... _Args>
ALWAYS_INLINE CONSTEXPR void _format_(_Stream& out, string::view<_Char> fmt, _Args&&... args) {
    auto args_tuple = std::forward_as_tuple(std::forward<_Args>(args)...);
    auto arg_indices = std::make_index_sequence<sizeof...(_Args)>();
    _format_iter_(out, fmt, args_tuple, arg_indices);
}

template <class... _Args>
ALWAYS_INLINE CONSTEXPR string::value<char> format(string::view<char> fmt, _Args&&... args) {
    buffer<char> b;
    _format_(b, fmt, forward<_Args>(args)...);
    return _str<char>(move(b));
}

template <class... _Args>
ALWAYS_INLINE CONSTEXPR auto format(string::view<char> fmt) {
    return fmt;
}

#if __cplusplus >= 201703L
template <size_t _N, class... _Args>
ALWAYS_INLINE CONSTEXPR auto bformat(string::view<char> fmt, _Args&&... args) {
    array<char, _N> b;
    _format_(b, fmt, forward<_Args>(args)...);
    return b;
}
#endif

};  // namespace fmt

};  // namespace qlib

#endif

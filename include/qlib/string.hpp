#ifndef QLIB_STRING_HPP
#define QLIB_STRING_HPP

#include "qlib/memory.h"
#include "qlib/object.h"

namespace qlib {

namespace string {

template <class _Char>
NODISCARD ALWAYS_INLINE constexpr bool_t in(_Char c, _Char const* str) noexcept {
    bool_t ok{False};
    while (likely(*str != '\0')) {
        if (unlikely(c == *str)) {
            ok = True;
            break;
        }
        ++str;
    }
    return ok;
}

template <class T, class Iter>
ALWAYS_INLINE CONSTEXPR enable_if_t<is_unsigned_v<T>, void> __to_chars(Iter __last,
                                                                       T __value) noexcept {
    constexpr char __digits[201] = "0001020304050607080910111213141516171819"
                                   "2021222324252627282930313233343536373839"
                                   "4041424344454647484950515253545556575859"
                                   "6061626364656667686970717273747576777879"
                                   "8081828384858687888990919293949596979899";
    while (__value >= 100) {
        auto const __num = (__value % 100) * 2;
        __value /= 100;
        *--__last = __digits[__num + 1];
        *--__last = __digits[__num];
    }
    if (__value >= 10) {
        auto const __num = __value * 2;
        *--__last = __digits[__num + 1];
        *--__last = __digits[__num];
    } else {
        *--__last = '0' + __value;
    }
}

template <class _Tp, class _Iter>
ALWAYS_INLINE CONSTEXPR enable_if_t<is_floating_point_v<_Tp>, void> __to_chars_for_frac(
    _Iter __first, _Tp __frac, size_t __precision) noexcept {
    *__first++ = '.';
    for (auto i = 0u; i < __precision - 1; ++i) {
        __frac *= 10u;
        uint8_t __digit = uint8_t(__frac);
        *__first++ = static_cast<char>(__digit + '0');
        __frac -= __digit;
    }
    __frac *= 10u;
    auto __digit = uint8_t(__frac);
    __frac -= __digit;
    if (__frac > 0.5) {
        __digit += 1u;
    }
    *__first++ = static_cast<char>(__digit + '0');
}

template <class T, class Iter1, class Iter2>
ALWAYS_INLINE CONSTEXPR enable_if_t<is_unsigned_v<T>, Iter1> to_chars(Iter1 __first,
                                                                      Iter2 __last,
                                                                      T __value) noexcept {
    auto __len = len(__value);
    throw_if(__len > decltype(__len)(distance(__first, __last)));

    __last = __first + __len;
    __to_chars<T>(__last, __value);

    return __last;
}

template <class T, class Iter1, class Iter2>
ALWAYS_INLINE CONSTEXPR enable_if_t<is_signed_v<T>, Iter1> to_chars(Iter1 __first,
                                                                    Iter2 __last,
                                                                    T __value) noexcept {
    using U = typename unsigned_traits<T>::type;
    U __val = __value;
    size_t __len{0u};
    if (__value < 0) {
        __val = U(~__value) + U(1);
        __len = 1u;
    }
    __len += len(__val);
    throw_if(__len > decltype(__len)(distance(__first, __last)));

    *__first = '-';
    __last = __first + __len;
    __to_chars<U>(__last, __val);

    return __last;
}

template <class _Tp, class Iter1, class Iter2>
ALWAYS_INLINE CONSTEXPR enable_if_t<is_floating_point_v<_Tp>, Iter1> to_chars(
    Iter1 __first, Iter2 __last, _Tp __value, size_t __precision = 6u) noexcept {
    throw_if(__builtin_isnan(__value) || __builtin_isinf(__value));

    size_t __len{0u};
    _Tp __val = __value;
    if (__value < 0) {
        __val = -__value;
        __len = 1u;
    }

    auto __val_int = static_cast<uint64_t>(__val);
    __len += len(__val_int);
    throw_if(__len + 1u + __precision > size_t(distance(__first, __last)));

    *__first = '-';
    __first = __first + __len;
    __to_chars<uint64_t>(__first, __val_int);
    __to_chars_for_frac<_Tp>(__first, __val - __val_int, __precision);
    return __first;
}

template <class _Char>
struct char_traits final : public object {
    using value_type = _Char;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using reference = value_type&;
    using const_reference = value_type const&;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using size_type = uint32_t;
    static constexpr size_type npos = size_type(-1);
};

template <class _Char, class _Enable = enable_if_t<is_trivially_copyable_v<_Char>>>
class view final : public object {
public:
    using self = view;
    using char_type = _Char;
    using traits_type = char_traits<_Char>;
    using value_type = typename traits_type::value_type;
    using const_pointer = typename traits_type::const_pointer;
    using const_reference = typename traits_type::const_reference;
    using const_iterator = typename traits_type::const_iterator;
    using size_type = typename traits_type::size_type;
    constexpr static size_type npos = traits_type::npos;

    constexpr static self true_str{"true"};
    constexpr static self false_str{"false"};
    constexpr static self null_str{"null"};
    constexpr static self nan_str{"nan"};
    constexpr static self inf_str{"infinity"};

protected:
    const_pointer _impl{nullptr};
    size_type _size{0u};

public:
    ALWAYS_INLINE constexpr view() noexcept = default;

    template <class Iter1, class Iter2>
    ALWAYS_INLINE explicit constexpr view(Iter1 begin, Iter2 end) noexcept
            : _impl(begin), _size(distance(begin, end)) {}

    ALWAYS_INLINE constexpr view(const_pointer o) noexcept : _impl(o), _size(len(o)) {}

    ALWAYS_INLINE constexpr view(self const& o) noexcept : _impl(o._impl), _size(o._size) {}

    ALWAYS_INLINE constexpr self& operator=(const_pointer o) noexcept {
        _impl = o;
        _size = len(o);
        return *this;
    }

    ALWAYS_INLINE constexpr self& operator=(self const& o) noexcept {
        _impl = o._impl;
        _size = o._size;
        return *this;
    }

    NODISCARD ALWAYS_INLINE constexpr const_pointer data() const noexcept { return _impl; }

    NODISCARD ALWAYS_INLINE constexpr size_type size() const noexcept { return _size; }

    NODISCARD ALWAYS_INLINE constexpr self substr(size_type pos,
                                                  size_type n = npos) const noexcept {
        // if (unlikely(pos > size())) {
        //     return self();
        // }
        size_type len = (n == npos || pos + n > size()) ? size() - pos : n;
        return self(data() + pos, data() + pos + len);
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t starts_with(const_pointer o) const noexcept {
        return starts_with(self(o));
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t starts_with(self const& o) const noexcept {
        return substr(0u, o.size()) == o;
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t ends_with(const_pointer o) const noexcept {
        return ends_with(self(o));
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t ends_with(self const& o) const noexcept {
        return size() >= o.size() && substr(size() - o.size(), o.size()) == o;
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t operator==(const_pointer o) const noexcept {
        return *this == self(o);
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t operator==(self const& o) const noexcept {
        return size() == o.size() && equal(begin(), end(), o.begin());
    }

    template <class T>
    NODISCARD ALWAYS_INLINE constexpr bool_t operator!=(T o) const noexcept {
        return !(*this == o);
    }

    NODISCARD ALWAYS_INLINE constexpr auto operator[](size_type pos) const noexcept {
        return *(data() + pos);
    }

    NODISCARD ALWAYS_INLINE constexpr auto empty() const noexcept { return size() == 0u; }

    NODISCARD ALWAYS_INLINE constexpr auto begin() const noexcept { return data(); }

    NODISCARD ALWAYS_INLINE constexpr auto end() const noexcept { return data() + size(); }

    NODISCARD ALWAYS_INLINE constexpr auto front() const noexcept { return *(data()); }

    NODISCARD ALWAYS_INLINE constexpr auto back() const noexcept { return *(data() + size() - 1); }

    NODISCARD ALWAYS_INLINE explicit operator bool_t() const noexcept { return !empty(); }

#ifdef _GLIBCXX_STRING_VIEW
    NODISCARD ALWAYS_INLINE explicit operator std::basic_string_view<value_type>() const noexcept {
        return std::basic_string_view<value_type>(data(), size());
    }
#endif

#ifdef _BASIC_STRING_H
    NODISCARD ALWAYS_INLINE explicit operator std::basic_string<_Char>() const noexcept {
        return std::basic_string<_Char>(begin(), end());
    }
#endif
};

template <class _Char, class _Enable>
constexpr view<_Char, _Enable> view<_Char, _Enable>::true_str;

template <class _Char, class _Enable>
constexpr view<_Char, _Enable> view<_Char, _Enable>::false_str;

template <class _Char, class _Enable>
constexpr view<_Char, _Enable> view<_Char, _Enable>::null_str;

template <class _Char, class _Enable>
constexpr view<_Char, _Enable> view<_Char, _Enable>::nan_str;

template <class _Char, class _Enable>
constexpr view<_Char, _Enable> view<_Char, _Enable>::inf_str;

template <class _Char,
          class Allocator = new_allocator_t,
          class _Enable = enable_if_t<is_trivially_copyable_v<_Char>>>
class value : public traits<Allocator>::reference {
public:
    using base = typename traits<Allocator>::reference;
    using self = value;
    using char_type = _Char;
    using allocator_type = Allocator;
    using view_type = view<_Char>;
    using traits_type = char_traits<_Char>;
    using value_type = typename traits_type::value_type;
    using pointer = typename traits_type::pointer;
    using const_pointer = typename traits_type::const_pointer;
    using reference = typename traits_type::reference;
    using const_reference = typename traits_type::const_reference;
    using iterator = typename traits_type::iterator;
    using const_iterator = typename traits_type::const_iterator;
    using size_type = typename traits_type::size_type;
    static constexpr size_type npos = traits_type::npos;

protected:
    _Char* _impl{nullptr};
    size_type _size{0u};
    size_type _capacity{0u};

    ALWAYS_INLINE CONSTEXPR allocator_type& _allocator_() const noexcept {
        return static_cast<base&>(const_cast<self&>(*this));
    }

    template <class Iter1, class Iter2>
    ALWAYS_INLINE constexpr void _init(Iter1 begin, Iter2 end) {
        const size_type size = distance(begin, end);
        _impl = _allocator_().template allocate<char_type>(size + 1u);
        _size = size;
        _capacity = size;
        copy(begin, end, _impl);
        _impl[size] = '\0';
    }

    template <class Iter1, class Iter2>
    ALWAYS_INLINE constexpr void _assign(Iter1 begin, Iter2 end) {
        const size_type size = distance(begin, end);
        // if (likely(size > 0)) {
        if (likely(size > capacity())) {
            _allocator_().template deallocate<char_type>(_impl, capacity());
            _impl = _allocator_().template allocate<char_type>(size + 1u);
            _capacity = size;
        }
        copy(begin, end, _impl);
        _impl[size] = '\0';
        _size = size;
        // }
    }

public:
    ALWAYS_INLINE constexpr value() noexcept(is_nothrow_constructible_v<base>) {}

    ALWAYS_INLINE constexpr explicit value(allocator_type& allocator) noexcept(
        is_nothrow_constructible_v<base>)
            : base(allocator) {}

    ALWAYS_INLINE constexpr explicit value(size_type capacity)
            : _impl{_allocator_().template allocate<char_type>(capacity + 1u)},
              _capacity(capacity) {}

    ALWAYS_INLINE constexpr explicit value(size_type capacity, allocator_type& allocator)
            : base(allocator), _impl{_allocator_().template allocate<char_type>(capacity + 1u)},
              _capacity(capacity) {}

    template <class Iter1,
              class Iter2,
              class Enable =
                  enable_if_t<is_same_v<typename iterator_traits<Iter1>::value_type, char_type> &&
                              is_same_v<typename iterator_traits<Iter2>::value_type, char_type>>>
    ALWAYS_INLINE constexpr explicit value(Iter1 begin, Iter2 end) {
        _init(begin, end);
    }

    template <class Iter1,
              class Iter2,
              class Enable =
                  enable_if_t<is_same_v<typename iterator_traits<Iter1>::value_type, char_type> &&
                              is_same_v<typename iterator_traits<Iter2>::value_type, char_type>>>
    ALWAYS_INLINE constexpr explicit value(Iter1 begin, Iter2 end, allocator_type& allocator)
            : base(allocator) {
        _init(begin, end);
    }

    ALWAYS_INLINE constexpr value(const_pointer str) : value(str, str + len(str)) {}

    ALWAYS_INLINE constexpr value(const_pointer str, allocator_type& allocator)
            : value(str, str + len(str), allocator) {}

    ALWAYS_INLINE constexpr value(view_type o) : value(o.begin(), o.end()) {}

    ALWAYS_INLINE constexpr value(view_type o, allocator_type& allocator)
            : value(o.begin(), o.end(), allocator) {}

    ALWAYS_INLINE constexpr value(self const& o) : base(o) { _init(o.begin(), o.end()); }

    ALWAYS_INLINE constexpr value(self&& o) noexcept
            : base(move(o)), _impl(o._impl), _size(o._size), _capacity(o._capacity) {
        o._impl = nullptr;
    }

#ifdef _GLIBCXX_STRING_VIEW
    ALWAYS_INLINE constexpr explicit value(std::basic_string_view<char_type> str)
            : value(str.data(), str.data() + str.size()) {}
#endif

    ALWAYS_INLINE ~value() noexcept(is_nothrow_destructible_v<base>) {
        if (_impl != nullptr) {
            _allocator_().template deallocate<char_type>(_impl, _capacity);
            _impl = nullptr;
            _size = 0u;
            _capacity = 0u;
        }
    }

    ALWAYS_INLINE constexpr self& operator=(const_pointer o) {
        _assign(o, o + len(o));
        return *this;
    }

    ALWAYS_INLINE constexpr self& operator=(view_type o) {
        _assign(o.begin(), o.end());
        return *this;
    }

    ALWAYS_INLINE constexpr self& operator=(self const& o) {
        if (unlikely(this != &o)) {
            _assign(o.begin(), o.end());
        }
        return *this;
    }

    ALWAYS_INLINE constexpr self& operator=(self&& o) noexcept {
        _allocator_().template deallocate<char_type>(_impl, _capacity);
        _impl = o._impl;
        _size = o._size;
        _capacity = o._capacity;
        o._impl = nullptr;
        o._size = 0u;
        o._capacity = 0u;
        return *this;
    }

#ifdef _BASIC_STRING_H
    ALWAYS_INLINE CONSTEXPR self& operator=(std::basic_string<char_type> const& __o) noexcept {
        *this = self(__o.data(), __o.data() + __o.size(), _allocator_());
        return *this;
    }
#endif

    ALWAYS_INLINE void reserve(size_type capacity) {
        if (capacity > _capacity) {
            size_type new_capacity = 1u;
            while (new_capacity < capacity + 1u) {
                new_capacity <<= 1u;
            }
            auto impl = _allocator_().template allocate<char_type>(new_capacity);
            copy(begin(), end(), impl);
            _allocator_().template deallocate<char_type>(_impl, _capacity);
            _impl = impl;
            _impl[_size] = '\0';
            _capacity = new_capacity - 1u;
        }
    }

    NODISCARD ALWAYS_INLINE constexpr pointer data() noexcept { return _impl; }

    NODISCARD ALWAYS_INLINE constexpr const_pointer data() const noexcept { return _impl; }

    NODISCARD ALWAYS_INLINE constexpr size_type size() const noexcept { return _size; }

    NODISCARD ALWAYS_INLINE constexpr size_type capacity() const noexcept { return _capacity; }

    NODISCARD ALWAYS_INLINE constexpr bool_t empty() const noexcept { return size() == 0u; }

    NODISCARD ALWAYS_INLINE constexpr reference operator[](size_type pos) { return data()[pos]; }

    NODISCARD ALWAYS_INLINE constexpr const_reference operator[](size_type pos) const {
        return const_cast<self&>(*this)[pos];
    }

    NODISCARD ALWAYS_INLINE constexpr iterator begin() noexcept { return data(); }

    NODISCARD ALWAYS_INLINE constexpr const_iterator begin() const noexcept { return data(); }

    NODISCARD ALWAYS_INLINE constexpr iterator end() noexcept { return data() + size(); }

    NODISCARD ALWAYS_INLINE constexpr const_iterator end() const noexcept {
        return data() + size();
    }

    NODISCARD ALWAYS_INLINE constexpr reference front() noexcept { return *data(); }
    NODISCARD ALWAYS_INLINE constexpr const_reference front() const noexcept {
        return const_cast<self&>(*this).front();
    }
    NODISCARD ALWAYS_INLINE constexpr reference back() noexcept { return *(data() + size() - 1); }
    NODISCARD ALWAYS_INLINE constexpr const_reference back() const noexcept {
        return const_cast<self&>(*this).back();
    }

    NODISCARD ALWAYS_INLINE constexpr const_pointer c_str() const noexcept { return data(); }

    NODISCARD ALWAYS_INLINE constexpr bool_t starts_with(self const& o) const noexcept {
        return static_cast<view_type>(*this).starts_with(o);
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t ends_with(self const& o) const noexcept {
        return static_cast<view_type>(*this).ends_with(o);
    }

    NODISCARD ALWAYS_INLINE constexpr bool_t operator==(view_type o) const noexcept {
        return size() == o.size() && equal(begin(), end(), o.begin());
    }

    template <class T>
    NODISCARD ALWAYS_INLINE constexpr bool_t operator!=(T o) const noexcept {
        return !(*this == o);
    }

    ALWAYS_INLINE CONSTEXPR void clear() noexcept { _size = 0u; }

    template <class Iter1, class Iter2>
    ALWAYS_INLINE void emplace(Iter1 __begin, Iter2 __end) noexcept {
        const size_type __size = distance(__begin, __end);
        // if (likely(__size > 0u)) {
        const size_type __new_size = size() + __size;
        reserve(__new_size);
        copy(__begin, __end, end());
        _impl[__new_size] = '\0';
        _size = __new_size;
        // }
    }

    ALWAYS_INLINE self& operator<<(char_type ch) {
        emplace(&ch, &ch + 1);
        return *this;
    }

    ALWAYS_INLINE self& operator<<(const_pointer o) {
        emplace(o, o + len(o));
        return *this;
    }

    ALWAYS_INLINE self& operator<<(view_type o) {
        emplace(o.begin(), o.end());
        return *this;
    }

    ALWAYS_INLINE self& operator<<(self const& o) {
        emplace(o.begin(), o.end());
        return *this;
    }

    ALWAYS_INLINE self& operator<<(bool_t ok) {
        view_type __view;
        if (ok) {
            __view = view_type::true_str;
        } else {
            __view = view_type::false_str;
        }
        emplace(__view.begin(), __view.end());
        return *this;
    }

    template <class _Tp>
    ALWAYS_INLINE enable_if_t<is_unsigned_v<_Tp>, self&> operator<<(_Tp __value) {
        auto __len = len(__value);
        const size_type __new_size = size() + __len;
        reserve(__new_size);
        __to_chars(_impl + __new_size, __value);
        _impl[__new_size] = '\0';
        _size = __new_size;
        return *this;
    }

    template <class _Tp>
    ALWAYS_INLINE enable_if_t<is_signed_v<_Tp>, self&> operator<<(_Tp __value) {
        using _Up = typename unsigned_traits<_Tp>::type;
        _Up __val{_Up(__value)};
        size_t __len{0u};
        if (__value < 0) {
            __val = _Up(~__value) + _Up(1);
            __len = 1u;
        }
        __len += len(__val);
        size_type __new_size = size() + __len;
        reserve(__new_size);
        _impl[size()] = '-';
        __to_chars(_impl + __new_size, __val);
        _impl[__new_size] = '\0';
        _size = __new_size;
        return *this;
    }

    template <class _Tp>
    ALWAYS_INLINE enable_if_t<is_floating_point_v<_Tp>, self&> operator<<(_Tp __value) {
        throw_if(__builtin_isnan(__value) || __builtin_isinf(__value));

        size_t __len{0u};
        _Tp __val{__value};
        if (__value < 0) {
            __val = -__value;
            __len = 1u;
        }

        auto __val_int = uint64_t(__val);
        __len += len(__val_int);
        size_type __precision = 6u;
        size_type __new_size = size() + __len + 1u + __precision;
        reserve(__new_size);
        _impl[size()] = '-';
        __to_chars<uint64_t>(_impl + size() + __len, __val_int);
        __to_chars_for_frac<_Tp>(_impl + size() + __len, __val - __val_int, __precision);
        _impl[__new_size] = '\0';
        _size = __new_size;
        return *this;
    }

    NODISCARD ALWAYS_INLINE operator view_type() const noexcept {
        return view_type(begin(), end());
    }

#ifdef _BASIC_STRING_H
    NODISCARD ALWAYS_INLINE operator std::basic_string<char_type>() const noexcept {
        return std::basic_string<char_type>(data());
    }
#endif

    template <class _Tp>
    NODISCARD ALWAYS_INLINE
        CONSTEXPR static enable_if_t<is_unsigned_v<_Tp>, self> from(_Tp __value) {
        auto __len = len(__value);
        self __str(__len);
        __to_chars(__str._impl + __len, __value);
        *(__str._impl + __len) = '\0';
        __str._size = __len;
        return __str;
    }

    template <class _Tp>
    NODISCARD ALWAYS_INLINE CONSTEXPR static enable_if_t<is_signed_v<_Tp>, self> from(_Tp __value) {
        auto __l = len(__value);
        using _Up = typename unsigned_traits<_Tp>::type;
        _Up __val = __value < 0 ? _Up(~__value) + 1u : _Up(__value);
        self __s{size_type(__l)};
        __s._impl[0] = '-';
        string::__to_chars(__s._impl + __l, __val);
        __s._impl[__l] = '\0';
        __s._size = __l;
        return __s;
    }

    template <class _Tp>
    NODISCARD ALWAYS_INLINE CONSTEXPR static enable_if_t<is_floating_point_v<_Tp>, self> from(
        _Tp __value, size_t __precision = 6u) {
        throw_if(__builtin_isnan(__value) || __builtin_isinf(__value));

        size_t __len{0u};
        _Tp __val{__value};
        if (__value < 0) {
            __val = -__value;
            __len = 1u;
        }

        auto __val_int = uint64_t(__val);
        __len += len(__val_int);
        size_type __new_size = __len + 1u + __precision;
        self __str(__new_size);
        *__str._impl = '-';
        __to_chars<uint64_t>(__str._impl + __len, __val_int);
        __to_chars_for_frac<_Tp>(__str._impl + __len, __val - __val_int, __precision);
        *(__str._impl + __new_size) = '\0';
        __str._size = __new_size;
        return __str;
    }

#ifdef _GLIBCXX_FSTREAM
    template <class _S>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto from_file(_S&& __path) {
        self __text;

        std::basic_ifstream<char_type> __stream(__path);
        if (__stream.is_open()) {
            __stream.seekg(0, std::ios::end);
            auto __len = static_cast<size_type>(__stream.tellg());
            __text.reserve(__len);
            __stream.seekg(0, std::ios::beg);
            __stream.read(__text.data(), __len);
            __text._size = __len;
        }

        return __text;
    }
#endif
};

#ifdef _GLIBCXX_OSTREAM
template <class _Char>
ALWAYS_INLINE std::basic_ostream<_Char>& operator<<(std::basic_ostream<_Char>& out, view<_Char> s) {
    out.write(s.data(), s.size());
    return out;
}

template <class _Char, class Allocator>
ALWAYS_INLINE std::basic_ostream<_Char>& operator<<(std::basic_ostream<_Char>& out,
                                                    value<_Char, Allocator> const& s) {
    out.write(s.data(), s.size());
    return out;
}
#endif

#ifdef _GLIBCXX_FSTREAM
template <class _Char>
NODISCARD ALWAYS_INLINE CONSTEXPR static auto from_file(view<_Char> const& __path) {
    return value<_Char>::from_file(value<_Char>{__path});
}

template <class _Char, class Allocator>
NODISCARD ALWAYS_INLINE CONSTEXPR static auto from_file(value<_Char, Allocator> const& __path) {
    return value<_Char, Allocator>::from_file(__path);
}
#endif

};  // namespace string

using string_view_t = string::view<char>;
using string_t = string::value<char>;
};  // namespace qlib

#endif

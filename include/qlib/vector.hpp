#pragma once

#define VECTOR_IMPLEMENTATION

#include "qlib/memory.h"
#include "qlib/object.h"

namespace qlib {
namespace vector {

class out_of_range final : public exception {
public:
    char const* what() const noexcept override { return "out of range"; }
};

template <class T>
struct vector_traits final : public object {
    using value_type = T;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using reference = value_type&;
    using const_reference = value_type const&;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using size_type = uint32_t;
    static constexpr size_type npos = size_type(-1);
};

template <class T, class Allocator = new_allocator_t>
class value final : public traits<Allocator>::reference {
public:
    using base = typename traits<Allocator>::reference;
    using self = value;
    using allocator_type = Allocator;
    using traits_type = vector_traits<T>;
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
    value_type* _impl{nullptr};
    size_type _size{0u};
    size_type _capacity{0u};

    NODISCARD FORCE_INLINE allocator_type& _allocator_() const noexcept {
        return static_cast<base&>(const_cast<self&>(*this));
    }

    template <class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<is_trivially_copyable_v<_uValue>, void> _move_(
        value_type* __dst, value_type* __src, size_type __size) noexcept {
        __builtin_memmove(__dst, __src, __size * sizeof(value_type));
    }

    template <class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<!is_trivially_copyable_v<_uValue>, void> _move_(
        value_type* __dst, value_type* __src, size_type __size) noexcept {
        for (size_type i = 0; i < __size; ++i) {
            _allocator_().construct(&__dst[i], move(__src[i]));
        }
    }

    template <class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<is_trivially_copyable_v<_uValue>, void> _copy_(
        value_type* __dst, value_type* __src, size_type __size) const noexcept {
        __builtin_memmove(__dst, __src, __size * sizeof(value_type));
    }

    template <class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<!is_trivially_copyable_v<_uValue>, void> _copy_(
        value_type* __dst, value_type* __src, size_type __size) const noexcept {
        for (size_type i = 0; i < __size; ++i) {
            _allocator_().construct(&__dst[i], __src[i]);
        }
    }

    template <class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<is_trivially_copyable_v<_uValue>, void> _destroy_(
        value_type* __src, size_type __size) noexcept {}

    template <class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<!is_trivially_copyable_v<_uValue>, void> _destroy_(
        value_type* __src, size_type __size) noexcept {
        for (size_type i = 0; i < __size; ++i) {
            _allocator_().destroy(&__src[i]);
        }
    }

    INLINE void _update_capacity(size_type capacity) {
        auto __impl = _allocator_().template allocate<value_type>(capacity);
        _move_(__impl, _impl, _size);
        _destroy_(_impl, _size);
        _allocator_().template deallocate<value_type>(_impl, _capacity);
        _impl = __impl;
        _capacity = capacity;
    }

public:
    INLINE constexpr value() noexcept(is_nothrow_constructible_v<base>) = default;

    INLINE constexpr explicit value(allocator_type& allocator) noexcept(
        is_nothrow_constructible_v<base>)
            : base(allocator) {}

    INLINE constexpr explicit value(size_type capacity) noexcept(is_nothrow_constructible_v<base>)
            : _capacity(capacity) {
        _impl = _allocator_().template allocate<value_type>(capacity);
    }

    INLINE constexpr explicit value(size_type capacity, allocator_type& allocator) noexcept(
        is_nothrow_constructible_v<base>)
            : base(allocator), _capacity(capacity) {
        _impl = _allocator_().template allocate<value_type>(capacity);
    }

    INLINE value(value const& other) : base(other), _size(other._size), _capacity(other._capacity) {
        if (likely(_size > 0)) {
            _impl = _allocator_().template allocate<value_type>(_capacity);
            _copy_(_impl, other._impl, other._size);
        }
    }

    INLINE constexpr value(value&& other) noexcept
            : base(move(other)), _impl(other._impl), _size(other._size),
              _capacity(other._capacity) {
        other._impl = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    INLINE ~value() noexcept(is_nothrow_destructible_v<allocator_type>) {
        _destroy_(_impl, _size);
        _allocator_().template deallocate<value_type>(_impl, _capacity);
        _impl = nullptr;
        _size = 0;
        _capacity = 0;
    }

    INLINE self& operator=(self const& other) noexcept(
        is_nothrow_constructible_v<self, self const&>) {
        if (likely(this != &other)) {
            this->~value();
            new (this) self(other);
        }
        return *this;
    }

    template <class... Args>
    INLINE self& operator=(Args&&... args) noexcept(is_nothrow_constructible_v<self, Args...>) {
        this->~value();
        new (this) self(forward<Args>(args)...);
        return *this;
    }

    INLINE constexpr void reserve(size_type capacity) noexcept {
        if (unlikely(capacity > _capacity)) {
            _update_capacity(capacity);
        }
    }

    FORCE_INLINE CONSTEXPR void resize(size_type __size) noexcept {
        reserve(__size);
        if CONSTEXPR (!is_trivially_copyable_v<value_type>) {
            if (_size < __size) {
                for (auto i = _size; i < __size; ++i) {
                    _allocator_().construct(&_impl[i]);
                }
            } else {
                for (auto i = __size; i < _size; ++i) {
                    _allocator_().destroy(&_impl[i]);
                }
            }
        }
        _size = __size;
    }

    INLINE constexpr pointer data() noexcept { return _impl; }
    INLINE constexpr const_pointer data() const noexcept { return _impl; }
    INLINE constexpr size_type size() const noexcept { return _size; }
    INLINE constexpr size_type capacity() const noexcept { return _capacity; }
    INLINE constexpr bool_t empty() const noexcept { return size() == 0; }
    INLINE constexpr explicit operator bool_t() const noexcept { return !empty(); }

    INLINE constexpr iterator begin() noexcept { return _impl; }
    INLINE constexpr iterator end() noexcept { return _impl + _size; }
    INLINE constexpr const_iterator begin() const noexcept { return _impl; }
    INLINE constexpr const_pointer end() const noexcept { return _impl + _size; }

    INLINE constexpr reference operator[](size_type index) { return _impl[index]; }

    INLINE constexpr const_reference operator[](size_type index) const {
        return const_cast<self&>(*this)[index];
    }

    INLINE constexpr reference front() { return _impl[0]; }
    INLINE constexpr reference back() { return _impl[_size - 1]; }
    INLINE constexpr const_reference front() const { return const_cast<self&>(*this).front(); }
    INLINE constexpr const_reference back() const { return const_cast<self&>(*this).back(); }

    template <class... Args>
    INLINE constexpr void emplace_back(Args&&... args) noexcept(
        is_nothrow_constructible_v<value_type, Args...>) {
        size_type capacity = _size + 1u;
        if (unlikely(capacity > _capacity)) {
            capacity = capacity * 2u;
            _update_capacity(capacity);
        }
        _allocator_().construct(_impl + _size, forward<Args>(args)...);
        ++_size;
    }

    template <class... Args>
    INLINE constexpr void push_back(Args&&... args) noexcept(
        is_nothrow_constructible_v<value_type, Args...>) {
        emplace_back(forward<Args>(args)...);
    }

    INLINE constexpr void pop_back() noexcept(is_nothrow_destructible_v<value_type>) {
        if (likely(_size > 0)) {
            _allocator_().destroy(_impl + _size - 1);
            --_size;
        }
    }

    FORCE_INLINE CONSTEXPR auto& allocator() const noexcept { return _allocator_(); }
};

};  // namespace vector

template <class T, class Allocator = new_allocator_t>
using vector_t = vector::value<T, Allocator>;

template <class _Tp, class _Allocator>
struct _is_container_helper_<vector::value<_Tp, _Allocator>> : public true_type {};

};  // namespace qlib

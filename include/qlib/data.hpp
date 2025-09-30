#ifndef QLIB_DATA_H
#define QLIB_DATA_H

#include "qlib/vector.h"

namespace qlib {

namespace data {
template <class _Key, class _Callback, class _Allocator = new_allocator_t>
class manager;
};

template <class _Key, class _Callback, class _Allocator>
struct traits<data::manager<_Key, _Callback, _Allocator>> {
    using reference = qlib::reference<data::manager<_Key, _Callback, _Allocator>>;
};

namespace data {

class redundant_key final : public exception {
public:
    char const* what() const noexcept override { return "redundant key"; }
};

class bad_key final : public exception {
public:
    char const* what() const noexcept override { return "bad key"; }
};

template <class _Key, class _Callback, class _Allocator>
class manager final : public object {
public:
    using base = object;
    using self = manager;
    using allocator_type = _Allocator;
    using key_type = _Key;
    using value_type = _Callback;

protected:
    vector_t<pair<key_type, value_type>, allocator_type> _map;

    using allocator_ref = typename traits<allocator_type>::reference;
    template <class _uKey, class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<is_base_of_v<allocator_ref, _uValue>, void> _emplace_(
        _uKey&& __key) {
        _map.emplace_back(forward<_uKey>(__key), value_type{_map.allocator()});
    }

    template <class _uKey, class _uValue = value_type>
    FORCE_INLINE CONSTEXPR enable_if_t<!is_base_of_v<allocator_ref, _uValue>, void> _emplace_(
        _uKey&& __key) {
        _map.emplace_back(forward<_uKey>(__key), value_type{});
    }

public:
    manager(self const&) = delete;
    self& operator=(self const&) = delete;

    manager() = default;
    manager(self&&) = default;
    self& operator=(self&&) = default;

    FORCE_INLINE CONSTEXPR manager(allocator_type& __allocator) : _map(__allocator) {}

    template <class _uKey, class _uValue = value_type>
    NODISCARD FORCE_INLINE CONSTEXPR value_type& operator[](_uKey&& __key) {
        auto __it = find(
            _map.begin(), _map.end(), __key,
            [](pair<key_type, value_type> const& __a, _uKey const& __b) { return __a.key == __b; });
        if (__it == _map.end()) {
            _emplace_(forward<_uKey>(__key));
            return _map.back().value;
        } else {
            return __it->value;
        }
    }
};

template <class _Manager>
class publisher final : public traits<_Manager>::reference {
public:
    using base = typename traits<_Manager>::reference;
    using self = publisher;
    using manager_type = _Manager;
    using key_type = typename manager_type::key_type;
    using value_type = typename manager_type::value_type;

protected:
    value_type& _impl;

    NODISCARD FORCE_INLINE CONSTEXPR manager_type& _manager_() const noexcept {
        return static_cast<base&>(const_cast<self&>(*this));
    }

    template <class _uValue = value_type, class... _Args>
    FORCE_INLINE CONSTEXPR enable_if_t<is_container_v<_uValue>, void> _emplace_(_Args&&... __args) {
        for (size_t i = 0; i < _impl.size(); ++i) {
            auto& __callback = _impl[i];
            if (__callback) {
                __callback(__args...);
            }
        }
    }

    template <class _uValue = value_type, class... _Args>
    FORCE_INLINE CONSTEXPR enable_if_t<!is_container_v<_uValue>, void> _emplace_(
        _Args&&... __args) {
        if (_impl) {
            _impl(forward<_Args>(__args)...);
        }
    }

public:
    template <class _Key, class _uManager>
    FORCE_INLINE CONSTEXPR publisher(_Key&& __key, _uManager&& __manager)
            : base(forward<_uManager>(__manager)), _impl{_manager_()[forward<_Key>(__key)]} {}

    template <class... _Args>
    FORCE_INLINE CONSTEXPR void publish(_Args&&... __args) {
        _emplace_(forward<_Args>(__args)...);
    }
};

template <class _Manager>
class subscriber final : public traits<_Manager>::reference {
public:
    using base = typename traits<_Manager>::reference;
    using self = subscriber;
    using manager_type = _Manager;
    using key_type = typename manager_type::key_type;
    using value_type = typename manager_type::value_type;

protected:
    value_type& _impl;

    NODISCARD FORCE_INLINE CONSTEXPR manager_type& _manager_() const noexcept {
        return static_cast<base&>(const_cast<self&>(*this));
    }

    template <class _uValue = value_type, class... _Args>
    FORCE_INLINE CONSTEXPR enable_if_t<is_container_v<_uValue>, void> _init_(_Args&&... __args) {
        _impl.emplace_back(forward<_Args>(__args)...);
    }

    template <class _uValue = value_type, class... _Args>
    FORCE_INLINE CONSTEXPR enable_if_t<!is_container_v<_uValue>, void> _init_(_Args&&... __args) {
        throw_if(static_cast<bool_t>(_impl), redundant_key{});
        _impl = value_type(forward<_Args>(__args)...);
    }

public:
    template <class _Key, class _Value, class _uManager>
    FORCE_INLINE CONSTEXPR subscriber(_Key&& __key, _Value&& __value, _uManager&& __manager)
            : base(forward<_uManager>(__manager)), _impl{_manager_()[forward<_Key>(__key)]} {
        _init_(forward<_Value>(__value));
    }
};

};  // namespace data

};  // namespace qlib

#endif

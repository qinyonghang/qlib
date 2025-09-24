#pragma once

#include "qlib/memory.hpp"

namespace qlib {

namespace any {
class bad_any_cast final : public exception {
public:
    char const* what() const noexcept override { return "bad any cast"; }
};

template <class Allocator = new_allocator_t>
class value : public object {
public:
    using base = object;
    using self = value;
    using allocator_type = Allocator;

protected:
    class manager : public traits<Allocator>::reference {
    public:
        using base = typename traits<Allocator>::reference;
        using self = manager;
        friend class value;

    protected:
        FORCE_INLINE CONSTEXPR allocator_type& _allocator_() noexcept {
            return static_cast<base&>(*this);
        }

        FORCE_INLINE CONSTEXPR allocator_type& _allocator_() const noexcept {
            return static_cast<base&>(const_cast<self&>(*this));
        }

    public:
        manager() = default;
        manager(manager const&) = default;
        manager(manager&&) = default;
        FORCE_INLINE CONSTEXPR manager(allocator_type& __a) : base(__a) {}

        virtual void construct(manager*) const {}
        virtual void move_construct(manager*) {}
        virtual void destroy() {}
        virtual bool_t has_value() const { return False; }
        virtual void* get() { return nullptr; }
        virtual void const* get() const { return nullptr; }
#ifdef _TYPEINFO
        virtual std::type_info const& type() const { return typeid(void); }
#endif
    };

    template <class _Tp, bool_t = (sizeof(_Tp) <= 8u)>
    class derived;

    template <class _Tp>
    class derived<_Tp, False> : public manager {
    public:
        using base = manager;
        using self = derived;
        using value_type = _Tp;

    protected:
        value_type* _impl{nullptr};

    public:
        template <class _Up>
        derived(_Up&& __value) : _impl(base::_allocator_().template allocate<value_type>(1u)) {
            new (_impl) value_type(forward<_Up>(__value));
        }

        template <class _Up>
        derived(_Up&& __value, allocator_type& __a)
                : base(__a), _impl(base::_allocator_().template allocate<value_type>(1u)) {
            new (_impl) value_type(forward<_Up>(__value));
        }

        void construct(manager* __ptr) const override {
            new (__ptr) derived<value_type>(*_impl, base::_allocator_());
        }

        void move_construct(manager* __ptr) override {
            new (__ptr) derived<value_type>(move(*_impl), base::_allocator_());
        }

        void destroy() override {
            _impl->~value_type();
            base::_allocator_().template deallocate(_impl, 1u);
        }

        bool_t has_value() const override { return True; }

        void* get() override { return _impl; }
        void const* get() const override { return _impl; }

#ifdef _TYPEINFO
        std::type_info const& type() const override { return typeid(value_type); }
#endif
    };

    template <class _Tp>
    class derived<_Tp, True> : public manager {
    public:
        using base = manager;
        using self = derived;

    protected:
        aligned_storage<8u, 8u>::type _impl;

    public:
        template <class _Up>
        derived(_Up&& __value) {
            new (&_impl) _Tp(forward<_Up>(__value));
        }

        template <class _Up>
        derived(_Up&& __value, allocator_type& __a) : base(__a) {
            new (&_impl) _Tp(forward<_Up>(__value));
        }

        _Tp* _get_() noexcept { return (_Tp*)(&_impl); }
        _Tp const* _get_() const noexcept { return (_Tp*)(&_impl); }

        void construct(manager* __ptr) const override {
            new (__ptr) derived<_Tp>(*_get_(), base::_allocator_());
        }

        void move_construct(manager* __ptr) override {
            new (__ptr) derived<_Tp>(move(*_get_()), base::_allocator_());
        }

        void destroy() override { _get_()->~_Tp(); }

        bool_t has_value() const override { return True; }

        void* get() override { return &_impl; }
        void const* get() const override { return &_impl; }

#ifdef _TYPEINFO
        std::type_info const& type() const override { return typeid(_Tp); }
#endif
    };

    typename aligned_storage<sizeof(derived<void*>), alignof(derived<void*>)>::type _storage;

    NODISCARD FORCE_INLINE manager* _manager_() noexcept { return (manager*)(&_storage); }
    NODISCARD FORCE_INLINE manager const* _manager_() const noexcept {
        return (manager*)(&_storage);
    }

public:
    FORCE_INLINE CONSTEXPR value() { new (&_storage) manager(); }
    FORCE_INLINE CONSTEXPR value(allocator_type& __a) { new (&_storage) manager(__a); }
    FORCE_INLINE CONSTEXPR value(self const& __a) { __a._manager_()->construct(_manager_()); }
    FORCE_INLINE CONSTEXPR value(self&& __a) { __a._manager_()->move_construct(_manager_()); }

    template <class _Tp, class Enable = enable_if_t<!is_same_v<remove_cvref_t<_Tp>, self>>>
    FORCE_INLINE CONSTEXPR value(_Tp&& value) {
        new (&_storage) derived<remove_cvref_t<_Tp>>(forward<_Tp>(value));
    }

    template <class _Tp, class Enable = enable_if_t<!is_same_v<remove_cvref_t<_Tp>, self>>>
    FORCE_INLINE CONSTEXPR value(_Tp&& value, allocator_type& __a) {
        new (&_storage) derived<remove_cvref_t<_Tp>>(forward<_Tp>(value), __a);
    }

    FORCE_INLINE ~value() { _manager_()->destroy(); }

    FORCE_INLINE self& operator=(self const& __a) {
        if (unlikely(this != &__a)) {
            _manager_()->destroy();
            __a._manager_()->construct(_manager_());
        }
        return *this;
    }

    FORCE_INLINE self& operator=(self&& __a) {
        _manager_()->destroy();
        __a._manager_()->move_construct(_manager_());
        return *this;
    }

    template <class _Tp>
    FORCE_INLINE self& operator=(_Tp&& value) {
        auto& allocator = _manager_()->_allocator_();
        _manager_()->destroy();
        new (&_storage) derived<remove_cvref_t<_Tp>>(forward<_Tp>(value), allocator);
        return *this;
    }

    NODISCARD FORCE_INLINE auto has_value() const noexcept -> bool_t {
        return _manager_()->has_value();
    }

#ifdef _TYPEINFO
    NODISCARD FORCE_INLINE auto& type() const noexcept { return _manager_()->type(); }
#endif

    // void swap(self& __a) noexcept {

    // }

    FORCE_INLINE void reset() {
        _manager_()->destroy();
        new (&_storage) manager();
    }

    template <class _Tp>
    NODISCARD FORCE_INLINE auto get() -> _Tp {
        using _Up = remove_cvref_t<_Tp>;
        auto ptr = _manager_()->get();
        throw_if(ptr == nullptr, bad_any_cast{});
        return *((_Up*)(ptr));
    }

    template <class _Tp>
    NODISCARD FORCE_INLINE auto get() const -> _Tp {
        using _Up = remove_cvref_t<_Tp>;
        auto ptr = _manager_()->get();
        throw_if(ptr == nullptr, bad_any_cast{});
        return *((_Up const*)(ptr));
    }
};

};  // namespace any

template <class Allocator = new_allocator_t>
using any_t = any::value<Allocator>;

template <class _Tp, class Allocator = new_allocator_t>
NODISCARD INLINE _Tp any_cast(any_t<Allocator> const& __a) {
    return __a.template get<remove_cvref_t<_Tp> const&>();
}

template <class _Tp, class Allocator = new_allocator_t>
NODISCARD INLINE _Tp any_cast(any_t<Allocator>& __a) {
    return __a.template get<remove_cvref_t<_Tp>&>();
}

template <class _Tp, class Allocator = new_allocator_t>
NODISCARD INLINE _Tp any_cast(any_t<Allocator>&& __a) {
    return move(__a.template get<remove_cvref_t<_Tp>&>());
}
};  // namespace qlib

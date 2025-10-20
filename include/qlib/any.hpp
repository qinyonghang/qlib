#ifndef QLIB_ANY_HPP
#define QLIB_ANY_HPP

#include "qlib/object.hpp"

namespace qlib {

namespace any {

class value : public object {
public:
    using base = object;
    using self = value;
    using size_type = size_t;
    constexpr static size_type threshold = 24u;

protected:
    class manager : public object {
    public:
        using base = object;
        using self = manager;
        friend class value;

    public:
        virtual void construct(manager*) const {}
        virtual void move_construct(manager*) {}
        virtual void destroy() {}
        virtual bool_t has_value() const { return False; }
#ifdef _TYPEINFO
        virtual std::type_info const& type() const { return typeid(void); }
#endif
    };

    template <class _Tp, bool_t = (sizeof(_Tp) <= threshold)>
    class derived;

    template <class _Tp>
    class derived<_Tp, False> : public manager {
    public:
        using base = manager;
        using self = derived;
        using value_type = _Tp;
        friend class value;

    protected:
        value_type* _impl{nullptr};

    public:
        ALWAYS_INLINE derived(value_type* __impl) : _impl{__impl} {}

        template <class _Up>
        derived(_Up&& __value) : _impl(new value_type(forward<_Up>(__value))) {}

        void construct(manager* __ptr) const override { new (__ptr) derived<value_type>(*_impl); }

        void move_construct(manager* __ptr) override {
            new (__ptr) derived<value_type>(_impl);
            new (this) manager();
        }

        void destroy() override { delete _impl; }

        bool_t has_value() const override { return _impl != nullptr; }

#ifdef _TYPEINFO
        std::type_info const& type() const override { return typeid(value_type); }
#endif
    };

    template <class _Tp>
    class derived<_Tp, True> : public manager {
    public:
        using base = manager;
        using self = derived;
        friend class value;

    protected:
        aligned_storage<threshold, 8u>::type _impl;

    public:
        ALWAYS_INLINE derived(decltype(_impl) __impl) : _impl(__impl) {}

        template <class _Up>
        derived(_Up&& __value) {
            new (&_impl) _Tp(forward<_Up>(__value));
        }

        _Tp* _get_() noexcept { return (_Tp*)(&_impl); }
        _Tp const* _get_() const noexcept { return (_Tp*)(&_impl); }

        void construct(manager* __ptr) const override { new (__ptr) derived<_Tp>(*_get_()); }

        void move_construct(manager* __ptr) override {
            new (__ptr) derived<_Tp>(_impl);
            new (this) manager();
        }

        void destroy() override { _get_()->~_Tp(); }

        bool_t has_value() const override { return True; }

#ifdef _TYPEINFO
        std::type_info const& type() const override { return typeid(_Tp); }
#endif
    };

    typename aligned_storage<sizeof(derived<void*>), alignof(derived<void*>)>::type _storage{};

    NODISCARD ALWAYS_INLINE manager* _manager_() noexcept { return (manager*)(&_storage); }
    NODISCARD ALWAYS_INLINE manager const* _manager_() const noexcept {
        return (manager*)(&_storage);
    }

public:
    ALWAYS_INLINE CONSTEXPR value() { new (&_storage) manager(); }
    ALWAYS_INLINE CONSTEXPR value(self const& __a) { __a._manager_()->construct(_manager_()); }
    ALWAYS_INLINE CONSTEXPR value(self&& __a) { __a._manager_()->move_construct(_manager_()); }

    ALWAYS_INLINE CONSTEXPR value(nullptr_t) : value() {}

    template <class _Tp, class Enable = enable_if_t<!is_same_v<remove_cvref_t<_Tp>, self>>>
    ALWAYS_INLINE CONSTEXPR value(_Tp&& value) {
        new (&_storage) derived<remove_cvref_t<_Tp>>(forward<_Tp>(value));
    }

    ALWAYS_INLINE ~value() { _manager_()->destroy(); }

    self& operator=(self const& __a) {
        if (unlikely(this != &__a)) {
            _manager_()->destroy();
            __a._manager_()->construct(_manager_());
        }
        return *this;
    }

    self& operator=(self&& __a) {
        if (unlikely(this != &__a)) {
            _manager_()->destroy();
            __a._manager_()->move_construct(_manager_());
        }
        return *this;
    }

    self& operator=(nullptr_t) {
        _manager_()->destroy();
        new (&_storage) manager();
        return *this;
    }

    template <class _Tp, class Enable = enable_if_t<!is_same_v<remove_cvref_t<_Tp>, self>>>
    self& operator=(_Tp&& value) {
        _manager_()->destroy();
        new (&_storage) derived<remove_cvref_t<_Tp>>(forward<_Tp>(value));
        return *this;
    }

    NODISCARD auto has_value() const noexcept -> bool_t { return _manager_()->has_value(); }

#ifdef _TYPEINFO
    NODISCARD auto& type() const noexcept { return _manager_()->type(); }
#endif

    ALWAYS_INLINE void reset() {
        _manager_()->destroy();
        new (&_storage) manager();
    }

    template <class _Tp>
    NODISCARD ALWAYS_INLINE enable_if_t<sizeof(remove_cvref_t<_Tp>) <= threshold, _Tp> get() {
        using _Up = remove_cvref_t<_Tp>;
        throw_if(_manager_()->type() != typeid(_Up), "bad any cast");
        auto __ptr = static_cast<derived<_Up>*>(_manager_());
        return *((_Up*)(&__ptr->_impl));
    }

    template <class _Tp>
    NODISCARD ALWAYS_INLINE enable_if_t<!(sizeof(remove_cvref_t<_Tp>) <= threshold), _Tp> get() {
        using _Up = remove_cvref_t<_Tp>;
        throw_if(_manager_()->type() != typeid(_Up), "bad any cast");
        auto __ptr = static_cast<derived<_Up>*>(_manager_());
        return *((_Up*)(__ptr->_impl));
    }

    template <class _Tp>
    _Tp get() const {
        return const_cast<self&>(*this).template get<_Tp>();
    }
};

};  // namespace any

using any_t = any::value;

template <class _Tp>
_Tp any_cast(any_t const& __a) {
    return __a.template get<remove_cvref_t<_Tp> const&>();
}

template <class _Tp>
_Tp any_cast(any_t& __a) {
    return __a.template get<remove_cvref_t<_Tp>&>();
}

template <class _Tp>
_Tp any_cast(any_t&& __a) {
    return move(__a.template get<remove_cvref_t<_Tp>&>());
}
};  // namespace qlib

#endif

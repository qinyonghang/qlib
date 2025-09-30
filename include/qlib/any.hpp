#pragma once

#include "qlib/memory.hpp"

namespace qlib {

namespace any {
class bad_any_cast final : public exception {
public:
    char const* what() const noexcept override { return "bad any cast"; }
};

class value : public object {
public:
    using base = object;
    using self = value;

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
        derived(_Up&& __value) : _impl(new value_type(forward<_Up>(__value))) {}

        void construct(manager* __ptr) const override { new (__ptr) derived<value_type>(*_impl); }

        void move_construct(manager* __ptr) override {
            new (__ptr) derived<value_type>(move(*_impl));
        }

        void destroy() override { delete _impl; }

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

        _Tp* _get_() noexcept { return (_Tp*)(&_impl); }
        _Tp const* _get_() const noexcept { return (_Tp*)(&_impl); }

        void construct(manager* __ptr) const override { new (__ptr) derived<_Tp>(*_get_()); }

        void move_construct(manager* __ptr) override { new (__ptr) derived<_Tp>(move(*_get_())); }

        void destroy() override { _get_()->~_Tp(); }

        bool_t has_value() const override { return True; }

        void* get() override { return &_impl; }
        void const* get() const override { return &_impl; }

#ifdef _TYPEINFO
        std::type_info const& type() const override { return typeid(_Tp); }
#endif
    };

    typename aligned_storage<sizeof(derived<void*>), alignof(derived<void*>)>::type _storage{};

    NODISCARD FORCE_INLINE manager* _manager_() noexcept { return (manager*)(&_storage); }
    NODISCARD FORCE_INLINE manager const* _manager_() const noexcept {
        return (manager*)(&_storage);
    }

public:
    FORCE_INLINE CONSTEXPR value() { new (&_storage) manager(); }
    FORCE_INLINE CONSTEXPR value(self const& __a) { __a._manager_()->construct(_manager_()); }
    FORCE_INLINE CONSTEXPR value(self&& __a) { __a._manager_()->move_construct(_manager_()); }

    template <class _Tp, class Enable = enable_if_t<!is_same_v<remove_cvref_t<_Tp>, self>>>
    FORCE_INLINE CONSTEXPR value(_Tp&& value) {
        new (&_storage) derived<remove_cvref_t<_Tp>>(forward<_Tp>(value));
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
        _manager_()->destroy();
        new (&_storage) derived<remove_cvref_t<_Tp>>(forward<_Tp>(value));
        return *this;
    }

    NODISCARD FORCE_INLINE auto has_value() const noexcept -> bool_t {
        return _manager_()->has_value();
    }

#ifdef _TYPEINFO
    NODISCARD FORCE_INLINE auto& type() const noexcept { return _manager_()->type(); }
#endif

    FORCE_INLINE void reset() {
        _manager_()->destroy();
        new (&_storage) manager();
    }

    template <class _Tp>
    NODISCARD FORCE_INLINE auto get() -> _Tp {
        using _Up = remove_cvref_t<_Tp>;
        throw_if(!has_value() || _manager_()->type() != typeid(_Up), bad_any_cast{});
        auto ptr = _manager_()->get();
        return *((_Up*)(ptr));
    }

    template <class _Tp>
    NODISCARD FORCE_INLINE auto get() const -> _Tp {
        using _Up = remove_cvref_t<_Tp>;
        throw_if(!has_value() || _manager_()->type() != typeid(_Up), bad_any_cast{});
        auto ptr = _manager_()->get();
        return *((_Up const*)(ptr));
    }
};

};  // namespace any

using any_t = any::value;

template <class _Tp>
NODISCARD INLINE _Tp any_cast(any_t const& __a) {
    return __a.template get<remove_cvref_t<_Tp> const&>();
}

template <class _Tp>
NODISCARD INLINE _Tp any_cast(any_t& __a) {
    return __a.template get<remove_cvref_t<_Tp>&>();
}

template <class _Tp>
NODISCARD INLINE _Tp any_cast(any_t&& __a) {
    return move(__a.template get<remove_cvref_t<_Tp>&>());
}
};  // namespace qlib

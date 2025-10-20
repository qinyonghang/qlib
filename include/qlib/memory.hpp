#ifndef QLIB_MEMORY_HPP
#define QLIB_MEMORY_HPP

#include "qlib/object.h"

namespace qlib {

namespace memory {
class new_allocator;
class pool_allocator;
template <size_t Capacity>
class stack_allocator;
template <class _Tp>
class default_deleter;
};  // namespace memory

template <>
struct traits<memory::new_allocator> : public object {
    using reference = memory::new_allocator;
};

template <>
struct traits<memory::pool_allocator> : public object {
    using reference = qlib::reference<memory::pool_allocator>;
};

template <size_t Capacity>
struct traits<memory::stack_allocator<Capacity>> : public object {
    using reference = qlib::reference<memory::stack_allocator<Capacity>>;
};

template <class _Tp>
struct traits<memory::default_deleter<_Tp>> : public object {
    using reference = memory::default_deleter<_Tp>;
};

namespace memory {

template <class T1, class T2>
NODISCARD ALWAYS_INLINE CONSTEXPR static auto align_up(T1 size, T2 alignment) noexcept {
    return (size + (alignment - 1)) & ~(alignment - 1);
}

class bad_alloc final : public exception {
public:
    char const* what() const noexcept override { return "bad alloc"; }
};

class new_allocator : public object {
public:
    using base = object;
    using self = new_allocator;
    using size_type = size_t;

    template <class T>
    NODISCARD ALWAYS_INLINE CONSTEXPR static T* allocate(size_type n) noexcept {
        static_assert(sizeof(T) > 0, "cannot allocate zero-sized object");
        return (T*)(::operator new(n * sizeof(T)));
    }

    template <class T>
    ALWAYS_INLINE CONSTEXPR static void deallocate(T* p, size_type n ATTR_UNUSED) noexcept {
        ::operator delete(p
#if __cpp_sized_deallocation
                          ,
                          n * sizeof(T)
#endif
        );
    }

    template <class T, class... Args>
    ALWAYS_INLINE CONSTEXPR static void construct(T* p, Args&&... args) noexcept(
        is_nothrow_constructible_v<T, Args...>) {
        new (p) T(forward<Args>(args)...);
    }

    template <class T>
    ALWAYS_INLINE CONSTEXPR static void destroy(T* p) noexcept(is_nothrow_destructible_v<T>) {
        p->~T();
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR static auto max_size() noexcept { return size_type(-1); }
};

class pool_allocator final : public new_allocator {
public:
    using base = new_allocator;
    using self = pool_allocator;
    using size_type = size_t;

protected:
    struct alignas(8) node {
        node* next{nullptr};
        uint8_t* data{nullptr};
        size_type used{0u};
        size_type capacity{0u};
    };

    node* _list{nullptr};

public:
    ALWAYS_INLINE CONSTEXPR pool_allocator(size_type capacity = 64 * 1024) noexcept {
        _list = (node*)base::allocate<uint8_t>(sizeof(node) + capacity);
        _list->next = nullptr;
        _list->data = (uint8_t*)_list + sizeof(node);
        _list->used = 0u;
        _list->capacity = capacity;
    }

    ALWAYS_INLINE ~pool_allocator() noexcept {
        node* cur = _list;
        while (cur != nullptr) {
            node* next = cur->next;
            base::deallocate<uint8_t>((uint8_t*)(cur), sizeof(node) + cur->capacity);
            cur = next;
        }
    }

    template <class T>
    NODISCARD ALWAYS_INLINE CONSTEXPR T* allocate(size_type n) noexcept {
        static_assert(sizeof(T) > 0, "cannot allocate zero-sized object");

        size_type size = align_up(sizeof(T) * n, sizeof(void*));

        if (unlikely(_list->used + size > _list->capacity)) {
            size_type new_capacity = _list->capacity * 2;
            while (new_capacity < size) {
                new_capacity *= 2;
            }
            auto new_node = (node*)base::allocate<uint8_t>(sizeof(node) + new_capacity);
            new_node->next = _list;
            new_node->data = (uint8_t*)new_node + sizeof(node);
            new_node->used = 0u;
            new_node->capacity = new_capacity;
            _list = new_node;
        }

        auto ptr = (T*)(_list->data + _list->used);
        _list->used += size;
        return ptr;
    }

    template <class T>
    ALWAYS_INLINE CONSTEXPR void deallocate(T* p ATTR_UNUSED, size_type n ATTR_UNUSED) noexcept {}
};

template <size_t Capacity>
class stack_allocator final : public new_allocator {
public:
    using base = new_allocator;
    using self = stack_allocator;
    using size_type = size_t;

protected:
    size_type _used{0u};
    alignas(8) uint8_t _impl[Capacity];

public:
    template <class T>
    NODISCARD ALWAYS_INLINE CONSTEXPR T* allocate(size_type n) noexcept {
        static_assert(sizeof(T) > 0, "cannot allocate zero-sized object");
        size_type size = align_up(sizeof(T) * n, sizeof(void*));
        throw_if(_used + size > Capacity, bad_alloc());
        auto ptr = (T*)(_impl + _used);
        _used += size;
        return ptr;
    }

    template <class T>
    ALWAYS_INLINE CONSTEXPR void deallocate(T* p ATTR_UNUSED, uint64_t n ATTR_UNUSED) noexcept {}
};

template <class _Tp>
class default_deleter : public object {
public:
    using base = object;
    using self = default_deleter;
    using value_type = _Tp;

public:
    ALWAYS_INLINE CONSTEXPR void operator()(value_type* __p) noexcept { delete __p; }
};

template <class _Tp>
class default_deleter<_Tp[]> : public object {
public:
    using base = object;
    using self = default_deleter;
    using value_type = _Tp;

public:
    ALWAYS_INLINE CONSTEXPR void operator()(value_type* __p) noexcept { delete[] __p; }
};

template <class _Tp, class _Dp = default_deleter<_Tp>>
class unique_ptr final : public traits<_Dp>::reference {
public:
    using base = typename traits<_Dp>::reference;
    using self = unique_ptr;
    using value_type = remove_all_extents_t<_Tp>;
    using deleter_type = _Dp;

protected:
    value_type* _impl{nullptr};

    NODISCARD ALWAYS_INLINE CONSTEXPR deleter_type& _deleter_() noexcept {
        return static_cast<deleter_type&>(*this);
    }

public:
    unique_ptr(unique_ptr const&) = delete;
    unique_ptr& operator=(unique_ptr const&) = delete;

    ALWAYS_INLINE CONSTEXPR unique_ptr() noexcept(is_nothrow_constructible_v<base>) = default;
    ALWAYS_INLINE CONSTEXPR
    unique_ptr(deleter_type& __d) noexcept(is_nothrow_constructible_v<base, deleter_type&>)
            : base(__d) {}

    ALWAYS_INLINE CONSTEXPR
    unique_ptr(self&& __o) noexcept(is_nothrow_constructible_v<base, base&&>)
            : base(move(__o)), _impl{__o._impl} {
        __o._impl = nullptr;
    }
    ALWAYS_INLINE CONSTEXPR explicit unique_ptr(value_type* __p) noexcept : _impl(__p) {}
    template <class... _Args>
    ALWAYS_INLINE CONSTEXPR explicit unique_ptr(value_type* __p, _Args&&... __args) noexcept(
        is_nothrow_constructible_v<base, deleter_type&>)
            : base(forward<_Args>(__args)...), _impl(__p) {}

    ALWAYS_INLINE ~unique_ptr() noexcept {
        if (_impl != nullptr) {
            _deleter_()(_impl);
        }
    }

    ALWAYS_INLINE CONSTEXPR void reset(value_type* __p = nullptr) noexcept {
        if (_impl != nullptr) {
            _deleter_()(_impl);
        }
        _impl = __p;
    }

    ALWAYS_INLINE CONSTEXPR self& operator=(self&& __o) noexcept {
        base::operator=(move(__o._deleter_()));
        reset(__o._impl);
        __o._impl = nullptr;
        return *this;
    }

    ALWAYS_INLINE CONSTEXPR self& operator=(value_type* __p) noexcept {
        reset(__p);
        return *this;
    }

    template <class _Up = value_type>
    NODISCARD ALWAYS_INLINE CONSTEXPR enable_if_t<!is_same_v<_Up, void>, _Up&>
    operator*() noexcept {
        return *_impl;
    }

    template <class _Up = value_type>
    NODISCARD ALWAYS_INLINE CONSTEXPR enable_if_t<!is_same_v<_Up, void>, _Up const&> operator*()
        const noexcept {
        return *_impl;
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR value_type* operator->() noexcept { return _impl; }
    NODISCARD ALWAYS_INLINE CONSTEXPR value_type const* operator->() const noexcept {
        return _impl;
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR value_type* get() noexcept { return _impl; }
    NODISCARD ALWAYS_INLINE CONSTEXPR value_type const* get() const noexcept { return _impl; }

    NODISCARD ALWAYS_INLINE CONSTEXPR bool_t empty() const noexcept { return _impl == nullptr; }
    NODISCARD ALWAYS_INLINE CONSTEXPR explicit operator bool_t() const noexcept { return empty(); }
    NODISCARD ALWAYS_INLINE CONSTEXPR bool_t operator==(nullptr_t) const noexcept {
        return empty();
    }
    NODISCARD ALWAYS_INLINE CONSTEXPR bool_t operator!=(nullptr_t) const noexcept {
        return !empty();
    }
};

};  // namespace memory

using new_allocator_t = memory::new_allocator;
using pool_allocator_t = memory::pool_allocator;
template <size_t Capacity = 64 * 1024>
using stack_allocator_t = memory::stack_allocator<Capacity>;

template <class _Tp, class _Dp = memory::default_deleter<_Tp>>
using unique_ptr_t = memory::unique_ptr<_Tp, _Dp>;

template <class _Tp, class _Dp>
struct _is_pointer_helper<memory::unique_ptr<_Tp, _Dp>> : public true_type {};

template <class _Tp, class... _Args>
NODISCARD ALWAYS_INLINE CONSTEXPR auto make_unique(_Args&&... args) {
    return unique_ptr_t<_Tp>{new _Tp(forward<_Args>(args)...)};
}

};  // namespace qlib

#endif

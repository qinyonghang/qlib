#include <gtest/gtest.h>

#include "qlib/any.h"

namespace qlib {

class Object : public object {
protected:
    size_t _impl{0u};
    static inline size_t _counter{0u};

public:
    INLINE Object() noexcept { ++_counter; }

    INLINE Object(Object const& __o) noexcept : _impl(__o._impl) { ++_counter; }

    INLINE Object(Object&& __o) noexcept : _impl(std::move(__o._impl)) { ++_counter; }

    INLINE Object(size_t __impl) noexcept : _impl(__impl) { ++_counter; }

    INLINE ~Object() noexcept { --_counter; }

    INLINE Object& operator=(Object const& __o) noexcept {
        if (this != &__o) {
            _impl = __o._impl;
        }
        return *this;
    }

    INLINE Object& operator=(Object&& __o) noexcept {
        _impl = std::move(__o._impl);
        __o._impl = 0u;
        return *this;
    }

    NODISCARD FORCE_INLINE bool_t operator==(Object const& __o) const { return _impl == __o._impl; }

    NODISCARD FORCE_INLINE bool_t operator==(size_t __o) const { return _impl == __o; }

    template <class _Up>
    NODISCARD FORCE_INLINE bool_t operator!=(_Up const& __o) const {
        return !(*this == __o);
    }

    NODISCARD FORCE_INLINE static size_t counter() noexcept { return _counter; }
};

class BigObject : public object {
protected:
    size_t _impl{0u};
    size_t _reserve[9u];
    static inline size_t _counter{0u};

public:
    INLINE BigObject() noexcept { ++_counter; }

    INLINE BigObject(BigObject const& __o) noexcept : _impl(__o._impl) { ++_counter; }

    INLINE BigObject(BigObject&& __o) noexcept : _impl(std::move(__o._impl)) { ++_counter; }

    INLINE BigObject(size_t __impl) noexcept : _impl(__impl) { ++_counter; }

    INLINE ~BigObject() noexcept { --_counter; }

    INLINE BigObject& operator=(BigObject const& __o) noexcept {
        if (this != &__o) {
            _impl = __o._impl;
        }
        return *this;
    }

    INLINE BigObject& operator=(BigObject&& __o) noexcept {
        _impl = std::move(__o._impl);
        __o._impl = 0u;
        return *this;
    }

    NODISCARD FORCE_INLINE bool_t operator==(BigObject const& __o) const {
        return _impl == __o._impl;
    }

    NODISCARD FORCE_INLINE bool_t operator==(size_t __o) const { return _impl == __o; }

    template <class _Up>
    NODISCARD FORCE_INLINE bool_t operator!=(_Up const& __o) const {
        return !(*this == __o);
    }

    NODISCARD FORCE_INLINE static size_t counter() noexcept { return _counter; }
};

template <class Any, class Object>
static void test() {
    typename Any::allocator_type allocator;
    Any value(allocator);
    bool_t throw_bad_any_cast{False};
    try {
        auto object = any_cast<Object>(value);
    } catch (any::bad_any_cast const&) {
        throw_bad_any_cast = True;
    }
    EXPECT_EQ(throw_bad_any_cast, True);
    value = Object{0u};
    throw_bad_any_cast = False;
    try {
        auto object = any_cast<Object>(value);
        EXPECT_EQ(object, 0u);
        EXPECT_NE(object, 1u);
    } catch (any::bad_any_cast const&) {
        throw_bad_any_cast = True;
    }
    EXPECT_EQ(throw_bad_any_cast, False);
    throw_bad_any_cast = False;
    try {
        auto& object = any_cast<Object&>(value);
        EXPECT_EQ(object, 0u);
        EXPECT_NE(object, 1u);
    } catch (any::bad_any_cast const&) {
        throw_bad_any_cast = True;
    }
    EXPECT_EQ(throw_bad_any_cast, False);
    throw_bad_any_cast = False;
    try {
        auto&& object = any_cast<Object&&>(move(value));
        EXPECT_EQ(object, 0u);
        EXPECT_NE(object, 1u);
    } catch (any::bad_any_cast const&) {
        throw_bad_any_cast = True;
    }
    EXPECT_EQ(throw_bad_any_cast, False);
    value = nullptr;
    EXPECT_EQ(Object::counter(), 0u);
}
};  // namespace qlib

TEST(qlib, Any) {
    using Any = qlib::any_t<>;
    EXPECT_EQ(sizeof(Any), 16u);
    qlib::test<Any, qlib::Object>();
};

TEST(qlib, AnyPool) {
    using Any = qlib::any_t<qlib::pool_allocator_t>;
    EXPECT_EQ(sizeof(Any), 24u);
    qlib::test<Any, qlib::Object>();
};

TEST(qlib, AnyBig) {
    using Any = qlib::any_t<>;
    EXPECT_EQ(sizeof(Any), 16u);
    qlib::test<Any, qlib::BigObject>();
}

TEST(qlib, AnyBigPool) {
    using Any = qlib::any_t<qlib::pool_allocator_t>;
    EXPECT_EQ(sizeof(Any), 24u);
    qlib::test<Any, qlib::BigObject>();
}

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    } while (false);

    return result;
}

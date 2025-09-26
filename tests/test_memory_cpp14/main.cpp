#include <gtest/gtest.h>

#include "qlib/memory.h"

namespace test {
class Object : public qlib::object {
protected:
    size_t _impl{0u};

public:
    INLINE Object() noexcept { std::cout << "Object()" << std::endl; }

    INLINE Object(Object const& __o) noexcept : _impl(__o._impl) {
        std::cout << "Object(Object const&)" << std::endl;
    }

    INLINE Object(Object&& __o) noexcept : _impl(std::move(__o._impl)) {
        std::cout << "Object(Object&&)" << std::endl;
    }

    INLINE Object(size_t __impl) noexcept : _impl(__impl) {
        std::cout << "Object(size_t)" << std::endl;
    }

    INLINE ~Object() noexcept { std::cout << "~Object()" << std::endl; }

    INLINE Object& operator=(Object const& __o) noexcept {
        std::cout << "Object::operator=(Object const&)" << std::endl;
        if (this != &__o) {
            _impl = __o._impl;
        }
        return *this;
    }

    INLINE Object& operator=(Object&& __o) noexcept {
        std::cout << "Object::operator=(Object&&)" << std::endl;
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
};
};  // namespace test

TEST(Json, UniquePtr) {
    using value_type = qlib::unique_ptr_t<test::Object>;
    EXPECT_EQ(sizeof(value_type), sizeof(void*));
    value_type value{new test::Object{}};
    EXPECT_EQ(*value, 0u);
    value_type other{std::move(value)};
    EXPECT_EQ(*other, test::Object{0u});
    EXPECT_NE(*other, test::Object{1u});
}

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    } while (false);

    return result;
}

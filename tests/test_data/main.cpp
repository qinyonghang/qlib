#include <gtest/gtest.h>

#include "qlib/data.h"
#include "qlib/string.h"

namespace qlib {

class Object : public object {
protected:
    size_t _impl{0u};
    static inline size_t _counter{0u};

public:
    ALWAYS_INLINE Object() noexcept { ++_counter; }

    ALWAYS_INLINE Object(Object const& __o) noexcept : _impl(__o._impl) { ++_counter; }

    ALWAYS_INLINE Object(Object&& __o) noexcept : _impl(std::move(__o._impl)) { ++_counter; }

    ALWAYS_INLINE Object(size_t __impl) noexcept : _impl(__impl) { ++_counter; }

    ALWAYS_INLINE ~Object() noexcept { --_counter; }

    ALWAYS_INLINE Object& operator=(Object const& __o) noexcept {
        if (this != &__o) {
            _impl = __o._impl;
        }
        return *this;
    }

    ALWAYS_INLINE Object& operator=(Object&& __o) noexcept {
        _impl = std::move(__o._impl);
        __o._impl = 0u;
        return *this;
    }

    NODISCARD ALWAYS_INLINE bool_t operator==(Object const& __o) const { return _impl == __o._impl; }

    NODISCARD ALWAYS_INLINE bool_t operator==(size_t __o) const { return _impl == __o; }

    template <class _Up>
    NODISCARD ALWAYS_INLINE bool_t operator!=(_Up const& __o) const {
        return !(*this == __o);
    }

    NODISCARD ALWAYS_INLINE static size_t counter() noexcept { return _counter; }
};

class BigObject : public object {
protected:
    size_t _impl{0u};
    size_t _reserve[9u];
    static inline size_t _counter{0u};

public:
    ALWAYS_INLINE BigObject() noexcept { ++_counter; }

    ALWAYS_INLINE BigObject(BigObject const& __o) noexcept : _impl(__o._impl) { ++_counter; }

    ALWAYS_INLINE BigObject(BigObject&& __o) noexcept : _impl(std::move(__o._impl)) { ++_counter; }

    ALWAYS_INLINE BigObject(size_t __impl) noexcept : _impl(__impl) { ++_counter; }

    ALWAYS_INLINE ~BigObject() noexcept { --_counter; }

    ALWAYS_INLINE BigObject& operator=(BigObject const& __o) noexcept {
        if (this != &__o) {
            _impl = __o._impl;
        }
        return *this;
    }

    ALWAYS_INLINE BigObject& operator=(BigObject&& __o) noexcept {
        _impl = std::move(__o._impl);
        __o._impl = 0u;
        return *this;
    }

    NODISCARD ALWAYS_INLINE bool_t operator==(BigObject const& __o) const {
        return _impl == __o._impl;
    }

    NODISCARD ALWAYS_INLINE bool_t operator==(size_t __o) const { return _impl == __o; }

    template <class _Up>
    NODISCARD ALWAYS_INLINE bool_t operator!=(_Up const& __o) const {
        return !(*this == __o);
    }

    NODISCARD ALWAYS_INLINE static size_t counter() noexcept { return _counter; }
};

};  // namespace qlib

TEST(qlib, Data) {
    using namespace qlib;
    using Manager = data::manager<string_view_t, std::function<void(int32_t)>>;
    using Publisher = data::publisher<Manager>;
    using Subscriber = data::subscriber<Manager>;
    Manager manager;
    {
        Publisher publisher("test1", manager);
        Subscriber subscriber(
            "test1",
            [](int32_t value) {
                EXPECT_EQ(value, 42);
                std::cout << "Call" << std::endl;
            },
            manager);
        publisher.publish(42);
    }
    {
        Subscriber subscriber(
            "test2",
            [](int32_t value) {
                EXPECT_EQ(value, 31);
                std::cout << "Call" << std::endl;
            },
            manager);
        Publisher publisher("test2", manager);
        publisher.publish(31);
    }
};

TEST(qlib, DataRedundant) {
    using namespace qlib;
    using Manager = data::manager<string_view_t, std::function<void(int32_t)>>;
    using Publisher = data::publisher<Manager>;
    using Subscriber = data::subscriber<Manager>;
    Manager manager;

    Publisher publisher("test1", manager);
    bool_t throw_redundant_key{False};
    try {
        Subscriber subscriber1("test1", [](int32_t value) { EXPECT_EQ(value, 42); }, manager);
        Subscriber subscriber2("test1", [](int32_t value) { EXPECT_EQ(value, 42); }, manager);
    } catch (data::redundant_key const&) {
        throw_redundant_key = True;
    }
    EXPECT_TRUE(throw_redundant_key);
    publisher.publish(42);
};

TEST(qlib, DataVector) {
    using namespace qlib;
    using Manager = data::manager<string_view_t, vector_t<std::function<void(int32_t)>>>;
    using Publisher = data::publisher<Manager>;
    using Subscriber = data::subscriber<Manager>;
    Manager manager;

    {
        Publisher publisher("test1", manager);
        Subscriber subscriber1("test1", [](int32_t value) { EXPECT_EQ(value, 42); }, manager);
        Subscriber subscriber2("test1", [](int32_t value) { EXPECT_EQ(value, 42); }, manager);
        publisher.publish(42);
    }
    {
        Subscriber subscriber1("test2", [](int32_t value) { EXPECT_EQ(value, 31); }, manager);
        Subscriber subscriber2("test2", [](int32_t value) { EXPECT_EQ(value, 31); }, manager);
        Publisher publisher("test2", manager);
        publisher.publish(31);
    }
};

TEST(qlib, DataAllocator) {
    using namespace qlib;
    using Manager = data::manager<string_view_t, std::function<void(int32_t)>, pool_allocator_t>;
    using Publisher = data::publisher<Manager>;
    using Subscriber = data::subscriber<Manager>;
    pool_allocator_t allocator;
    Manager manager(allocator);
    {
        Publisher publisher("test1", manager);
        Subscriber subscriber("test1", [](int32_t value) { EXPECT_EQ(value, 42); }, manager);
        publisher.publish(42);
    }
    {
        Subscriber subscriber("test2", [](int32_t value) { EXPECT_EQ(value, 31); }, manager);
        Publisher publisher("test2", manager);
        publisher.publish(31);
    }
};

TEST(qlib, DataVectorAllocator) {
    using namespace qlib;
    using Manager =
        data::manager<string_view_t, vector_t<std::function<void(int32_t)>>, pool_allocator_t>;
    using Publisher = data::publisher<Manager>;
    using Subscriber = data::subscriber<Manager>;
    pool_allocator_t allocator;
    Manager manager(allocator);

    {
        Publisher publisher("test1", manager);
        Subscriber subscriber1("test1", [](int32_t value) { EXPECT_EQ(value, 42); }, manager);
        Subscriber subscriber2("test1", [](int32_t value) { EXPECT_EQ(value, 42); }, manager);
        publisher.publish(42);
    }
    {
        Subscriber subscriber1("test2", [](int32_t value) { EXPECT_EQ(value, 31); }, manager);
        Subscriber subscriber2("test2", [](int32_t value) { EXPECT_EQ(value, 31); }, manager);
        Publisher publisher("test2", manager);
        publisher.publish(31);
    }
};

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    } while (false);

    return result;
}

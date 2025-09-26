#include <gtest/gtest.h>

#include "qlib/data.h"
#include "qlib/string.h"

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

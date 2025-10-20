#include <gtest/gtest.h>

#include "qlib/any.h"

namespace qlib {

class Object : public object {
protected:
    size_t _impl{0u};
    static size_t _counter;

public:
    Object() noexcept { ++_counter; }
    Object(Object const& __o) noexcept : _impl(__o._impl) { ++_counter; }
    Object(Object&& __o) noexcept : _impl(__o._impl) { ++_counter; }
    Object(size_t __impl) noexcept : _impl(__impl) { ++_counter; }
    ~Object() noexcept { --_counter; }

    Object& operator=(Object const& __o) noexcept {
        if (this != &__o) {
            _impl = __o._impl;
        }
        return *this;
    }

    Object& operator=(Object&& __o) noexcept {
        if (this != &__o) {
            _impl = std::move(__o._impl);
            __o._impl = 0u;
        }
        return *this;
    }

    bool_t operator==(Object const& __o) const { return _impl == __o._impl; }
    bool_t operator==(size_t __o) const { return _impl == __o; }

    template <class _Up>
    bool_t operator!=(_Up const& __o) const {
        return !(*this == __o);
    }

    static size_t counter() noexcept { return _counter; }
};

size_t Object::_counter{0u};

class Object1 : public Object {
public:
    using Object::Object;
};

class Object2 : public Object {
public:
    size_t _reserve[1u];
    using Object::Object;
};

class Object3 : public Object {
public:
    size_t _reserve[2u];
    using Object::Object;
};

class Object4 : public Object {
public:
    size_t _reserve[3u];
    using Object::Object;
};

template <class Any, class Object>
static void test_basic() {
    Any value;
    bool_t throw_exception{False};
    try {
        auto object = any_cast<Object>(value);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, True);
    value = Object{0u};
    throw_exception = False;
    try {
        auto object = any_cast<Object>(value);
        EXPECT_EQ(object, 0u);
        EXPECT_NE(object, 1u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
    throw_exception = False;
    try {
        auto& object = any_cast<Object&>(value);
        EXPECT_EQ(object, 0u);
        EXPECT_NE(object, 1u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
    throw_exception = False;
    try {
        auto&& object = any_cast<Object&&>(move(value));
        EXPECT_EQ(object, 0u);
        EXPECT_NE(object, 1u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
    value = nullptr;
    EXPECT_EQ(Object::counter(), 0u);
}

// 测试移动语义
template <class Any, class Object>
static void test_move_semantics() {
    // 测试移动构造
    Any value1{Object{42u}};
    Any value2{std::move(value1)};

    bool_t throw_exception{False};
    try {
        auto object = any_cast<Object>(value2);
        EXPECT_EQ(object, 42u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);

    // 测试移动赋值
    Any value3;
    value3 = std::move(value2);

    throw_exception = False;
    try {
        auto object = any_cast<Object>(value3);
        EXPECT_EQ(object, 42u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
}

// 测试复制语义
template <class Any, class Object>
static void test_copy_semantics() {
    // 测试拷贝构造
    Any value1{Object{99u}};
    Any value2{value1};

    bool_t throw_exception{False};
    try {
        auto object1 = any_cast<Object>(value1);
        auto object2 = any_cast<Object>(value2);
        EXPECT_EQ(object1, object2);
        EXPECT_EQ(object1, 99u);
    } catch (exception const& e) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);

    // 测试拷贝赋值
    Any value3;
    value3 = value1;
    throw_exception = False;
    try {
        auto object = any_cast<Object>(value3);
        EXPECT_EQ(object, 99u);
    } catch (exception const& e) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
}

// 测试重置功能
template <class Any, class Object>
static void test_reset() {
    Any value{Object{123u}};

    // 确认值存在
    EXPECT_TRUE(value.has_value());

    // 重置
    value.reset();

    // 确认值已被清除
    EXPECT_FALSE(value.has_value());

    // 尝试访问应该抛出异常
    bool_t throw_exception{False};
    try {
        auto object = any_cast<Object>(value);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, True);
}

// 测试赋值操作
template <class Any, class Object>
static void test_assignment() {
    Any value;

    // 默认构造的 any 应该没有值
    EXPECT_FALSE(value.has_value());

    // 赋值
    value = Object{55u};

    // 确认有值
    EXPECT_TRUE(value.has_value());

    bool_t throw_exception{False};
    try {
        auto object = any_cast<Object>(value);
        EXPECT_EQ(object, 55u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);

    // 重新赋值
    value = Object{66u};

    throw_exception = False;
    try {
        auto object = any_cast<Object>(value);
        EXPECT_EQ(object, 66u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
}

// 测试多个不同类型存储
template <class Any>
static void test_multiple_types() {
    Any value1{Object1{1u}};
    Any value2{Object2{2u}};
    Any value3{Object3{3u}};
    Any value4{Object4{4u}};

    bool_t throw_exception{False};
    try {
        auto obj1 = any_cast<Object1>(value1);
        auto obj2 = any_cast<Object2>(value2);
        auto obj3 = any_cast<Object3>(value3);
        auto obj4 = any_cast<Object4>(value4);
        EXPECT_EQ(obj1, 1u);
        EXPECT_EQ(obj2, 2u);
        EXPECT_EQ(obj3, 3u);
        EXPECT_EQ(obj4, 4u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
}

// 测试自我赋值
template <class Any, class Object>
static void test_self_assignment() {
    Any value{Object{77u}};

    // 拷贝自我赋值
    value = value;

    bool_t throw_exception{False};
    try {
        auto object = any_cast<Object>(value);
        EXPECT_EQ(object, 77u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);

    // 移动自我赋值（实际上是拷贝，因为移动自我赋值被阻止了）
    value = std::move(value);

    throw_exception = False;
    try {
        auto object = any_cast<Object>(value);
        EXPECT_EQ(object, 77u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
}

// 测试大小对象的混合操作
template <class Any>
static void test_mixed_operations() {
    // 小对象操作
    Any small_value{Object1{10u}};
    Any small_copy{small_value};
    Any small_moved{std::move(small_value)};

    // 大对象操作
    Any big_value{Object4{20u}};
    Any big_copy{big_value};
    Any big_moved{std::move(big_value)};

    bool_t throw_exception{False};
    try {
        auto small_obj = any_cast<Object1>(small_moved);
        auto big_obj = any_cast<Object4>(big_moved);
        EXPECT_EQ(small_obj, 10u);
        EXPECT_EQ(big_obj, 20u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
}

// 测试边界情况
template <class Any, class Object>
static void test_edge_cases() {
    // 测试默认构造
    Any value1;
    EXPECT_FALSE(value1.has_value());

    // 测试空指针赋值
    Any value2{Object{1u}};
    EXPECT_TRUE(value2.has_value());
    value2 = nullptr;
    EXPECT_FALSE(value2.has_value());

    // 测试连续赋值不同类型
    Any value3;
    value3 = Object1{1u};
    value3 = Object2{2u};
    value3 = Object3{3u};
    value3 = Object4{4u};

    bool_t throw_exception{False};
    try {
        auto obj = any_cast<Object4>(value3);
        EXPECT_EQ(obj, 4u);
    } catch (exception const&) {
        throw_exception = True;
    }
    EXPECT_EQ(throw_exception, False);
}

};  // namespace qlib

TEST(qlib, AnyObjectBasic) {
    qlib::test_basic<qlib::any_t, qlib::Object1>();
    qlib::test_basic<qlib::any_t, qlib::Object2>();
    qlib::test_basic<qlib::any_t, qlib::Object3>();
    qlib::test_basic<qlib::any_t, qlib::Object4>();
};

TEST(qlib, AnyMoveSemantics) {
    qlib::test_move_semantics<qlib::any_t, qlib::Object1>();
    qlib::test_move_semantics<qlib::any_t, qlib::Object2>();
    qlib::test_move_semantics<qlib::any_t, qlib::Object3>();
    qlib::test_move_semantics<qlib::any_t, qlib::Object4>();
}

TEST(qlib, AnyCopySemantics) {
    qlib::test_copy_semantics<qlib::any_t, qlib::Object1>();
    qlib::test_copy_semantics<qlib::any_t, qlib::Object2>();
    qlib::test_copy_semantics<qlib::any_t, qlib::Object3>();
    qlib::test_copy_semantics<qlib::any_t, qlib::Object4>();
}

TEST(qlib, AnyReset) {
    qlib::test_reset<qlib::any_t, qlib::Object1>();
    qlib::test_reset<qlib::any_t, qlib::Object2>();
    qlib::test_reset<qlib::any_t, qlib::Object3>();
    qlib::test_reset<qlib::any_t, qlib::Object4>();
}

TEST(qlib, AnyAssignment) {
    qlib::test_assignment<qlib::any_t, qlib::Object1>();
    qlib::test_assignment<qlib::any_t, qlib::Object2>();
    qlib::test_assignment<qlib::any_t, qlib::Object3>();
    qlib::test_assignment<qlib::any_t, qlib::Object4>();
}

TEST(qlib, AnyMultipleTypes) {
    qlib::test_multiple_types<qlib::any_t>();
}

TEST(qlib, AnySelfAssignment) {
    qlib::test_self_assignment<qlib::any_t, qlib::Object1>();
    qlib::test_self_assignment<qlib::any_t, qlib::Object2>();
    qlib::test_self_assignment<qlib::any_t, qlib::Object3>();
    qlib::test_self_assignment<qlib::any_t, qlib::Object4>();
}

TEST(qlib, AnyMixedOperations) {
    qlib::test_mixed_operations<qlib::any_t>();
}

TEST(qlib, AnyEdgeCases) {
    qlib::test_edge_cases<qlib::any_t, qlib::Object1>();
    qlib::test_edge_cases<qlib::any_t, qlib::Object2>();
    qlib::test_edge_cases<qlib::any_t, qlib::Object3>();
    qlib::test_edge_cases<qlib::any_t, qlib::Object4>();
}

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    } while (false);

    return result;
}

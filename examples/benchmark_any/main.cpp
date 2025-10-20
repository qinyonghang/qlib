// #include <chrono>
#include <benchmark/benchmark.h>
#include <any>
#include <charconv>
#include <cstring>
#include <iostream>

#include "qlib/any.h"

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
static void benchmark_creation(benchmark::State& state) {
    for (auto _ : state) {
        // 模拟实际创建any对象的场景
        Any value{Object{1u}};
        benchmark::DoNotOptimize(value);
    }
}

template <class Any, class Object>
static void benchmark_copy(benchmark::State& state) {
    Any original{Object{1u}};
    for (auto _ : state) {
        // 模拟复制any对象的场景
        Any copy{original};
        benchmark::DoNotOptimize(copy);
    }
}

template <class Any, class Object>
static void benchmark_move(benchmark::State& state) {
    for (auto _ : state) {
        // 模拟移动any对象的场景
        Any value{Object{1u}};
        Any moved = std::move(value);
        benchmark::DoNotOptimize(moved);
    }
}

template <class Any, class Object>
static void benchmark_assignment(benchmark::State& state) {
    Any value;
    Object obj{1u};
    for (auto _ : state) {
        // 模拟赋值操作的场景
        value = obj;
        benchmark::DoNotOptimize(value);
    }
}

template <class Any, class Object>
static void benchmark_cast(benchmark::State& state) {
    Any value{Object{1u}};
    for (auto _ : state) {
        // 模拟类型转换的场景
        auto& object = any_cast<Object&>(value);
        benchmark::DoNotOptimize(object);
    }
}

template <class Any, class Object>
static void benchmark_mixed_operations(benchmark::State& state) {
    for (auto _ : state) {
        // 模拟混合操作的场景
        Any value{Object{42u}};

        // 创建和使用引用
        auto& ref = any_cast<Object&>(value);
        benchmark::DoNotOptimize(ref);

        // 复制操作
        auto copy = value;
        benchmark::DoNotOptimize(copy);

        // 移动操作
        auto moved = std::move(copy);
        benchmark::DoNotOptimize(moved);

        // 重新赋值
        value = Object{99u};
        benchmark::DoNotOptimize(value);
    }
}

};  // namespace qlib

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        auto iterations = 1000u;

        if (argc > 1) {
            auto [ptr, ec] = std::from_chars(argv[1], argv[1] + ::strlen(argv[1]), iterations);
            if (ec != std::errc{}) {
                std::cout << argv[0] << " <iterations>" << std::endl;
                result = -1;
                break;
            }
        }

        benchmark::Initialize(&argc, argv);
        // benchmark::ReportUnrecognizedArguments(argc, argv);

        using namespace qlib;

        throw_if(sizeof(Object1) != 8u);
        throw_if(sizeof(Object2) != 16u);
        throw_if(sizeof(Object3) != 24u);
        throw_if(sizeof(Object4) != 32u);

        BENCHMARK(benchmark_creation<std::any, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_creation<any_t, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_creation<std::any, Object4>)->Iterations(iterations);
        BENCHMARK(benchmark_creation<any_t, Object4>)->Iterations(iterations);

        BENCHMARK(benchmark_copy<std::any, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_copy<any_t, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_copy<std::any, Object4>)->Iterations(iterations);
        BENCHMARK(benchmark_copy<any_t, Object4>)->Iterations(iterations);

        BENCHMARK(benchmark_move<std::any, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_move<any_t, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_move<std::any, Object4>)->Iterations(iterations);
        BENCHMARK(benchmark_move<any_t, Object4>)->Iterations(iterations);

        BENCHMARK(benchmark_assignment<std::any, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_assignment<any_t, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_assignment<std::any, Object4>)->Iterations(iterations);
        BENCHMARK(benchmark_assignment<any_t, Object4>)->Iterations(iterations);

        BENCHMARK(benchmark_cast<std::any, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_cast<any_t, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_cast<std::any, Object4>)->Iterations(iterations);
        BENCHMARK(benchmark_cast<any_t, Object4>)->Iterations(iterations);

        BENCHMARK(benchmark_mixed_operations<std::any, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_mixed_operations<any_t, Object1>)->Iterations(iterations);
        BENCHMARK(benchmark_mixed_operations<std::any, Object2>)->Iterations(iterations);
        BENCHMARK(benchmark_mixed_operations<any_t, Object2>)->Iterations(iterations);
        BENCHMARK(benchmark_mixed_operations<std::any, Object3>)->Iterations(iterations);
        BENCHMARK(benchmark_mixed_operations<any_t, Object3>)->Iterations(iterations);
        BENCHMARK(benchmark_mixed_operations<std::any, Object4>)->Iterations(iterations);
        BENCHMARK(benchmark_mixed_operations<any_t, Object4>)->Iterations(iterations);

        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
    } while (false);

    return result;
}

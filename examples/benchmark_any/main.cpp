// #include <chrono>
#include <benchmark/benchmark.h>
#include <any>
#include <charconv>
#include <cstring>
#include <iostream>

#include "qlib/any.h"

namespace qlib {

class SmallObject : public object {
protected:
    size_t _impl{0u};
    static inline size_t _counter{0u};

public:
    INLINE SmallObject() noexcept { ++_counter; }

    INLINE SmallObject(SmallObject const& __o) noexcept : _impl(__o._impl) { ++_counter; }

    INLINE SmallObject(SmallObject&& __o) noexcept : _impl(std::move(__o._impl)) { ++_counter; }

    INLINE SmallObject(size_t __impl) noexcept : _impl(__impl) { ++_counter; }

    INLINE ~SmallObject() noexcept { --_counter; }

    INLINE SmallObject& operator=(SmallObject const& __o) noexcept {
        if (this != &__o) {
            _impl = __o._impl;
        }
        return *this;
    }

    INLINE SmallObject& operator=(SmallObject&& __o) noexcept {
        _impl = std::move(__o._impl);
        __o._impl = 0u;
        return *this;
    }

    NODISCARD FORCE_INLINE bool_t operator==(SmallObject const& __o) const {
        return _impl == __o._impl;
    }

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
    size_t _reserve[1u];
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
FORCE_INLINE CONSTEXPR void benchmark(Any& value) {
    {
        auto object = any_cast<Object>(value);
        benchmark::DoNotOptimize(object);
    }
    {
        auto& object = any_cast<Object&>(value);
        benchmark::DoNotOptimize(object);
    }
    {
        auto&& object = any_cast<Object&&>(std::move(value));
        benchmark::DoNotOptimize(object);
    }
    {
        auto other = value;
        benchmark::DoNotOptimize(other);
    }
    {
        auto other = qlib::move(value);
        benchmark::DoNotOptimize(other);
    }
    {
        value = Object{};
        benchmark::DoNotOptimize(value);
    }
}

};  // namespace qlib

static auto benchmark_stl_any_small(benchmark::State& state) {
    using Any = std::any;
    using Object = qlib::SmallObject;
    qlib::throw_if(sizeof(Any) != 16u);
    for (auto _ : state) {
        Any value{Object{1u}};
        qlib::benchmark<Any, Object>(value);
        benchmark::DoNotOptimize(value);
    }
}

static auto benchmark_any_small(benchmark::State& state) {
    using Any = qlib::any_t;
    using Object = qlib::SmallObject;
    qlib::throw_if(sizeof(Any) != 16u);
    for (auto _ : state) {
        Any value{Object{1u}};
        qlib::benchmark<Any, Object>(value);
        benchmark::DoNotOptimize(value);
    }
}

static auto benchmark_stl_any_big(benchmark::State& state) {
    using Any = std::any;
    using Object = qlib::BigObject;
    qlib::throw_if(sizeof(Any) != 16u);
    for (auto _ : state) {
        Any value{Object{1u}};
        qlib::benchmark<Any, Object>(value);
        benchmark::DoNotOptimize(value);
    }
}

static auto benchmark_any_big(benchmark::State& state) {
    using Any = qlib::any_t;
    using Object = qlib::BigObject;
    qlib::throw_if(sizeof(Any) != 16u);
    for (auto _ : state) {
        Any value{Object{1u}};
        qlib::benchmark<Any, Object>(value);
        benchmark::DoNotOptimize(value);
    }
}

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        auto iterations = 1000u;

        if (argc > 1) {
            auto [ptr, ec] = std::from_chars(argv[1], argv[1] + ::strlen(argv[1]), iterations);
            if (ec != std::errc{}) {
                result = -1;
                break;
            }
        }

        benchmark::Initialize(&argc, argv);
        // benchmark::ReportUnrecognizedArguments(argc, argv);

        BENCHMARK(benchmark_stl_any_small)->Iterations(iterations);
        BENCHMARK(benchmark_any_small)->Iterations(iterations);
        BENCHMARK(benchmark_stl_any_big)->Iterations(iterations);
        BENCHMARK(benchmark_any_big)->Iterations(iterations);

        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
    } while (false);

    return result;
}

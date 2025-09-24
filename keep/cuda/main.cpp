#include <chrono>
#include <iostream>
#include <vector>
#include <charconv>
#include <string>
#include <benchmark/benchmark.h>
#include <cmath>

// 声明 CUDA 核函数
void vectorAdd(int* h_a, int* h_b, int* h_result, int n);

// int main(int argc, char** argv) {
//     if (argc < 2) {
//         std::cout << "Usage: " << argv[0] << " <n>" << std::endl;
//         return 0;
//     }

//     std::string_view _n{argv[1]};

//     int n{0u};
//     auto [ptr, ec] = std::from_chars(_n.data(), _n.data() + _n.size(), n);
//         if (ec != std::errc{} || ptr == nullptr || *ptr != '\0') {
//             std::cout << "Value({}) is Invalid! Value is " << argv[1] << ", Error Code is " << static_cast<int>(ec) << std::endl;
//             return -1;
//         }

//     // constexpr auto n = 10000000;
//     std::vector<int> a(n);
//     std::vector<int> b(n);
//     std::vector<int> result(n);

//     vectorAdd(a.data(), b.data(), result.data(), n);

//     auto start = std::chrono::high_resolution_clock::now();
//     // 调用 CUDA 函数
//     vectorAdd(a.data(), b.data(), result.data(), n);
//     auto end = std::chrono::high_resolution_clock::now();
//     std::cout << "Time taken: "
//               << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns"
//               << std::endl;

//     std::cout << "N: " << n << std::endl;

//     return 0;
// }

// Benchmark 函数
static void BM_VectorAdd(benchmark::State& state) {
    int n = state.range(0);
    std::vector<int> a(n, 1);
    std::vector<int> b(n, 2);
    std::vector<int> result(n);

    for (auto _ : state) {
        vectorAdd(a.data(), b.data(), result.data(), n);
    }
}

// 注册测试用例
BENCHMARK(BM_VectorAdd)->Arg(10000)->Arg(100000)->Arg(100000)->Arg(1000000)->Arg(1000000)->Iterations(100);

// 启动 Benchmark
BENCHMARK_MAIN();

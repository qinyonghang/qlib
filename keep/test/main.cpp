#include <benchmark/benchmark.h>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <vector>

// constexpr auto feature_w = 512;
// constexpr auto feature_h = 512;
// constexpr auto class_count = 81;
// constexpr auto anchor_size = 3;

struct box {
    uint32_t type;
    float* data;
    float score;
};

auto process(float* data,
             float threshold,
             uint32_t feature_h,
             uint32_t feature_w,
             std::vector<uint32_t> const& anchors,
             uint32_t class_size) {
    const auto anchor_size = anchors.size() / 2u;
    const auto feature_step = feature_h * feature_w;
    const auto anchor_step = feature_step * (4 + class_size);

    std::vector<box> boxes;
    for (auto c = 0u; c < anchor_size; ++c) {
        auto data_p = data + c * anchor_step;
        for (auto i = 0u; i < feature_h; ++i) {
            auto row_data_p = data_p + i * feature_w;
            for (auto j = 0u; j < feature_w; ++j) {
                auto cur_data_p = row_data_p + j;
                auto cur_score_p = cur_data_p + 4u * feature_step;
                uint32_t type = 0u;
                auto score = 0.f;
                for (auto k = 0u; k < class_size; ++k) {
                    auto kscore_p = cur_score_p + k * feature_step;
                    if (*kscore_p > threshold && *kscore_p > score) {
                        type = k;
                        score = *kscore_p;
                    }
                }
                if (score > threshold) {
                    boxes.emplace_back(box{type, cur_data_p, score});
                }
            }
        }
    }

    return boxes;
}

auto process_optimized(float* data,
                       float threshold,
                       uint32_t feature_h,
                       uint32_t feature_w,
                       std::vector<uint32_t> const& anchors,
                       uint32_t class_size) {
    const auto anchor_size = anchors.size() / 2u;
    const auto feature_step = feature_h * feature_w;
    const auto anchor_step = feature_step * (4 + class_size);

    std::vector<box> boxes;
    boxes.reserve(feature_step); // 避免扩容
    for (auto c = 0u; c < anchor_size; ++c) {
        auto data_p = data + c * anchor_step;
        for (auto i = 0u; i < feature_step; ++i) {
            auto cur_data_p = data_p + i;
            auto cur_score_p = cur_data_p + 4u * feature_step;
            uint32_t type = 0u;
            auto score = 0.f;
            for (auto k = 0u; k < class_size; ++k) {
                auto kscore_p = cur_score_p + k * feature_step;
                if (*kscore_p > threshold && *kscore_p > score) {
                    type = k;
                    score = *kscore_p;
                }
            }
            if (score > threshold) {
                boxes.emplace_back(box{type, cur_data_p, score});
            }
        }
    }

    return boxes;
}

auto process_optimized2(float* data,
                        float threshold,
                        uint32_t feature_h,
                        uint32_t feature_w,
                        std::vector<uint32_t> const& anchors,
                        uint32_t class_size) {
    const auto anchor_size = anchors.size() / 2u;
    const auto feature_step = feature_h * feature_w;
    const auto anchor_step = feature_step * (4 + class_size);

    std::vector<box> boxes;
    boxes.reserve(feature_step);
    for (auto c = 0u; c < anchor_size; ++c) {
        auto data_p = data + c * anchor_step;
        for (auto i = 0u; i < feature_step; ++i) {
            auto cur_data_p = data_p + i;
            auto cur_score_p = cur_data_p + 4u * feature_step;
            uint32_t type = 0u;
            auto score = 0.f;
            for (auto k = 0u; k < class_size; ++k) {
                auto kscore_p = cur_score_p + k * feature_step;
                if (__glibc_unlikely(*kscore_p > threshold && *kscore_p > score)) { // 指导流水线取指
                    type = k;
                    score = *kscore_p;
                }
            }
            if (__glibc_unlikely(score > threshold)) {
                boxes.emplace_back(box{type, cur_data_p, score});
            }
        }
    }

    return boxes;
}

// auto process_optimized(float* data,
//                        float threshold,
//                        uint32_t feature_h,
//                        uint32_t feature_w,
//                        std::vector<uint32_t> const& anchors,
//                        uint32_t class_size) {
//     const auto anchor_size = anchors.size() / 2u;
//     const auto feature_step = feature_h * feature_w;
//     const auto anchor_step = feature_step * (4 + class_size);

//     std::vector<std::tuple<uint32_t, uint32_t, float, float*>> scores;
//     scores.reserve(feature_step);
//     for (auto k = 0u; k < class_size; ++k) {
//         for (auto c = 0u; c < anchor_size; ++c) {
//             auto data_p = data + c * anchor_step;
//             for (auto i = 0u; i < feature_step; ++i) {
//                 auto cur_data_p = data_p + i;
//                 auto kscore_p = cur_data_p + 4u * feature_step + k * feature_step;
//                 if (__glibc_unlikely((kscore_p[i] > threshold))) {
//                     scores.emplace_back(std::make_tuple(i, k, kscore_p[i], cur_data_p));
//                 }
//             }
//         }
//     }
//     // std::cout << "Total " << scores.size() << " candidates" << std::endl;

//     struct Visited {
//         bool ok{false};
//         uint32_t type{0u};
//         float score{0.f};
//         float* data;
//     };

//     std::vector<Visited> visited(feature_step, {false, 0u, 0.f, nullptr});
//     for (auto const& [i, k, score, data_p] : scores) {
//         if ((!visited[i].ok) || (visited[i].score < score)) {
//             visited[i] = {true, k, score, data_p};
//         }
//     }

//     std::vector<box> boxes;
//     boxes.reserve(visited.size());
//     for (auto const& [ok, k, score, data_p] : visited) {
//         if (ok) {
//             // std::cout << "box: " << k << " score: " << score << std::endl;
//             boxes.emplace_back(box{k, data_p, score});
//         }
//     }

//     return boxes;
// }

void test() {
    // 测试参数
    constexpr uint32_t feature_h = 64;
    constexpr uint32_t feature_w = 240;
    constexpr uint32_t class_size = 1;

    const std::vector<uint32_t> anchors{12, 4, 26, 4, 22, 8, 47, 8};
    const auto total_data_size = feature_h * feature_w * (4 + class_size) * anchors.size() / 2;

    std::vector<float> data(total_data_size);
    std::default_random_engine engine(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::generate(data.begin(), data.end(), [&]() { return dist(engine); });

    std::vector<float> scores;
    const auto anchor_size = anchors.size() / 2u;
    const auto feature_step = feature_h * feature_w;
    const auto anchor_step = feature_step * (4 + class_size);
    const uint32_t k = 0;  // class_size = 1，所以只考虑 k = 0
    for (auto c = 0u; c < anchor_size; ++c) {
        auto data_p = data.data() + c * anchor_step;
        for (auto i = 0u; i < feature_h; ++i) {
            auto row_data_p = data_p + i * feature_w;
            for (auto j = 0u; j < feature_w; ++j) {
                auto cur_data_p = row_data_p + j;
                auto cur_score_p = cur_data_p + 4u * feature_step;
                auto kscore_p = cur_score_p + k * feature_step;
                scores.emplace_back(*kscore_p);
            }
        }
    }

    std::sort(scores.begin(), scores.end());
    auto threshold = scores[scores.size() * 0.99];

    std::cout << "Total: " << scores.size() << std::endl;
    std::cout << "Threshold: " << threshold << std::endl;

    // 执行测试
    std::cout << "Running test..." << std::endl;

    // 测试 process 函数
    std::vector<box> boxes1;
    {
        auto start = std::chrono::high_resolution_clock::now();
        boxes1 = process(data.data(), threshold, feature_h, feature_w, anchors, class_size);
        auto end = std::chrono::high_resolution_clock::now();
        auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "process 函数耗时: " << cost_ms << " ms" << std::endl;
    }
    std::cout << "process 函数返回的 box 数量: " << boxes1.size() << std::endl;

    // 测试 process_optimized 函数
    std::vector<box> boxes2;
    {
        auto start = std::chrono::high_resolution_clock::now();
        boxes2 =
            process_optimized(data.data(), threshold, feature_h, feature_w, anchors, class_size);
        auto end = std::chrono::high_resolution_clock::now();
        auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "process_optimized 函数耗时: " << cost_ms << " ms" << std::endl;
    }
    std::cout << "process_optimized 函数返回的 box 数量: " << boxes2.size() << std::endl;

    // 测试 process 函数
    // std::vector<box> boxes1;
    {
        auto start = std::chrono::high_resolution_clock::now();
        boxes1 = process(data.data(), threshold, feature_h, feature_w, anchors, class_size);
        auto end = std::chrono::high_resolution_clock::now();
        auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "process 函数耗时: " << cost_ms << " ms" << std::endl;
    }
    std::cout << "process 函数返回的 box 数量: " << boxes1.size() << std::endl;

    // 测试 process_optimized 函数
    // std::vector<box> boxes2;
    {
        auto start = std::chrono::high_resolution_clock::now();
        boxes2 =
            process_optimized(data.data(), threshold, feature_h, feature_w, anchors, class_size);
        auto end = std::chrono::high_resolution_clock::now();
        auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "process_optimized 函数耗时: " << cost_ms << " ms" << std::endl;
    }
    std::cout << "process_optimized 函数返回的 box 数量: " << boxes2.size() << std::endl;
}

// 测试参数
constexpr uint32_t g_feature_h = 64;
constexpr uint32_t g_feature_w = 240;
constexpr uint32_t g_class_size = 1;

const std::vector<uint32_t> g_anchors{12, 4, 26, 4, 22, 8, 47, 8};
std::vector<float> g_data(g_feature_h* g_feature_w * (4 + g_class_size) * g_anchors.size() / 2);

float g_threshold = 0.f;

auto init =
    []() {
        std::default_random_engine engine(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::generate(g_data.begin(), g_data.end(), [&]() { return dist(engine); });

        std::vector<float> scores;
        const auto anchor_size = g_anchors.size() / 2u;
        const auto feature_step = g_feature_h * g_feature_w;
        const auto anchor_step = feature_step * (4 + g_class_size);
        const uint32_t k = 0;  // class_size = 1，所以只考虑 k = 0
        for (auto c = 0u; c < anchor_size; ++c) {
            auto data_p = g_data.data() + c * anchor_step;
            for (auto i = 0u; i < g_feature_h; ++i) {
                auto row_data_p = data_p + i * g_feature_w;
                for (auto j = 0u; j < g_feature_w; ++j) {
                    auto cur_data_p = row_data_p + j;
                    auto cur_score_p = cur_data_p + 4u * feature_step;
                    auto kscore_p = cur_score_p + k * feature_step;
                    scores.emplace_back(*kscore_p);
                }
            }
        }

        std::sort(scores.begin(), scores.end());
        auto threshold = scores[scores.size() * 0.99];

        std::cout << "Threshold: " << threshold << std::endl;

        return true;
    }(

    );

// Benchmark 函数
static void BM_process(benchmark::State& state) {
    for (auto _ : state) {
        auto boxes1 =
            process(g_data.data(), g_threshold, g_feature_h, g_feature_w, g_anchors, g_class_size);
        // std::cout << boxes1.size() << std::endl;
    }
}

static void BM_process_optimized(benchmark::State& state) {
    for (auto _ : state) {
        auto boxes2 = process_optimized(g_data.data(), g_threshold, g_feature_h, g_feature_w,
                                        g_anchors, g_class_size);
        // std::cout << boxes2.size() << std::endl;
    }
}

static void BM_process_optimized2(benchmark::State& state) {
    for (auto _ : state) {
        auto boxes2 = process_optimized2(g_data.data(), g_threshold, g_feature_h, g_feature_w,
                                         g_anchors, g_class_size);
    }
}

// 注册测试用例
BENCHMARK(BM_process_optimized)->Iterations(100);
BENCHMARK(BM_process)->Iterations(100);
BENCHMARK(BM_process_optimized)->Iterations(100);
BENCHMARK(BM_process_optimized2)->Iterations(100);

// 启动 Benchmark
BENCHMARK_MAIN();

// int main() {
//     test();
//     return 0;
// }

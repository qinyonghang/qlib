#pragma once

#include <vector>

#include "QObject.h"

namespace qlib {

template <typename, typename = void>
struct has_c_str : std::false_type {};

template <typename T>
struct has_c_str<T, std::void_t<decltype(std::declval<T>().c_str())>> : std::true_type {};

template <typename T>
constexpr bool has_c_str_v = has_c_str<T>::value;

template <typename, typename = void>
struct has_string : std::false_type {};

template <typename T>
struct has_string<T, std::void_t<decltype(std::declval<T>().string())>> : std::true_type {};

template <typename T>
constexpr bool has_string_v = has_string<T>::value;

template <typename String>
constexpr bool is_string_v = (has_c_str_v<String> || std::is_convertible_v<String, char const*>);

template <typename T>
inline bool exists(T&& _path) {
    std::filesystem::path path{_path};
    return std::filesystem::exists(path);
}

template <typename T>
inline T align(T value, size_t alignment) {
    return (((value) + ((alignment)-1)) & ~((alignment)-1));
}

template <typename T>
inline T align_32(T value) {
    return align(value, 32);
}

template <typename T = float64_t>
T sigmoid(T x) {
    return static_cast<T>(1.0) / (static_cast<T>(1.0) + exp(-x));
}

template <typename T = float64_t>
std::tuple<std::vector<T>, std::vector<size_t>> topk(std::vector<T> const& vec, size_t k) {
    std::vector<int> indices(vec.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::partial_sort(indices.begin(), indices.begin() + k, indices.end(),
                      [&vec](int a, int b) { return vec[a] > vec[b]; });

    std::vector<T> topk_values(k);
    std::vector<size_t> topk_indices(k);
    for (auto i = 0u; i < k; ++i) {
        topk_values[i] = vec[indices[i]];
        topk_indices[i] = indices[i];
    }
    return {topk_values, topk_indices};
}

template <typename T>
struct QTraits;

template <typename QDerived>
class QAlgorithm : public QObject {
public:
    virtual typename QTraits<QDerived>::return_type operator()() = 0;
};

};  // namespace qlib

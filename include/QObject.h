#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#ifdef WITH_NEON
#include <arm_neon.h>
#endif

namespace qlib {

using string = std::string;
using byte = std::uint8_t;
using int8_t = std::int8_t;
using uint8_t = std::uint8_t;
using int16_t = std::int16_t;
using uint16_t = std::uint16_t;
using int32_t = std::int32_t;
using uint32_t = std::uint32_t;
using int64_t = std::int64_t;
using uint64_t = std::uint64_t;
using float32_t = float;
using float64_t = double;

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

template <typename T>
inline auto serialize(T const* value, size_t size) {
    std::stringstream out{};

    out << "[";
    for (auto i = 0u; i < size; ++i) {
        out << value[i];
        if (i != size - 1) {
            out << ",";
        }
    }
    out << "]";

    return out.str();
}

template <typename QVec>
auto serialize(QVec const& arr) {
    return serialize(arr.data(), arr.size());
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

class QObject {
public:
    using Self = QObject;
    using Ptr = std::shared_ptr<Self>;

    QObject(QObject* parent = nullptr) : __parent{parent} {}
    virtual ~QObject() = 0;

    void set_parent(QObject* parent) { __parent = parent; }

protected:
    QObject* __parent{nullptr};
};

using QObjectPtr = std::shared_ptr<QObject>;

template <typename T>
struct QTraits;

template <typename QDerived>
class QAlgorithm : public QObject {
public:
    virtual typename QTraits<QDerived>::return_type operator()() = 0;
};

};  // namespace qlib

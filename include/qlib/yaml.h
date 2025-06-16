#pragma once

#define YAML_IMPLEMENTATION

#include "yaml-cpp/yaml.h"

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {

namespace yaml {

using node = YAML::Node;

class loader : public object {
public:
    static node make(string_t const& path) { return YAML::LoadFile(path); }

#ifdef _GLIBCXX_FILESYSTEM
    static node make(std::filesystem::path const& path) { return make(path.string()); }
#endif

    template <class T>
    static T get(node const& node) {
        return node.template as<T>();
    }

    template <class T>
    static T get(node const& node, std::string_view key) {
        return get<T>(node[key]);
    }

    template <class T>
    static T get(node const& node, std::string_view key, T const& default_value) {
        auto value_node = node[key];
        return value_node ? get<T>(value_node) : default_value;
    }

    template <class T>
    static void get(T* value_ptr, node const& node, std::string_view key) {
        *value_ptr = get<T>(node, key);
    }

    template <class T>
    static void get(T* value_ptr, node const& node, std::string_view key, T const& default_value) {
        *value_ptr = get<T>(node, key, default_value);
    }
};

template <>
inline position loader::get<position>(node const& node) {
    auto value = get<std::array<float64_t, 3u>>(node);
    return position{value[0], value[1], value[2]};
}

#ifdef PSDK_IMPLEMENTATION
template <>
psdk::init_parameter loader::get<psdk::init_parameter>(node const& node);
#endif

#ifdef _GLIBCXX_FILESYSTEM
template <>
inline std::filesystem::path loader::get<std::filesystem::path>(node const& node) {
    return get<std::string>(node);
}
#endif

#ifdef LOGGER_IMPLEMENTATION
template <>
inline logger::level loader::get<logger::level>(node const& node) {
    return static_cast<logger::level>(get<size_t>(node));
}
#endif

};  // namespace yaml

};  // namespace qlib

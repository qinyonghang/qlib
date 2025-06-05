#pragma once

#define YAML_IMPLEMENTATION

#include "yaml-cpp/yaml.h"

#include "qlib/exception.h"
#include "qlib/object.h"

namespace YAML {
#ifdef _GLIBCXX_FILESYSTEM
template <>
struct convert<std::filesystem::path> {
    static bool decode(Node const& node, std::filesystem::path& rhs) {
        std::string value;
        auto result = convert<std::string>::decode(node, value);
        if (result) {
            rhs = value;
        }
        return result;
    }
};
#endif
}  // namespace YAML

#ifdef SPDLOG_ACTIVE_LEVEL
template <>
struct YAML::convert<spdlog::level::level_enum> {
    static bool decode(YAML::Node const& node, spdlog::level::level_enum& rhs) {
        size_t value;
        auto result = YAML::convert<size_t>::decode(node, value);
        if (result) {
            if (value >= spdlog::level::trace && value < spdlog::level::n_levels) {
                rhs = static_cast<spdlog::level::level_enum>(value);
            } else {
                result = false;
            }
        }
        return result;
    }
};
#endif

#ifdef OBJECT_IMPLEMENTATION
template <>
struct YAML::convert<qlib::position> {
    static bool decode(YAML::Node const& node, qlib::position& rhs) {
        bool result{true};

        do {
            if (!node.IsSequence()) {
                result = false;
                break;
            }

            if (node.size() == 2) {
                rhs.longitude = node[0].template as<decltype(rhs.longitude)>();
                rhs.latitude = node[1].template as<decltype(rhs.latitude)>();
                rhs.relative_height = 0;
            } else if (node.size() == 3) {
                rhs.longitude = node[0].template as<decltype(rhs.longitude)>();
                rhs.latitude = node[1].template as<decltype(rhs.latitude)>();
                rhs.relative_height = node[2].template as<decltype(rhs.relative_height)>();
            } else {
                result = false;
                break;
            }
        } while (false);

        return result;
    }
};
#endif

// #ifdef _GLIBCXX_TUPLE
// namespace YAML {
// template <std::size_t I, class... Ts>
// std::enable_if_t<I == sizeof...(Ts), bool> tuple_decode_helper(const YAML::Node& node,
//                                                                std::tuple<Ts...>& t) {
//     return true;
// }

// template <std::size_t I, class... Ts>
//     std::enable_if_t <
//     I<sizeof...(Ts), bool> tuple_decode_helper(const YAML::Node& node, std::tuple<Ts...>& t) {
//     bool result{true};

//     do {
//         std::get<I>(t) = node[I].as<typename std::tuple_element_t<I, std::tuple<Ts...>>>();
//         result = tuple_decode_helper<I + 1>(node, t);
//     } while (false);

//     return result;
// }

// template <class... Args>
// struct convert<std::tuple<Args...>> {
//     static bool decode(const Node& node, std::tuple<Args...>& rhs) {
//         bool result{true};

//         do {
//             if (!node.IsSequence() || node.size() != sizeof...(Args)) {
//                 result = false;
//                 break;
//             }

//             result = tuple_decode_helper(node, rhs);
//         } while (false);

//         return result;
//     }
// };

// };  // namespace YAML
// #endif

namespace qlib {

namespace yaml {

using node = YAML::Node;

template <class Value>
void set(Value* value, YAML::Node const& node, std::string_view key) {
    auto value_node = node[key];
    if (value_node) {
        *value = value_node.template as<Value>();
    }
}

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

#ifdef PSDK_IMPLEMENTATION
template <>
psdk::init_parameter loader::get<psdk::init_parameter>(node const& node);
#endif

};  // namespace yaml

};  // namespace qlib

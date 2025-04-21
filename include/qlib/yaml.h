#pragma once

#define YAML_IMPLEMENTATION

#include <filesystem>

#include "yaml-cpp/yaml.h"

namespace YAML {
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

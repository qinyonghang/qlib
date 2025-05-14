#pragma once

#include "object.h"

namespace qlib {
template <class T>
class config : public qlib::object<config<T>> {
protected:
    config() = default;

public:
    using base = qlib::object<config<T>>;
    using self = config<T>;
    using ptr = std::shared_ptr<self>;

#ifdef YAML_IMPLEMENTATION
    template <class Value>
    static void set(Value* value, YAML::Node const& node, std::string_view key) {
        auto value_node = node[key];
        if (value_node) {
            *value = value_node.template as<Value>();
        }
    }
#endif

    T& derived() { return static_cast<T&>(*this); }

    T const& derived() const { return static_cast<T const&>(*this); }

    int32_t init(std::filesystem::path const& path) {
        int32_t result{0};

        do {
            if (!std::filesystem::exists(path)) {
                result = qlib::FILE_NOT_FOUND;
                break;
            }

            if (path.extension() == ".yaml") {
#ifdef YAML_IMPLEMENTATION
                result = init(YAML::LoadFile(path));
#else
                result = qlib::FILE_NOT_SUPPORT;
#endif
                if (0 != result) {
                    break;
                }
            } else {
                result = qlib::FILE_NOT_SUPPORT;
                break;
            }
        } while (false);

        return result;
    }

#ifdef YAML_IMPLEMENTATION
    int32_t init(YAML::Node const& node) {
        int32_t result{0};

        do {
            try {
                load(node);
            } catch (YAML::Exception const& e) {
                result = qlib::YAML_PARSE_ERROR;
                break;
            }
        } while (false);

        return result;
    }
#endif

    template <
        class Source,
        class = std::enable_if_t<!std::is_same_v<std::decay_t<Source>, std::filesystem::path>>>
    void load(Source&& source) {
        derived().load(source);
    }
};

};  // namespace qlib
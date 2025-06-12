#pragma once

#define ARGPARSE_IMPLEMENTATION

#include <any>
#include <charconv>
#include <limits>
#include <sstream>
#include <vector>

// #include "argparse/argparse.hpp"
#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {
namespace argparse {

template <class T, class Enable = void>
struct convert;

template <>
struct convert<string_t> : public object {
    static string_t call(std::string_view s) { return string_t{s}; }
};

#ifdef _GLIBCXX_FILESYSTEM
template <>
struct convert<std::filesystem::path> : public object {
    static std::filesystem::path call(std::string_view s) { return std::filesystem::path{s}; }
};
#endif

template <>
struct convert<bool_t> : public object {
    static bool_t call(std::string_view s) {
        bool_t result{False};

        if (s == "true" || s == "True") {
            result = True;
        } else if (s == "false" || s == "False") {
            result = False;
        } else {
            THROW_EXCEPTION(False, "value({}) is invalid bool!", s.data());
        }

        return result;
    }
};

template <class T>
class convert<T,
              std::enable_if_t<std::disjunction_v<std::is_same<T, int8_t>,
                                                  std::is_same<T, int16_t>,
                                                  std::is_same<T, int32_t>,
                                                  std::is_same<T, int64_t>,
                                                  std::is_same<T, uint8_t>,
                                                  std::is_same<T, uint16_t>,
                                                  std::is_same<T, uint32_t>,
                                                  std::is_same<T, uint64_t>>>> : public object {
public:
    static T call(std::string_view s) {
        T result;
        auto res = std::from_chars(s.data(), s.data() + s.size(), result);
        THROW_EXCEPTION(res.ec == std::errc{}, "value({}) is invalid {}!", s.data(),
                        typeid(T).name());
        return result;
    }
};

// template <class T, class = std::enable_if_t<std::is_same_v<std::decay_t<T>, int8_t, int16_t, int32_t, int64_t>>>
// T convert<T>::call(std::string_view value) {

// }

class argument final : public object {
public:
    template <class String>
    argument(String&& name = "default") : _name{std::forward<String&&>(name)} {}

    template <class String, class T, class String2>
    argument(String name, T&& default_value, String2&& help = "")
            : _name{std::forward<String&&>(name)}, _default_value{std::forward<T&&>(default_value)},
              _help{std::forward<String2&&>(help)} {}

    void parse(std::string_view value) { _value = value; }

    auto name() const noexcept { return _name; }

    template <class... Args, class = std::enable_if_t<sizeof...(Args)>>
    argument& name(Args&&... args) {
        _name = decltype(_name)(std::forward<Args>(args)...);
        return *this;
    }

    auto help() const noexcept { return _help; }

    template <class... Args, class = std::enable_if_t<sizeof...(Args)>>
    argument& help(Args&&... args) {
        _help = decltype(_help)(std::forward<Args>(args)...);
        return *this;
    }

    template <typename T>
    T get() const {
        std::any any;
        if (_value.empty()) {
            THROW_EXCEPTION(_default_value.has_value(), "Argument({}) is not set.", _name);
            any = _default_value;
        } else {
            any = convert<T>::call(_value);
        }
        return std::any_cast<T>(_default_value);
    }

    template <class... Args>
    argument& default_value(Args&&... args) {
        _default_value = decltype(_default_value)(std::forward<Args>(args)...);
        return *this;
    }

    static bool_t is_positional(std::string_view name) noexcept {
        return std::memcmp(name.data(), "--", 2) != 0;
    }

protected:
    string_t _name;
    string_t _value;
    std::any _default_value;
    string_t _help;
};

class parser final : public object {
public:
    using self = parser;
    using ptr = sptr<self>;
    using base = object;

    template <class String>
    parser(String name) noexcept : _name{std::forward<String>(name)} {}

    template <class... Args>
    argument& add_argument(Args&&... args) noexcept {
        self::arguments.emplace_back(std::forward<Args>(args)...);
        return self::arguments.back();
    }

    template <typename T>
    T get(std::string_view name) {
        for (auto& it : self::arguments) {
            if (it.name() == name || it.name() == fmt::format("--{}", name)) {
                return it.get<T>();
            }
        }

        THROW_EXCEPTION(false, "Argument({}) not found", name);
    }

    template <class It1, class It2>
    void parse_args(It1 const& begin, It2 const& end) {
        bool_t position{True};
        size_t i{0u};
        for (auto it = begin; it != end; ++it, ++i) {
            if (position) {
                if (argument::is_positional(*it) &&
                    argument::is_positional(self::arguments[i].name())) {
                    self::arguments[i].parse(*it);
                } else {
                    position = False;
                }
            }

            if (!position) {
                THROW_EXCEPTION(!argument::is_positional(*it), "Unknown Argument: {}", *it);
                for (auto& argument : arguments) {
                    if (argument.name() == *it) {
                        THROW_EXCEPTION(it + 1 != end, "Missing Value at Argument: {}", *it);
                        ++it;
                        argument.parse(*it);
                        break;
                    }
                }
            }
        }
    }

    string_t help() const noexcept {
        std::stringstream out;

        out << "usage: " << self::_name << " ";
        std::vector<argument const*> positional_arguments;
        std::vector<argument const*> optional_arguments;
        for (auto const& it : self::arguments) {
            if (argument::is_positional(it.name())) {
                positional_arguments.emplace_back(&it);
            } else {
                optional_arguments.emplace_back(&it);
            }
        }

        for (auto const& it : optional_arguments) {
            out << "[" << it->name() << "] ";
        }

        for (auto const& it : positional_arguments) {
            out << it->name() << " ";
        }
        out << std::endl;

        out << std::endl << "positional arguments:" << std::endl;
        for (auto const& it : positional_arguments) {
            out << "  " << it->name();
            if (!it->help().empty()) {
                out << "\t\t\t" << it->help();
            }
            out << std::endl;
        }
        out << std::endl << "optional arguments:" << std::endl;
        for (auto const& it : optional_arguments) {
            out << "  " << it->name();
            if (!it->help().empty()) {
                out << "\t\t\t" << it->help();
            }
            out << std::endl;
        }
        out << std::endl;

        return out.str();
    }

protected:
    string_t _name;
    std::vector<argument> arguments;
};
};  // namespace argparse
};  // namespace qlib

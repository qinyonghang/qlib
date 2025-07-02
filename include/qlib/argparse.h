#pragma once

#define ARGPARSE_IMPLEMENTATION

#include <any>
#include <charconv>
#include <vector>

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {
namespace argparse {

template <class T, class Enable = void>
struct convert : public object {
    static T call(std::string_view s) { return T{s}; }
};

template <>
struct convert<bool_t> : public object {
    static bool_t call(std::string_view s) {
        bool_t result{False};

        if (s == "true" || s == "True") {
            result = True;
        } else if (s == "false" || s == "False") {
            result = False;
        } else {
            THROW_EXCEPTION(False, "Value({}) is Invalid Bool!", s.data());
        }

        return result;
    }
};

template <class T>
struct convert<T,
               std::enable_if_t<is_one_of_v<T,
                                            int8_t,
                                            int16_t,
                                            int32_t,
                                            int64_t,
                                            ssize_t,
                                            uint8_t,
                                            uint16_t,
                                            uint32_t,
                                            uint64_t,
                                            size_t>>> : public object {
    static T call(std::string_view s) {
        T result;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result);
        THROW_EXCEPTION(ec == std::errc{} && ptr != nullptr && *ptr == '\0',
                        "Value({}) is Invalid {}! Err={}!", s.data(), typeid(T).name(),
                        static_cast<int32_t>(ec));
        return result;
    }
};

class argument final : public object {
public:
    using base = object;
    using self = argument;
    using ptr = sptr<self>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    template <class String>
    argument(String&& name = "default") : _name{std::forward<String>(name)} {}

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
            any = _default_value;
        } else {
            any = convert<T>::call(_value);
        }
        return std::any_cast<T>(any);
    }

    template <class... Args>
    argument& default_value(Args&&... args) {
        _default_value = decltype(_default_value)(std::forward<Args>(args)...);
        return *this;
    }

    bool has_value() const noexcept { return _default_value.has_value() || (!_value.empty()); }

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

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    template <class String>
    parser(String name) noexcept : _name{std::forward<String>(name)} {}

    template <class... Args>
    argument& add_argument(std::string_view const& name, Args&&... args) noexcept {
        argument::ptr arg;

        if (starts_with(name, "--")) {
            arg = argument::make(name.data() + 2, std::forward<Args>(args)...);
            self::optional_arguments.emplace_back(arg);
        } else if (starts_with(name, "-")) {
            arg = argument::make(name.data() + 1, std::forward<Args>(args)...);
            self::optional_arguments.emplace_back(arg);
        } else {
            arg = argument::make(name, std::forward<Args>(args)...);
            self::positional_arguments.emplace_back(arg);
        }

        return *arg;
    }

    template <typename T>
    T get(std::string_view name) {
        string_t _name;
        if (starts_with(name, "--")) {
            _name = name.data() + 2;
        } else if (starts_with(name, "-")) {
            _name = name.data() + 1;
        } else {
            _name = name;
        }

        argument::ptr arg{nullptr};
        arg = find(self::positional_arguments, _name);
        if (arg == nullptr) {
            arg = find(self::optional_arguments, _name);
        }
        THROW_EXCEPTION(arg != nullptr, "Argument({}) not Found!", name);

        return arg->get<T>();
    }

    template <class It1, class It2>
    void parse_args(It1 const& begin, It2 const& end) {
        auto it = begin;
        for (auto& arg : self::positional_arguments) {
            THROW_EXCEPTION(it != end, "Argument({}) not Found!", arg->name());
            arg->parse(*it);
            ++it;
        }

        for (; it != end; ++it) {
            argument::ptr arg{nullptr};
            if (starts_with(*it, "--")) {
                arg = find(self::optional_arguments, *it + 2);
            } else if (starts_with(*it, "-")) {
                arg = find(self::optional_arguments, *it + 1);
            } else {
                THROW_EXCEPTION(False, "Unknown Argument: {}", *it);
            }
            THROW_EXCEPTION(arg != nullptr, "Unknown Argument: {}", *it);
            THROW_EXCEPTION(it + 1 != end, "Missing Value at Argument: {}", *it);
            arg->parse(*++it);
        }

        for (auto& arg : self::optional_arguments) {
            THROW_EXCEPTION(arg->has_value(), "Missing Value at Argument: {}", arg->name());
        }
    }

    string_t help() const noexcept;

protected:
    string_t _name;
    std::vector<argument::ptr> positional_arguments;
    std::vector<argument::ptr> optional_arguments;

    static bool_t starts_with(std::string_view s, std::string_view prefix) {
        return s.substr(0, prefix.size()) == prefix;
    }

    static bool_t ends_with(std::string_view s, std::string_view suffix) {
        return s.substr(s.size() - suffix.size()) == suffix;
    }

    static argument::ptr find(std::vector<argument::ptr> const& args, std::string_view name) {
        argument::ptr result{nullptr};
        for (auto& arg : args) {
            if (arg->name() == name) {
                result = arg;
                break;
            }
        }
        return result;
    }
};
};  // namespace argparse
};  // namespace qlib

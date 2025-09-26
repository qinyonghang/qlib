#pragma once

#include <map>
#include <regex>

#include "qlib/vector.h"

namespace qlib {

namespace nlu {

template <class Derived>
class Base : public object {
protected:
    uint32_t _impl{};

    std::map<uint32_t, string_view_t> const& _map_() const { return derived()._map_(); }

public:
    Base(string_view_t __value) {
        bool_t __find{False};
        for (auto const& [__key, __val] : _map_()) {
            if (__val == __value) {
                _impl = __key;
                __find = True;
                break;
            }
        }
        if (!__find) {
            std::cout << "do not find " << __value << std::endl;
        }
    }

    operator uint32_t() const noexcept { return _impl; }
    operator string_view_t() const noexcept { return _map_().at(_impl); }

    NODISCARD FORCE_INLINE CONSTEXPR Derived& derived() noexcept {
        return static_cast<Derived&>(*this);
    }

    NODISCARD FORCE_INLINE CONSTEXPR Derived const& derived() const noexcept {
        return static_cast<Derived const&>(*this);
    }
};

class Device final : public Base<Device> {
public:
    enum : uint32_t { unknown = 0u, light = 1u, air_conditioner = 2u, tv = 3u, curtain = 4u };

protected:
    auto const& _map_() const {
        static std::map<uint32_t, string_view_t> _map{{unknown, "unknown"},
                                                      {light, "light"},
                                                      {air_conditioner, "air_conditioner"},
                                                      {tv, "tv"},
                                                      {curtain, "curtain"}};
        return _map;
    }

    friend class Base<Device>;

public:
    using Base::Base;
};

template <class OutStream>
OutStream& operator<<(OutStream& out, Device const& value) {
    out << static_cast<string_view_t>(value);
    return out;
}

class Action final : public Base<Action> {
public:
    enum : uint32_t {
        unknown = 0u,
        turn_on = 1u,
        turn_off = 2u,
        set_brightness = 3u,
        adjust_brightness_up = 4u,
        adjust_brightness_down = 5u,
        set_temperature = 6u,
        adjust_temperature_up = 7u,
        adjust_temperature_down = 8u,
        set_mode = 9u,
        adjust_volume_up = 10u,
        adjust_volume_down = 11u,
        change_channel = 12u,
        mute = 13u,
        open = 14u,
        close = 15u,
        set_position = 16u
    };

protected:
    auto const& _map_() const {
        static std::map<uint32_t, string_view_t> _map{
            {unknown, "unknown"},
            {turn_on, "turn_on"},
            {turn_off, "turn_off"},
            {set_brightness, "set_brightness"},
            {adjust_brightness_up, "adjust_brightness_up"},
            {adjust_brightness_down, "adjust_brightness_down"},
            {set_temperature, "set_temperature"},
            {adjust_temperature_up, "adjust_temperature_up"},
            {adjust_temperature_down, "adjust_temperature_down"},
            {set_mode, "set_mode"},
            {adjust_volume_up, "adjust_volume_up"},
            {adjust_volume_down, "adjust_volume_down"},
            {change_channel, "change_channel"},
            {mute, "mute"},
            {open, "open"},
            {close, "close"},
            {set_position, "set_position"}};
        return _map;
    }

    friend class Base<Action>;

public:
    using Base::Base;
};

template <class OutStream>
OutStream& operator<<(OutStream& out, Action const& value) {
    out << static_cast<string_view_t>(value);
    return out;
}

class Location final : public Base<Location> {
public:
    enum : uint32_t { unknown = 0u, living_room = 1u, bedroom = 2u, kitchen = 3u, all = 4u };

protected:
    auto const& _map_() const {
        static std::map<uint32_t, string_view_t> _map{{unknown, "unknown"},
                                                      {living_room, "living_room"},
                                                      {bedroom, "bedroom"},
                                                      {kitchen, "kitchen"},
                                                      {all, "all"}};
        return _map;
    }

    friend class Base<Location>;

public:
    using Base::Base;
};

template <class OutStream>
OutStream& operator<<(OutStream& out, Location const& value) {
    out << static_cast<string_view_t>(value);
    return out;
}

class Pattern final : public object {
protected:
    string_view_t _impl;

public:
    template <class... Args>
    Pattern(Args&&... args) : _impl{forward<Args>(args)...} {}

    bool_t match(string_view_t text) const {
        try {
            std::regex pattern(std::string{_impl.begin(), _impl.end()});
            return std::regex_match(std::string{text.begin(), text.end()}, pattern);
        } catch (...) {
            return False;
        }
    }
};

template <class Name,
          class Pattern,
          class Device,
          class Action,
          class Location,
          class Allocator = new_allocator_t>
class parser : public object {
public:
    using base = object;
    using self = parser;
    using allocator_type = Allocator;
    using name_type = Name;
    using pattern_type = Pattern;
    using device_type = Device;
    using action_type = Action;
    using location_type = Location;

    struct result {
        location_type location;
        device_type device;
        action_type action;
    };

    using result_type = result;
    using results_type = vector_t<result_type, allocator_type>;

    struct rule {
        name_type name;
        pattern_type pattern;
        results_type results;
    };

    using rule_type = rule;
    using rules_type = vector_t<rule_type, allocator_type>;

protected:
    rules_type _rules;

    NODISCARD FORCE_INLINE CONSTEXPR allocator_type& _allocator_() noexcept {
        using refence = typename allocator_type::reference;
        return static_cast<refence&>(*this);
    }

    NODISCARD FORCE_INLINE CONSTEXPR allocator_type& _allocator_() const noexcept {
        using refence = typename allocator_type::refence;
        return static_cast<refence&>(const_cast<self&>(this));
    }

public:
    template <class... Args>
    FORCE_INLINE CONSTEXPR parser(Args&&... args) : _rules(forward<Args>(args)...) {}

    template <class String>
    results_type operator()(String const& input) const {
        results_type results(8u, _rules.allocator());

        for (auto& rule : _rules) {
            if (rule.pattern.match(input)) {
                for (auto& result : rule.results) {
                    results.emplace_back(result);
                }
            }
        }

        return results;
    }
};

    // auto _nlu_init_(yaml_view_t const& node) {
    //     // _json_text = string_t::from_file(config_path);
    //     // auto json_error = json::parse(&_config, _json_text.begin(), _json_text.end());
    //     // if (0 != json_error) {
    //     //     _logger->error("json::parse return {}!", json_error);
    //     //     result = -1;
    //     //     break;
    //     // }
    //     // _logger->trace("config: {}", _config.to());

    //     // NluParser::rules_type rules;
    //     // for (auto& rule : _config["rules"].array()) {
    //     //     NluParser::results_type results;
    //     //     for (auto& result : rule["results"].array()) {
    //     //         results.emplace_back(NluParser::result_type{
    //     //             .location = result["location"].get<string_view_t>(),
    //     //             .device = result["device"].get<string_view_t>(),
    //     //             .action = result["action"].get<string_view_t>(),
    //     //         });
    //     //     }
    //     //     rules.emplace_back(NluParser::rule_type{
    //     //         .name = rule["name"].get<string_view_t>(),
    //     //         .pattern = rule["pattern"].get<string_view_t>(),
    //     //         .results = move(results),
    //     //     });
    //     // }

    //     // _nlu_parser = make_unique<NluParser>(move(rules));

    //     // std::string input;
    //     // while (!_exit) {
    //     //     struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    //     //     if (poll(&pfd, 1, 100) > 0 && (pfd.revents & POLLIN)) {
    //     //         std::getline(std::cin, input);
    //     //         NluParser::results_type results;
    //     //         {
    //     //             profile _profile{"nlu parser"};
    //     //             results = _nlu_parser->operator()(
    //     //                 string_view_t{input.data(), input.data() + input.size()});
    //     //         }
    //     //         if (results.size() > 0) {
    //     //             for (auto& result : results) {
    //     //                 std::cout << "Result(" << result.device << ":" << result.location << ":"
    //     //                           << result.action << ")" << std::endl;
    //     //             }
    //     //         } else {
    //     //             std::cout << "No result" << std::endl;
    //     //         }
    //     //     }
    //     // }
    //     // return 0;
    //     return True;
    // }

};  // namespace nlu

};  // namespace qlib

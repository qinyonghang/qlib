#pragma once

#include <condition_variable>
#include <fstream>
#include <map>
#include <mutex>
#include <regex>
#include <string>

#include "qlib/any.h"
#include "qlib/data.h"
#include "qlib/json.hpp"
#include "qlib/string.h"
#include "qlib/vector.h"

#include "logger.hpp"

namespace qlib {

template <class _DataManager>
class nlu : public object {
public:
    using base = object;
    using self = nlu;
    using publisher_type = data::publisher<_DataManager>;
    using subscriber_type = data::subscriber<_DataManager>;

    enum : int32_t {
        ok = 0,
        unknown = -1,
        already_inited = -2,
        not_inited = -3,
        rules_load_error = -4,
    };

protected:
    struct command {
        string_view_t location;
        string_view_t device;
        string_view_t action;
    };

    struct rule {
        string_view_t name;
        string_view_t pattern;
        vector_t<command> commands;
    };

    logger& _logger;
    bool_t _init{False};
    bool_t _exit{False};
    std::mutex _mutex;
    std::condition_variable _condition;
    unique_ptr_t<subscriber_type> _subscriber_text;
    unique_ptr_t<publisher_type> _publisher_command;
    std::shared_ptr<string_t> _text;
    string_t _rules_text;
    vector_t<rule> _rules;

    string_t _file_(string_view_t __file) const {
        string_t __f{__file.begin(), __file.end()};
        if (!std::filesystem::exists(__f.c_str())) {
            char buf[1024u];
            auto len = readlink("/proc/self/exe", buf, sizeof(buf));
            if (len != -1) {
                buf[len] = '\0';
                __f = (std::filesystem::path(buf).parent_path() / __f.c_str()).string();
            }
        }
        return __f;
    }

    template <class _Yaml>
    auto _init_(_Yaml const& node, _DataManager& manager) {
        int32_t result{0};

        do {
            if (_init) {
                result = already_inited;
                _logger.error("nlu: already initialized!");
                break;
            }

            _subscriber_text = qlib::make_unique<subscriber_type>(
                node["in_text_topic"].template get<string_view_t>(),
                [this](any_t const& data) {
                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        _text = any_cast<std::shared_ptr<string_t>>(data);
                    }
                    _condition.notify_one();
                    auto text = any_cast<std::shared_ptr<string_t>>(data);
                    _logger.debug("nlu: received text({})!", *text);
                },
                manager);

            _publisher_command = qlib::make_unique<publisher_type>(
                node["out_command_topic"].template get<string_view_t>(), manager);

            auto rules_path = _file_(node["rules"].template get<string_view_t>());
            if (!std::filesystem::exists(rules_path.c_str())) {
                _logger.error("nlu: rules file not found!");
                result = rules_load_error;
                break;
            }
            _rules_text = string::from_file(rules_path);
            json_view_t json;
            result = json::parse(&json, _rules_text.begin(), _rules_text.end());
            if (0 != result) {
                _logger.error("nlu: json::parse return {}!", result);
                result = rules_load_error;
                break;
            }
            _logger.debug("nlu::rules: {}", string::from_json(json, _rules_text.size()));

            _rules.reserve(64u);
            for (auto& r : json["rules"].array()) {
                vector_t<command> commands;
                for (auto& c : r["commands"].array()) {
                    commands.emplace_back(command{
                        .location = c["location"].get<string_view_t>(),
                        .device = c["device"].get<string_view_t>(),
                        .action = c["action"].get<string_view_t>(),
                    });
                }
                _rules.emplace_back(rule{
                    .name = r["name"].get<string_view_t>(),
                    .pattern = r["pattern"].get<string_view_t>(),
                    .commands = move(commands),
                });
            }

            _init = True;
            _logger.trace("nlu: init!");
        } while (false);

        return result;
    }

    auto _exec_() {
        int32_t result{0};

        do {
            if (!_init) {
                result = not_inited;
                _logger.error("nlu: not initialized!");
                break;
            }

            while (!_exit) {
                std::shared_ptr<string_t> text;

                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _condition.wait(lock);
                    if (_exit) {
                        break;
                    }
                    text = _text;
                }

                _logger.debug("nlu: parse enter:");
                _logger.debug("nlu: text: {}", *text);

                for (auto& r : _rules) {
                    std::regex pattern(std::string{r.pattern.begin(), r.pattern.end()});
                    std::smatch match;
                    std::string input_str{text->begin(), text->end()};

                    if (std::regex_search(input_str, match, pattern)) {
                        for (auto const& command : r.commands) {
                            _publisher_command->publish(std::make_shared<json_t>(json_t::object({
                                {"device", command.device},
                                {"location", command.location},
                                {"action", command.action},
                            })));
                            _logger.info("nlu: command: device:{}, location:{}, action:{}",
                                         command.device, command.location, command.action);
                        }
                    }
                }

                _logger.debug("nlu: parse exit!");
            }
        } while (false);

        return result;
    }

    void _exit_() {
        _exit = True;
        _condition.notify_one();
    }

public:
    nlu() = delete;
    nlu(self const&) = delete;
    nlu(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    nlu(logger& logger) : _logger{logger} {}

    template <class... _Args>
    nlu(_Args&&... args) {
        int32_t result{init(forward<_Args>(args)...)};
        throw_if(0 != result, "nlu::nlu() exception!");
    }

    ~nlu() {
        _exit_();
        _logger.trace("nlu: deinit!");
    }

    template <class... _Args>
    auto init(_Args&&... args) {
        return _init_(forward<_Args>(args)...);
    }

    template <class... _Args>
    auto exec(_Args&&... args) {
        return _exec_(forward<_Args>(args)...);
    }

    void exit() { _exit_(); }
};  // namespace nlu

};  // namespace qlib

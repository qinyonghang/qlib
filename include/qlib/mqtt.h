#pragma once

#include <functional>

#include "mqtt/async_client.h"

#include "qlib/logger.h"
#include "qlib/object.h"

namespace qlib {

using namespace ::mqtt;

namespace mqtt {

class action_listener : public ::mqtt::iaction_listener {
protected:
    static inline std::map<::mqtt::token::Type, qlib::string> ops{
        {::mqtt::token::Type::CONNECT, "CONNECT"},
        {::mqtt::token::Type::DISCONNECT, "DISCONNECT"},
        {::mqtt::token::Type::SUBSCRIBE, "SUBSCRIBE"},
        {::mqtt::token::Type::UNSUBSCRIBE, "UNSUBSCRIBE"},
        {::mqtt::token::Type::PUBLISH, "PUBLISH"}};

    static auto to_string(::mqtt::string_collection::const_ptr_t const& topics_ptr) {
        std::stringstream topics;
        if (topics_ptr != nullptr) {
            for (auto i = 0u; i < topics_ptr->size(); ++i) {
                topics << topics_ptr->operator[](i);
                if (i != topics_ptr->size() - 1) {
                    topics << ",";
                }
            }
        }
        return topics.str();
    }

public:
    using ptr = std::shared_ptr<iaction_listener>;
    void on_failure(::mqtt::token const& token) override {
        auto it = ops.find(token.get_type());
        if (it != ops.end()) {
            qWarn("Action({}) failed! Topics={}", it->second, to_string(token.get_topics()));
        } else {
            qWarn("Action({}) failed! Topics={}", static_cast<uint32_t>(token.get_type()),
                  to_string(token.get_topics()));
        }
    }

    void on_success(::mqtt::token const& token) override {
        auto it = ops.find(token.get_type());
        if (it != ops.end()) {
            qInfo("Action({}) success! Topics={}", it->second, to_string(token.get_topics()));
        } else {
            qInfo("Action({}) success! Topics={}", static_cast<uint32_t>(token.get_type()),
                  to_string(token.get_topics()));
        }
    }
};

class callback : public ::mqtt::callback {
protected:
    std::function<void(string const&, string const&)> cb{nullptr};

public:
    using self = callback;
    using ptr = std::shared_ptr<self>;

    callback() = default;

    template <class Callback>
    callback(Callback&& _cb) : cb{std::forward<Callback>(_cb)} {}

    void connected(string const& cause) override { qInfo("Connected! Cause={}", cause); }

    void connection_lost(string const& cause) override {
        qError("Connection lost! Cause={}", cause);
    }

    void message_arrived(::mqtt::const_message_ptr msg) override {
        if (cb != nullptr) {
            cb(msg->get_topic(), msg->get_payload_str());
        }
    }

    void delivery_complete(::mqtt::delivery_token_ptr token) override {}
};

};  // namespace mqtt

};  // namespace qlib

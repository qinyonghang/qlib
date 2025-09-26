#include <filesystem>
#include <vector>

#include "qlib/application.h"
#include "qlib/argparse.h"
#include "qlib/dds.h"
#include "qlib/logger.h"

namespace qlib {
class Application : public application {
public:
    using base = application;
    using self = Application;
    using ptr = sptr<self>;

    constexpr static inline auto module_name = "ArgParseTest";

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    template <class... Args>
    Application(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        if (0 != result) {
            std::exit(result);
        }
    }

    ~Application() noexcept {}

    int32_t init(int32_t argc, char** argv) {
        int32_t result{0};

        do {
            argparse::parser parser{module_name};
            parser.add_argument("type").help("type: publish, subscribe or both");
            parser.add_argument("--publish").default_value<string_t>("topic");
            parser.add_argument("--subscribe").default_value<string_t>("topic");
            if (!base::parse_args(&parser, argc, argv)) {
                result = -1;
                break;
            }

            auto type = parser.get<string_t>("type");
            auto subscribe = parser.get<string_t>("subscribe");
            auto publish = parser.get<string_t>("publish");

            qInfo("type: {}, subscribe : {}, publish: {}", type, subscribe, publish);

            self::type = type;
            self::subscribe = subscribe;
            self::publish = publish;
        } while (false);

        return result;
    }

    int32_t exec() {
        if (type == "publish") {
            auto publisher_ptr = qlib::dds::publisher::make<std::vector<uint8_t>>(publish);
            for (auto i = 0u; !base::exit; ++i) {
                std::vector<uint8_t> message;
                for (auto j = 0u; j < i; ++j) {
                    message.emplace_back(j);
                }
                auto result = publisher_ptr->publish(message);
                if (0 != result) {
                    qError("Application: Publish failed! Message={}", message);
                    break;
                }
                qInfo("Application: Publish Success! Message={}", message);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else if (type == "subscribe") {
            auto subscriber_ptr = qlib::dds::subscriber::make<std::vector<uint8_t>>(
                subscribe, [](std::vector<uint8_t>&& message) {
                    qInfo("Application: Subscribe success! Message={}", message);
                });
            while (!self::exit) {
                qInfo("Application: Running...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            auto publisher_ptr = qlib::dds::publisher::make<std::vector<uint8_t>>(publish);
            auto subscriber_ptr = qlib::dds::subscriber::make<std::vector<uint8_t>>(
                subscribe, [](std::vector<uint8_t>&& message) {
                    qInfo("Application: Subscribe success! Message={}", message);
                });
            for (auto i = 0u; !self::exit; ++i) {
                std::vector<uint8_t> message;
                for (auto j = 0u; j < i; ++j) {
                    message.emplace_back(j);
                }
                auto result = publisher_ptr->publish(message);
                if (0 != result) {
                    qError("Application: Publish failed! Message={}", message);
                    break;
                }
                qInfo("Application: Publish Success! Message={}", message);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        return 0;
    }

protected:
    string_t type;
    string_t subscribe;
    string_t publish;
};
};  // namespace qlib

int32_t main(int32_t argc, char** argv) {
    auto ptr = qlib::Application::make(argc, argv);
    return ptr->exec();
}

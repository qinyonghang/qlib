#include <signal.h>

#include <atomic>
#include <thread>

#include "qlib/argparse.h"
#include "qlib/dds.h"
#include "qlib/logger.h"

namespace {
std::atomic_bool __exit{false};
}

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        argparse::ArgumentParser program("dds_test");

        program.add_argument("type").help("type: publish, subscribe or both");
        program.add_argument("--sub_topic").default_value<std::string>("");
        program.add_argument("--pub_topic").default_value<std::string>("");

        try {
            program.parse_args(argc, argv);
        } catch (std::exception const& err) {
            std::cout << err.what() << std::endl;
            std::cout << program;
            result = -1;
            break;
        }

        signal(SIGINT, +[](int32_t) { __exit = true; });

        auto type = program.get<std::string>("type");
        if (type == "publish") {
            auto publisher_ptr = std::make_shared<qlib::dds::publisher<std::string>>("topic");
            std::string message;
            for (auto i = 0u; !__exit; ++i) {
                message = fmt::format("{}{}", message, i);
                result = publisher_ptr->publish(message);
                if (0 != result) {
                    qError("publish failed! message=[{}:{}]", message.size(), message);
                    break;
                }
                qInfo("publish success! message=[{}:{}]", message.size(), message);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else if (type == "subscribe") {
            auto subscriber_ptr = std::make_shared<qlib::dds::subscriber<std::string>>("topic");
            subscriber_ptr->subscribe([](std::string const& message) {
                qInfo("subscribe success! message=[{}:{}]", message.size(), message);
            });
            while (!__exit) {
                qInfo("running...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            auto sub_topic = program.get<std::string>("sub_topic");
            auto pub_topic = program.get<std::string>("pub_topic");
            auto publisher_ptr = std::make_shared<qlib::dds::publisher<std::string>>(pub_topic);
            auto subscriber_ptr = std::make_shared<qlib::dds::subscriber<std::string>>(sub_topic);
            subscriber_ptr->subscribe([](std::string const& message) {
                qInfo("subscribe success! message=[{}:{}]", message.size(), message);
            });
            std::string message;
            for (auto i = 0u; !__exit; ++i) {
                message = fmt::format("{}{}", message, i);
                result = publisher_ptr->publish(message);
                if (0 != result) {
                    qError("publish failed! message=[{}:{}]", message.size(), message);
                    break;
                }
                qInfo("publish success! message=[{}:{}]", message.size(), message);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } while (false);

    return result;
}
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
        program.add_argument("--sub").default_value<std::string>("topic");
        program.add_argument("--pub").default_value<std::string>("topic");

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
        auto sub_topic = program.get<std::string>("sub");
        auto pub_topic = program.get<std::string>("pub");
        if (type == "publish") {
            auto publisher_ptr = qlib::dds::publisher::make<qlib::dds::string>(pub_topic);
            for (auto i = 0u; !__exit; ++i) {
                auto message = fmt::format("{}", i);
                result = publisher_ptr->publish(message);
                if (0 != result) {
                    qError("publish failed! message={}", message);
                    break;
                }
                qInfo("publish success! message={}", message);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else if (type == "subscribe") {
            auto subscriber_ptr = qlib::dds::subscriber::make<qlib::dds::string>(
                sub_topic, [](qlib::dds::string::ptr const& message_ptr) {
                    qInfo("subscribe success! message=[{}:{}]", *message_ptr);
                });
            while (!__exit) {
                qInfo("running...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            auto publisher_ptr = qlib::dds::publisher::make<qlib::dds::string>(pub_topic);
            auto subscriber_ptr = qlib::dds::subscriber::make<qlib::dds::string>(
                sub_topic, [](qlib::dds::string::ptr const& message_ptr) {
                    qInfo("subscribe success! message=[{}:{}]", *message_ptr);
                });
            for (auto i = 0u; !__exit; ++i) {
                auto message = fmt::format("{}", i);
                result = publisher_ptr->publish(message);
                if (0 != result) {
                    qError("publish failed! message={}", message);
                    break;
                }
                qInfo("publish success! message={}", message);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } while (false);

    return result;
}
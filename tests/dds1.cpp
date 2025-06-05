#include <signal.h>

#include <atomic>
#include <thread>

#include "qlib/argparse.h"
#include "qlib/dds.h"
#include "qlib/logger.h"

namespace qlib {
class Application : public object {
public:
    using base = object;
    using self = Application;
    using ptr = std::shared_ptr<self>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    template <class... Args>
    Application(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(int32_t argc, char* argv[]) {
        int32_t result{0};

        do {
            signal(SIGINT, +[](int32_t) { self::exit = true; });

            argparse::ArgumentParser program(self::name);

            program.add_argument("topic").default_value<std::string>("topic");

            try {
                program.parse_args(argc, argv);
            } catch (std::exception const& err) {
                std::cout << err.what() << std::endl;
                std::cout << program;
                result = -1;
                break;
            }

            auto topic = program.get<std::string>("topic");

            subscriber_ptr = dds::subscriber::make<dds::sequence<uint32_t>>(
                topic, [](dds::sequence<uint32_t>::ptr const& values_ptr) {
                    auto values = static_cast<std::vector<uint32_t>>(*values_ptr);
                    qInfo("Receive message: {}", values);
                });

        } while (false);

        return result;
    }

    int32_t exec() {
        while (!self::exit) {
            qTrace("Running...");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return 0;
    }

protected:
    constexpr static inline auto name = "dds";
    static inline std::atomic_bool exit{false};

    dds::subscriber::ptr subscriber_ptr;
};
};  // namespace qlib

int32_t main(int32_t argc, char* argv[]) {
    auto app = qlib::Application::make(argc, argv);
    return app->exec();
}

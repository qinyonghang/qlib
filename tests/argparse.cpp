#include <filesystem>

#include "qlib/application.h"
#include "qlib/argparse.h"
#include "qlib/logger.h"

namespace qlib {
class Application : public application {
public:
    using base = application;
    using self = Application;
    using ptr = sptr<self>;

    constexpr static inline auto module_name = "ArgParser";

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
            parser.add_argument("path").help("Path");
            parser.add_argument("--verbose").help("Verbose");
            parser.add_argument("--debug").default_value(True).help("Debug");
            parser.add_argument("--log_level")
                .default_value(logger::level::trace)
                .help("Log level");
            if (!base::parse_args(&parser, argc, argv)) {
                result = -1;
                break;
            }

            auto path = parser.get<std::filesystem::path>("path");
            auto verbose = parser.get<bool_t>("verbose");
            auto debug = parser.get<bool_t>("--debug");
            auto log_level = parser.get<logger::level>("log_level");

            qInfo("Path: {}", path);
            qInfo("Verbose: {}", verbose);
            qInfo("Debug: {}", debug);
            qInfo("Log level: {}", log_level);
        } while (false);

        return result;
    }

    int32_t exec() { return 0; }
};
};  // namespace qlib

int32_t main(int32_t argc, char** argv) {
    auto ptr = qlib::Application::make(argc, argv);
    return ptr->exec();
}

// wget https://github.com/k2-fsa/sherpa-onnx/releases/download/kws-models/sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01-mobile.tar.bz2
// tar xvf sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01-mobile.tar.bz2
// rm sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01-mobile.tar.bz2

// #include <poll.h>
#include <signal.h>

#include <any>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

#include "qlib/argparse.h"
#include "qlib/json.h"

#include "audio.hpp"
#include "kws.hpp"
#include "recognizer.hpp"

namespace qlib {

class Application final : public object {
public:
    using base = object;
    using self = Application;
    using data_manager_type =
        data::manager<std::string, vector_t<std::function<void(any_t const&)>>>;
    // string_t _json_text;
    // json_view_t _config;
    // using Name = string_view_t;
    // using NluParser = nlu::parser<Name, nlu::Pattern, nlu::Device, nlu::Action, nlu::Location>;
    // unique_ptr_t<NluParser> _nlu_parser;

protected:
    string_t _yaml_text{};
    yaml_view_t _config{};
    data_manager_type _data_manager{};
    logger _logger{};
    recognizer<data_manager_type> _recognizer{_logger};
    kws<data_manager_type> _kws{_logger};
    audio::reader<data_manager_type> _audio_reader{_logger};

    static inline std::condition_variable _condition{};
    constexpr static inline string_view_t _name{"smart-home"};

public:
    Application(int32_t argc, char* argv[]) {
        try {
            auto result = init(argc, argv);
            if (0 != result) {
                std::exit(0);
            }
        } catch (exception const& e) {
            std::cout << e.what() << std::endl;
            std::exit(0);
        }
    }

    int32_t init(int32_t argc, char* argv[]) {
        int32_t result{0};

        do {
            argparse::parser<> parser{_name};

            parser.add_argument("config").help("config file path");

            try {
                parser.parse_args(argv + 1, argv + argc);
            } catch (...) {
                std::cout << parser.help() << std::endl;
                result = -1;
                break;
            }

            signal(SIGINT, +[](int signal) { _condition.notify_one(); });

            auto config_path = parser.get<string_t>("config");
            std::cout << _name << ": " << config_path << std::endl;

            _yaml_text = string_t::from_file(config_path);
            result = yaml::parse(&_config, _yaml_text.begin(), _yaml_text.end());
            if (0 != result) {
                std::cout << "yaml::parse return " << result << "!" << std::endl;
                break;
            }

            _logger.init(_config["logger"]);
            {
                string_t __text{_yaml_text.size()};
                yaml_view_t::to(__text, _config);
                _logger.trace("Application: config: {}", __text);
            }

            result = _recognizer.init(_config["recognizer"], _data_manager);
            if (0 != result) {
                _logger.error("recognizer::init return {}!", result);
                break;
            }

            result = _kws.init(_config["kws"], _data_manager);
            if (0 != result) {
                _logger.error("kws::init return {}!", result);
                break;
            }

            result = _audio_reader.init(_config["audio"]["reader"], _data_manager);
            if (0 != result) {
                _logger.error("audio::reader::init return {}!", result);
                break;
            }
        } while (false);

        return result;
    }

    int32_t exec() {
        int32_t result{0};

        do {
            auto __kws_future = std::async(std::launch::async, [this]() { return _kws.exec(); });
            auto __recognizer_future =
                std::async(std::launch::async, [this]() { return _recognizer.exec(); });
            _audio_reader.start();
            {
                std::mutex __mutex;
                std::unique_lock<std::mutex> __lock(__mutex);
                _condition.wait(__lock);
            }
            _logger.info("Application: received exit signal!");
            _kws.exit();
            _recognizer.exit();
            result = __kws_future.get();
            if (0 != result) {
                _logger.error("kws::exec return {}!", result);
            }
            result = __recognizer_future.get();
            if (0 != result) {
                _logger.error("recognizer::exec return {}!", result);
            }
        } while (false);

        return result;
    }
};
};  // namespace qlib

int32_t main(int32_t argc, char* argv[]) {
    qlib::Application app(argc, argv);
    return app.exec();
}

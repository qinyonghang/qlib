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
#include "nlu.hpp"
#include "recognizer.hpp"

namespace qlib {

class Application final : public object {
public:
    using base = object;
    using self = Application;
    using data_manager_type =
        data::manager<string_view_t, vector_t<std::function<void(any_t const&)>>>;

protected:
    string_t _yaml_text{};
    yaml_view_t _config{};
    data_manager_type _data_manager{};
    slogger _logger{};
    recognizer<data_manager_type> _recognizer{_logger};
    kws<data_manager_type> _kws{_logger};
    audio::reader<data_manager_type> _audio_reader{_logger};
    nlu<data_manager_type> _nlu{_logger};

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

            result = parser.parse_args(argc, argv);
            if (0 != result) {
                std::cout << parser.help() << std::endl;
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

            result = _nlu.init(_config["nlu"], _data_manager);
            if (0 != result) {
                _logger.error("Application: nlu::init return {}!", result);
                break;
            }

            result = _recognizer.init(_config["recognizer"], _data_manager);
            if (0 != result) {
                _logger.error("Application: recognizer::init return {}!", result);
                break;
            }

            result = _kws.init(_config["kws"], _data_manager);
            if (0 != result) {
                _logger.error("Application: kws::init return {}!", result);
                break;
            }

            result = _audio_reader.init(_config["audio"]["reader"], _data_manager);
            if (0 != result) {
                _logger.error("Application: audio::reader::init return {}!", result);
                break;
            }
        } while (false);

        return result;
    }

    int32_t exec() {
        int32_t result{0};

        do {
            auto nlu_future = std::async([this]() { return _nlu.exec(); });
            auto recognizer_future = std::async([this]() { return _recognizer.exec(); });
            auto kws_future = std::async([this]() { return _kws.exec(); });

            _audio_reader.start();

            {
                std::mutex mutex;
                std::unique_lock<std::mutex> lock(mutex);
                _condition.wait(lock);
            }
            _logger.info("Application: received exit signal!");

            _kws.exit();
            _recognizer.exit();
            _nlu.exit();

            result = kws_future.get();
            if (0 != result) {
                _logger.error("Application: kws::exec return {}!", result);
            }
            result = recognizer_future.get();
            if (0 != result) {
                _logger.error("Application: recognizer::exec return {}!", result);
            }
            result = nlu_future.get();
            if (0 != result) {
                _logger.error("Application: nlu::exec return {}!", result);
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

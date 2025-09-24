// wget https://github.com/k2-fsa/sherpa-onnx/releases/download/kws-models/sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01-mobile.tar.bz2
// tar xvf sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01-mobile.tar.bz2
// rm sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01-mobile.tar.bz2

// #include <poll.h>
#include <signal.h>

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

#include "spdlog/fmt/chrono.h"
#include "spdlog/sinks/basic_file_sink.h"
#ifdef _WIN32
#include "spdlog/sinks/wincolor_sink.h"
#else
#include "spdlog/sinks/ansicolor_sink.h"
#endif
#include "spdlog/spdlog.h"
// #include "yaml-cpp/yaml.h"

#include "qlib/any.h"
#include "qlib/argparse.h"
#include "qlib/json.h"

#include "audio.hpp"
#include "kws.hpp"
#include "nlu.h"
#include "profile.h"

template <class Char>
struct fmt::formatter<qlib::string::value<Char>, Char>
        : public fmt::formatter<fmt::basic_string_view<Char>, Char> {
    template <class FormatContext>
    auto format(qlib::string::value<Char> value, FormatContext& ctx) const {
        using Base = fmt::basic_string_view<Char>;
        return fmt::formatter<Base, Char>::format(Base{value.data(), value.size()}, ctx);
    }
};

template <class Char>
struct fmt::formatter<qlib::string::view<Char>, Char>
        : public fmt::formatter<fmt::basic_string_view<Char>, Char> {
    template <class FormatContext>
    auto format(qlib::string::view<Char> value, FormatContext& ctx) const {
        using Base = fmt::basic_string_view<Char>;
        return fmt::formatter<Base, Char>::format(Base{value.data(), value.size()}, ctx);
    }
};

namespace qlib {

template <class Sample>
class WavWriter final : public object {
public:
    using base = object;
    using self = WavWriter;
    using sample_type = Sample;

protected:
    std::ofstream _file;
    struct Header {
        // RIFF块
        uint8_t riff_id[4] = {'R', 'I', 'F', 'F'};
        uint32_t riff_size = 0;  // 总大小 = 36 + 数据大小（后续更新）
        uint8_t wave_id[4] = {'W', 'A', 'V', 'E'};

        // fmt块
        uint8_t fmt_id[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_size = 16u;                         // PCM格式固定为16字节
        uint16_t audio_format = 1;                       // 1 = PCM
        uint16_t num_channels = 0;                       // 声道数（1=单声道，2=立体声）
        uint32_t sample_rate = 0;                        // 采样率（如44100Hz）
        uint32_t byte_rate = 0;                          // 字节率 = 采样率 * 声道数 * 位深/8
        uint16_t block_align = 0;                        // 块对齐 = 声道数 * 位深/8
        uint16_t bits_per_sample = sizeof(Sample) * 8u;  // 位深（16位）

        // data块
        uint8_t data_id[4] = {'d', 'a', 't', 'a'};
        uint32_t data_size = 0;
    };
    Header _header{};

public:
    WavWriter(self const&) = delete;
    WavWriter(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    template <class String>
    WavWriter(String&& file_path, uint32_t sample_rate, size_t channels = 1) {
        _file.open(file_path, std::ios::binary | std::ios::trunc);
        throw_if(!_file.is_open());

        _header.num_channels = channels;
        _header.sample_rate = sample_rate;
        _header.byte_rate = sample_rate * channels * sizeof(Sample);
        _header.block_align = channels * sizeof(Sample);
        _header.riff_size = 36;
        _header.data_size = 0;

        _file.write(reinterpret_cast<const char*>(&_header), sizeof(Header));
    }

    ~WavWriter() noexcept {
        if (_file.is_open()) {
            _file.seekp(0);
            _file.write(reinterpret_cast<const char*>(&_header), sizeof(Header));
            _file.close();
        };
    }

    auto write(sample_type const* data, size_t sample_count) {
        size_t const byte_count = sample_count * sizeof(sample_type);
        _file.write(reinterpret_cast<const char*>(data), byte_count);
        _header.data_size += static_cast<uint32_t>(byte_count);
        _header.riff_size = 36 + _header.data_size;
    }
};

class Application final : public object {
public:
    using base = object;
    using self = Application;
    using data_manager_type = data::manager<std::string, std::function<void(any_t<> const&)>>;
    // string_t _json_text;
    // json_view_t _config;
    // using Name = string_view_t;
    // using NluParser = nlu::parser<Name, nlu::Pattern, nlu::Device, nlu::Action, nlu::Location>;
    // unique_ptr_t<NluParser> _nlu_parser;

protected:
    string_t _yaml_text{};
    yaml_view_t _config{};
    data_manager_type _data_manager{};
    audio::reader<data_manager_type> _audio_reader{};
    kws<data_manager_type> _kws{};

    static inline std::atomic<bool_t> _exit{False};
    static inline std::condition_variable _condition{};

    constexpr static inline string_view_t _name{"smart-home"};

    auto _log_init_(yaml_view_t const& node) {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.reserve(2);
        auto& console = node["console"];
        if (console["enable"] && console["enable"].get<bool_t>()) {
            auto level = console["level"].get<uint32_t>();
#ifdef _WIN32
            auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
            auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
            color_sink->set_level(spdlog::level::level_enum(level));
            sinks.emplace_back(color_sink);
        }
        auto& file = node["file"];
        if (file["enable"] && file["enable"].get<bool_t>()) {
            auto path = file["path"].get<std::string>();
            auto level = file["level"].get<uint32_t>();
            if (path == "auto") {
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                path = fmt::format("logs/{:%Y-%m-%d_%H-%M-%S}.log", fmt::localtime(time));
            }
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.c_str());
            file_sink->set_level(spdlog::level::level_enum(level));
            sinks.emplace_back(file_sink);
        }
        auto logger =
            std::make_shared<spdlog::logger>(std::string{_name}, sinks.begin(), sinks.end());
        auto level = node["level"].get<uint32_t>();
        logger->set_level(spdlog::level::level_enum(level));
        spdlog::set_default_logger(logger);
    }

    // auto _nlu_init_(yaml_view_t const& node) {
    //     // _json_text = string_t::from_file(config_path);
    //     // auto json_error = json::parse(&_config, _json_text.begin(), _json_text.end());
    //     // if (0 != json_error) {
    //     //     spdlog::error("json::parse return {}!", json_error);
    //     //     result = -1;
    //     //     break;
    //     // }
    //     // spdlog::trace("config: {}", _config.to());

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

public:
    Application(int32_t argc, char* argv[]) {
        auto result = init(argc, argv);
        if (0 != result) {
            std::exit(result);
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

            signal(
                SIGINT, +[](int signal) {
                    _exit = True;
                    _condition.notify_one();
                });

            auto config_path = parser.get<string_t>("config");
            std::cout << _name << ": " << config_path << std::endl;

            _yaml_text = string_t::from_file(config_path);
            result = yaml::parse(&_config, _yaml_text.begin(), _yaml_text.end());
            if (0 != result) {
                spdlog::error("yaml::parse return {}!", result);
                break;
            }

            _log_init_(_config["log"]);

            result = _audio_reader.init(_config["audio"]["reader"], _data_manager);
            if (0 != result) {
                spdlog::error("audio::reader::init return {}!", result);
                break;
            }

            result = _kws.init(_config["kws"], _data_manager);
            if (0 != result) {
                spdlog::error("kws::init return {}!", result);
                break;
            }
        } while (false);

        return result;
    }

    int32_t exec() {
        int32_t result{0};

        do {
            auto __kws_future = std::async(std::launch::async, [this]() { return _kws.exec(); });
            _audio_reader.start();
            std::mutex __mutex;
            std::unique_lock<std::mutex> __lock(__mutex);
            _condition.wait(__lock);
            __lock.unlock();
            _kws.exit();
            result = __kws_future.get();
            if (0 != result) {
                spdlog::error("kws::exec return {}!", result);
                break;
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

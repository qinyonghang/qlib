#ifndef RECOGNIZER_HPP
#define RECOGNIZER_HPP

#include <any>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "sherpa-onnx/c-api/cxx-api.h"

// #include "qlib/any.h"
#include "qlib/data.h"

#include "logger.hpp"

namespace qlib {

template <class _DataManager>
class recognizer : public object {
public:
    using base = object;
    using self = recognizer;
    using publisher_type = data::publisher<_DataManager>;
    using subscriber_type = data::subscriber<_DataManager>;
    using RecognizerConfig = sherpa_onnx::cxx::OfflineRecognizerConfig;
    using Recognizer = sherpa_onnx::cxx::OfflineRecognizer;
    using clock_type = std::chrono::steady_clock;
    using vad_config = sherpa_onnx::cxx::VadModelConfig;
    using vad_type = sherpa_onnx::cxx::VoiceActivityDetector;

    enum : int32_t {
        ok = 0,
        unknown = -1,
        not_init = -2,
        file_not_found = -3,
    };

protected:
    bool_t _init{False};
    std::atomic<bool_t> _exit{False};
    std::atomic<bool_t> _wakeup{False};
    std::mutex _mutex;
    std::condition_variable _cond;
    logger& _logger;
    uint32_t _in_sample_rate{0};
    uint32_t _in_max_size{0};
    uint32_t _in_window_size{512};
    uint32_t _in_sleep_time{1500};
    std::unique_ptr<subscriber_type> _subscriber_audio;
    std::unique_ptr<subscriber_type> _subscriber_wakeup;
    std::unique_ptr<publisher_type> _publisher;
    std::unique_ptr<vad_type> _vad;
    std::unique_ptr<Recognizer> _model;
    std::queue<any_t> _in_datas;
    std::condition_variable _cond_datas;

    template <class _Str>
    _Str _file_(_Str const& __file) const {
        _Str __f{__file};
        if (!std::filesystem::exists(__f)) {
            char buf[1024u];
            auto len = readlink("/proc/self/exe", buf, sizeof(buf));
            if (len != -1) {
                buf[len] = '\0';
                __f = (std::filesystem::path(buf).parent_path() / __f).string();
            }
        }
        return __f;
    }

    template <class _Yaml>
    auto _init_(_Yaml const& __yaml, _DataManager& __manager) {
        int32_t result{0};

        do {
            _subscriber_audio = std::make_unique<subscriber_type>(
                __yaml["in_audio_topic"].template get<std::string>(),
                [this](any_t const& __data) {
                    {
                        std::lock_guard<std::mutex> __lock(_mutex);
                        if (_in_datas.size() > _in_max_size) {
                            _in_datas.pop();
                        }
                        _in_datas.emplace(__data);
                    }
                    _cond_datas.notify_one();
                    auto __samples = any_cast<std::shared_ptr<vector_t<float32_t>>>(__data);
                    _logger.debug("recognizer: received {} samples!", __samples->size());
                },
                __manager);

            _subscriber_wakeup = std::make_unique<subscriber_type>(
                __yaml["in_wakeup_topic"].template get<std::string>(),
                [this](any_t const& __data) {
                    {
                        std::lock_guard<std::mutex> __lock(_mutex);
                        _wakeup = True;
                    }
                    _cond.notify_one();
                    auto __keyword = any_cast<std::shared_ptr<std::string>>(__data);
                    _logger.info("recognizer: wakeup! keyword: {}", *__keyword);
                },
                __manager);

            _publisher = std::make_unique<publisher_type>(
                __yaml["out_topic"].template get<std::string>(), __manager);

            _in_sample_rate = __yaml["in_sample_rate"].template get<uint32_t>();
            _in_max_size = __yaml["in_max_size"].template get<uint32_t>();
            _in_window_size = __yaml["in_window_size"].template get<uint32_t>();
            _in_sleep_time = __yaml["in_sleep_time"].template get<uint32_t>(1500);

            {
                vad_config __vad_config;
                auto& __vad_yaml = __yaml["vad"];
                auto __model_path = _file_(__vad_yaml["model"].template get<std::string>());
                if (!std::filesystem::exists(__model_path)) {
                    result = file_not_found;
                    _logger.error("recognizer: vad: model file is not exists! {}", __model_path);
                    break;
                }
                __vad_config.silero_vad.model = __model_path;
                __vad_config.silero_vad.threshold =
                    __vad_yaml["threshold"].template get<float32_t>(0.5);
                __vad_config.silero_vad.min_silence_duration =
                    __vad_yaml["min_silence_duration"].template get<float32_t>(0.1);
                __vad_config.silero_vad.min_speech_duration =
                    __vad_yaml["min_speech_duration"].template get<float32_t>(0.25);
                __vad_config.silero_vad.max_speech_duration =
                    __vad_yaml["max_speech_duration"].template get<float32_t>(8);
                __vad_config.sample_rate = __vad_yaml["sample_rate"].template get<int32_t>(16000u);
                __vad_config.num_threads = __vad_yaml["num_threads"].template get<int32_t>(1);
                __vad_config.provider = __vad_yaml["provider"].template get<std::string>("cpu");
                __vad_config.debug = __vad_yaml["debug"].template get<bool_t>(False);
                _vad = std::make_unique<vad_type>(std::move(vad_type::Create(__vad_config, 20)));
            }

            {
                RecognizerConfig __config;
                if (!__yaml["type"] || __yaml["type"].template get<string_view_t>() == "auto" ||
                    __yaml["type"].template get<string_view_t>() == "wenetspeech") {
                    auto __model_path = _file_(__yaml["model"].template get<std::string>());
                    if (!std::filesystem::exists(__model_path)) {
                        result = file_not_found;
                        _logger.error("recognizer: model file is not exists! {}", __model_path);
                        break;
                    }
                    __config.model_config.wenet_ctc.model = __model_path;
                    auto __tokens_path = _file_(__yaml["tokens"].template get<std::string>());
                    if (!std::filesystem::exists(__tokens_path)) {
                        result = file_not_found;
                        _logger.error("recognizer: tokens file is not exists! {}", __tokens_path);
                        break;
                    }
                    __config.model_config.tokens = __tokens_path;
                    __config.model_config.num_threads =
                        __yaml["num_threads"].template get<uint32_t>(1);
                    __config.model_config.provider =
                        __yaml["provider"].template get<std::string>("cpu");
                    __config.model_config.debug = __yaml["debug"].template get<bool_t>(False);
                } else {
                    result = unknown;
                    _logger.error("recognizer: invalid model type!");
                    break;
                }
                _model = std::make_unique<Recognizer>(std::move(Recognizer::Create(__config)));
            }

            _init = True;
            _logger.trace("recognizer: init!");
        } while (false);

        return result;
    }

    auto _exec_() {
        int32_t result{0};

        do {
            _logger.debug("recognizer: exec enter:");
            if (!_init) {
                result = not_init;
                _logger.error("recognizer: not init!");
                break;
            }

            std::vector<float32_t> total_samples;
            uint32_t offset{0u};
            bool_t has_speech{False};
            clock_type::time_point __last_time{clock_type::now()};
            while (!_exit) {
                std::shared_ptr<vector_t<float32_t>> __samples;
                {
                    std::unique_lock<std::mutex> __lock(_mutex);
                    while (!_exit && !_wakeup) {
                        _cond.wait(__lock);
                        __last_time = clock_type::now();
                    }
                    if (_exit) {
                        break;
                    }
                    if (_in_datas.empty()) {
                        _cond_datas.wait(__lock);
                        continue;
                    }
                    __samples = any_cast<std::shared_ptr<vector_t<float32_t>>>(_in_datas.front());
                    _in_datas.pop();
                }
                _logger.debug("recognizer: handle enter:");
                total_samples.insert(total_samples.end(), __samples->begin(), __samples->end());
                _logger.debug("recognizer: samples: {}", total_samples.size());
                for (; offset + _in_window_size <= total_samples.size();
                     offset += _in_window_size) {
                    _vad->AcceptWaveform(total_samples.data() + offset, _in_window_size);
                    if (!has_speech && _vad->IsDetected()) {
                        has_speech = True;
                        __last_time = clock_type::now();
                        _logger.info("recognizer: wakeup!");
                    }
                }

                if (total_samples.size() > 10 * _in_window_size) {
                    offset -= (total_samples.size() - 10 * _in_window_size);
                    total_samples = {total_samples.end() - 10 * _in_window_size,
                                     total_samples.end()};
                }

                if (!_vad->IsEmpty()) {
                    _logger.debug("recognizer: speech recognize enter:");
                    while (!_vad->IsEmpty()) {
                        auto __seg = _vad->Front();
                        _vad->Pop();
                        auto __stream = _model->CreateStream();
                        __stream.AcceptWaveform(_in_sample_rate, __seg.samples.data(),
                                                __seg.samples.size());
                        _model->Decode(&__stream);
                        auto __result = _model->GetResult(&__stream);
                        _publisher->publish(std::make_shared<std::string>(__result.text));
                        _logger.info("recognizer: sended {} to text!", __result.text);
                    }
                    _logger.debug("recognizer: speech recognize exit!");
                    has_speech = False;
                    total_samples.clear();
                    offset = 0;
                    __last_time = clock_type::now();
                }

                if (clock_type::now() - __last_time > std::chrono::milliseconds(_in_sleep_time)) {
                    _wakeup = False;
                    _logger.info("recognizer: sleep!");
                }

                _logger.debug("recognizer: handle exit!");
            }
            _logger.debug("recognizer: exec exit!");
        } while (false);

        return result;
    }

public:
    recognizer() = delete;
    recognizer(self const&) = delete;
    recognizer(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    recognizer(logger& __logger) : _logger(__logger) {}

    template <class _Yaml>
    recognizer(_Yaml const& __yaml, _DataManager& __manager, logger& __logger) : _logger(__logger) {
        auto result = _init_(__yaml, __manager);
        throw_if(result != 0, "recognizer: exception!");
    }

    ~recognizer() {
        exit();
        _logger.trace("recognizer: deinit!");
    }

    template <class _Yaml>
    auto init(_Yaml const& __yaml, _DataManager& __manager) {
        return _init_(__yaml, __manager);
    }

    template <class... _Args>
    auto exec(_Args&&... __args) {
        return _exec_(forward<_Args>(__args)...);
    }

    auto exit() {
        _exit = True;
        _cond.notify_one();
    }
};

};  // namespace qlib

#endif
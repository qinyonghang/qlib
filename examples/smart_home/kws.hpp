#ifndef KWS_HPP
#define KWS_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

#include "sherpa-onnx/c-api/cxx-api.h"
#include "spdlog/spdlog.h"
// #include "yaml-cpp/yaml.h"

#include "qlib/any.h"
#include "qlib/data.h"
#include "qlib/yaml.h"

namespace qlib {

template <class _DataManager>
class kws final : public object {
public:
    using self = kws;
    using subscriber_type = data::subscriber<_DataManager>;
    using publisher_type = data::publisher<_DataManager>;
    using KeywordSpotterConfig = sherpa_onnx::cxx::KeywordSpotterConfig;
    using KeywordSpotter = sherpa_onnx::cxx::KeywordSpotter;

protected:
    bool_t _exit{False};
    bool_t _init{False};
    uint32_t _sample_rate{16000};
    unique_ptr_t<publisher_type> _publisher_wakeup;
    unique_ptr_t<subscriber_type> _subscriber_audio;
    unique_ptr_t<KeywordSpotter> _keyword_spotter{};
    std::mutex _mutex;
    std::condition_variable _condition;
    std::queue<any_t<>> _audio_datas;

    auto _init_(yaml_view_t const& node, _DataManager& data_manager) {
        int32_t result{0};

        do {
            _publisher_wakeup = qlib::make_unique<publisher_type>(
                node["publisher"].get<std::string>(), data_manager);

            auto& subscriber_node = node["subscriber"];
            _subscriber_audio = qlib::make_unique<subscriber_type>(
                subscriber_node["audio"].get<std::string>(),
                [this](any_t<> const& data) {
                    spdlog::debug("kws received audio data from audio::reader!");
                    {
                        std::lock_guard<std::mutex> lock{_mutex};
                        _audio_datas.emplace(data);
                    }
                    _condition.notify_one();
                },
                data_manager);

            if (node["sample_rate"] && node["sample_rate"].get<std::string>() != "auto") {
                _sample_rate = node["sample_rate"].get<uint32_t>();
            }

            KeywordSpotterConfig ksc{};

            auto& model = node["model"];
            auto& transducer = model["transducer"];
            ksc.model_config.transducer.encoder = transducer["encoder"].get<std::string>();
            ksc.model_config.transducer.decoder = transducer["decoder"].get<std::string>();
            ksc.model_config.transducer.joiner = transducer["joiner"].get<std::string>();
            ksc.model_config.tokens = model["tokens"].get<std::string>();
            ksc.model_config.num_threads = model["num_threads"].get<uint32_t>(1);
            ksc.model_config.provider = model["provider"].get<std::string>("cpu");
            ksc.model_config.debug = model["debug"].get<bool_t>(False);
            ksc.keywords_file = node["keywords"]["path"].get<std::string>();

            _keyword_spotter = make_unique<KeywordSpotter>(move(KeywordSpotter::Create(ksc)));

            _init = True;
            spdlog::trace("kws Init!");
        } while (false);

        return result;
    }

    auto _run_() {
        int32_t result{0};

        do {
            if (!_init) {
                spdlog::error("kws not initialized!");
                break;
            }

            auto __stream = _keyword_spotter->CreateStream();
            while (!_exit) {
                std::unique_lock<std::mutex> __lock(_mutex);
                _condition.wait(__lock);
                if (_exit) {
                    break;
                }
                if (_audio_datas.empty()) {
                    continue;
                }
                auto __audio_datas = std::move(_audio_datas);
                _audio_datas = {};
                __lock.unlock();

                spdlog::debug("kws handle enter:");
                while (!__audio_datas.empty()) {
                    auto __audio_data_any = std::move(__audio_datas.front());
                    __audio_datas.pop();
                    auto __audio_data =
                        any_cast<std::shared_ptr<vector_t<float32_t>>>(__audio_data_any);
                    __stream.AcceptWaveform(_sample_rate, __audio_data->data(),
                                            __audio_data->size());
                }

                while (_keyword_spotter->IsReady(&__stream)) {
                    _keyword_spotter->Decode(&__stream);
                    auto r = _keyword_spotter->GetResult(&__stream);
                    if (!r.keyword.empty()) {
                        _publisher_wakeup->publish(r.keyword);
                        spdlog::info("kws sended {} to wakeup!", r.keyword);
                        _keyword_spotter->Reset(&__stream);
                    }
                }
                spdlog::debug("kws handle exit!");
            }
        } while (false);

        return result;
    }

public:
    kws(self const&) = delete;
    kws(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    kws() = default;

    template <class... _Args>
    kws(_Args&&... args) {
        bool_t result{_init_(forward<_Args>(args)...)};
        throw_if(!result, "kws::exception");
    }

    ~kws() noexcept {
        exit();
        spdlog::trace("kws Deinit!");
    }

    template <class... _Args>
    auto init(_Args&&... args) {
        return _init_(forward<_Args>(args)...);
    }

    template <class... _Args>
    auto exec(_Args&&... __args) {
        return _run_(forward<_Args>(__args)...);
    }

    auto exit() {
        _exit = True;
        _condition.notify_one();
    }
};
};  // namespace qlib

#endif

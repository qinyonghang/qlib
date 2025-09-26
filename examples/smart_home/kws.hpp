#ifndef KWS_HPP
#define KWS_HPP

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>

#include "sherpa-onnx/c-api/cxx-api.h"

#include "qlib/any.h"
#include "qlib/data.h"
#include "qlib/yaml.h"

#include "logger.hpp"

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
    uint32_t _in_sample_rate{16000};
    unique_ptr_t<publisher_type> _publisher_wakeup;
    unique_ptr_t<subscriber_type> _subscriber_audio;
    unique_ptr_t<KeywordSpotter> _keyword_spotter{};
    std::mutex _mutex;
    std::condition_variable _condition;
    std::queue<any_t<>> _audio_datas;
    logger& _logger;

    template <class _Str>
    _Str _file_(_Str const& __file) const {
        _Str __f{__file};
        if (!std::filesystem::exists(__f)) {
            char buf[1024u];
            auto len = readlink("/proc/self/exe", buf, sizeof(buf));
            if (len != -1) {
                buf[len] = '\0';
                __f = (std::filesystem::path(buf).parent_path() / "kws" / __f).string();
            }
        }
        return __f;
    }

    template <class _Yaml>
    auto _init_(_Yaml const& node, _DataManager& data_manager) {
        int32_t result{0};

        do {
            _publisher_wakeup = qlib::make_unique<publisher_type>(
                node["out_topic"].template get<std::string>(), data_manager);

            _subscriber_audio = qlib::make_unique<subscriber_type>(
                node["in_topic"].template get<std::string>(),
                [this](any_t<> const& data) {
                    _logger.debug("kws received audio data from audio::reader!");
                    {
                        std::lock_guard<std::mutex> lock{_mutex};
                        _audio_datas.emplace(data);
                    }
                    _condition.notify_one();
                },
                data_manager);

            if (node["in_sample_rate"] &&
                node["in_sample_rate"].template get<std::string>() != "auto") {
                _in_sample_rate = node["in_sample_rate"].template get<uint32_t>();
            }

            KeywordSpotterConfig ksc{};

            auto& model = node["model"];
            auto& transducer = model["transducer"];
            auto __encoder_path = _file_(transducer["encoder"].template get<std::string>());
            _logger.debug("kws encoder: {}", __encoder_path);
            throw_if(!std::filesystem::exists(__encoder_path), "file not found");
            ksc.model_config.transducer.encoder = __encoder_path;
            auto __decoder_path = _file_(transducer["decoder"].template get<std::string>());
            _logger.debug("kws decoder: {}", __encoder_path);
            throw_if(!std::filesystem::exists(__decoder_path), "file not found");
            ksc.model_config.transducer.decoder = __decoder_path;
            auto __joiner_path = _file_(transducer["joiner"].template get<std::string>());
            _logger.debug("kws joiner: {}", __joiner_path);
            throw_if(!std::filesystem::exists(__joiner_path), "file not found");
            ksc.model_config.transducer.joiner = __joiner_path;
            auto __tokens_path = _file_(model["tokens"].template get<std::string>());
            _logger.debug("kws tokens: {}", __tokens_path);
            throw_if(!std::filesystem::exists(__tokens_path), "file not found");
            ksc.model_config.tokens = __tokens_path;
            ksc.model_config.num_threads = model["num_threads"].template get<uint32_t>(1);
            ksc.model_config.provider = model["provider"].template get<std::string>("cpu");
            ksc.model_config.debug = model["debug"].template get<bool_t>(False);
            auto __keywords_path = _file_(node["keywords"]["path"].template get<std::string>());
            _logger.debug("kws keywords: {}", __keywords_path);
            throw_if(!std::filesystem::exists(__keywords_path), "file not found");
            ksc.keywords_file = __keywords_path;

            _keyword_spotter = make_unique<KeywordSpotter>(move(KeywordSpotter::Create(ksc)));

            _init = True;
            _logger.trace("kws init!");
        } while (false);

        return result;
    }

    auto _run_() {
        int32_t result{0};

        do {
            if (!_init) {
                _logger.error("kws not initialized!");
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

                _logger.debug("kws handle enter:");
                while (!__audio_datas.empty()) {
                    auto __audio_data_any = std::move(__audio_datas.front());
                    __audio_datas.pop();
                    auto __audio_data =
                        any_cast<std::shared_ptr<vector_t<float32_t>>>(__audio_data_any);
                    __stream.AcceptWaveform(_in_sample_rate, __audio_data->data(),
                                            __audio_data->size());
                }

                while (_keyword_spotter->IsReady(&__stream)) {
                    _keyword_spotter->Decode(&__stream);
                    auto r = _keyword_spotter->GetResult(&__stream);
                    if (!r.keyword.empty()) {
                        _publisher_wakeup->publish(r.keyword);
                        _logger.info("kws sended {} to wakeup!", r.keyword);
                        _keyword_spotter->Reset(&__stream);
                    }
                }
                _logger.debug("kws handle exit!");
            }
        } while (false);

        return result;
    }

public:
    kws() = delete;
    kws(self const&) = delete;
    kws(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    template <class _Yaml>
    kws(_Yaml const& __node, _DataManager& __manager, logger& __logger) : _logger(__logger) {
        auto __result = _init_(__node, __manager);
        throw_if(__result != 0, "kws exception");
    }

    ~kws() noexcept {
        exit();
        _logger.trace("kws deinit!");
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

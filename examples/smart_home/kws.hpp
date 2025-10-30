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
    std::queue<any_t> _audio_datas;
    slogger& _logger;

    std::string _file_(string_view_t __file) const {
        std::string __f{__file.begin(), __file.end()};
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
    auto _init_(_Yaml const& node, _DataManager& data_manager) {
        int32_t result{0};

        do {
            _publisher_wakeup = qlib::make_unique<publisher_type>(
                node["out_topic"].template get<string_view_t>(), data_manager);

            _subscriber_audio = qlib::make_unique<subscriber_type>(
                node["in_topic"].template get<string_view_t>(),
                [this](any_t const& data) {
                    {
                        std::lock_guard<std::mutex> lock{_mutex};
                        _audio_datas.emplace(data);
                    }
                    _condition.notify_one();
                    auto __samples = any_cast<std::shared_ptr<vector_t<float32_t>>>(data);
                    _logger.debug("kws: received {} samples!", __samples->size());
                },
                data_manager);

            if (node["in_sample_rate"] &&
                node["in_sample_rate"].template get<string_view_t>() != "auto") {
                _in_sample_rate = node["in_sample_rate"].template get<uint32_t>();
            }

            KeywordSpotterConfig ksc{};

            auto& model = node["model"];
            auto& transducer = model["transducer"];
            auto __encoder_path = _file_(transducer["encoder"].template get<string_view_t>());
            if (!std::filesystem::exists(__encoder_path)) {
                _logger.error("kws: file not found! {}", __encoder_path);
                result = -1;
                break;
            }
            ksc.model_config.transducer.encoder = __encoder_path;
            auto __decoder_path = _file_(transducer["decoder"].template get<string_view_t>());
            if (!std::filesystem::exists(__decoder_path)) {
                _logger.error("kws: file not found! {}", __decoder_path);
                result = -1;
                break;
            }
            ksc.model_config.transducer.decoder = __decoder_path;
            auto __joiner_path = _file_(transducer["joiner"].template get<string_view_t>());
            if (!std::filesystem::exists(__joiner_path)) {
                _logger.error("kws: file not found! {}", __joiner_path);
                result = -1;
                break;
            }
            ksc.model_config.transducer.joiner = __joiner_path;
            auto __tokens_path = _file_(model["tokens"].template get<string_view_t>());
            if (!std::filesystem::exists(__tokens_path)) {
                _logger.error("kws: file not found! {}", __tokens_path);
                result = -1;
                break;
            }
            ksc.model_config.tokens = __tokens_path;
            ksc.model_config.num_threads = model["num_threads"].template get<uint32_t>(1);
            ksc.model_config.provider = model["provider"].template get<string_t>("cpu").c_str();
            ksc.model_config.debug = model["debug"].template get<bool_t>(False);
            auto __keywords_path = _file_(node["keywords"]["path"].template get<string_view_t>());
            if (!std::filesystem::exists(__keywords_path)) {
                _logger.error("kws: file not found! {}", __keywords_path);
                result = -1;
                break;
            }
            ksc.keywords_file = __keywords_path;

            _keyword_spotter = make_unique<KeywordSpotter>(qlib::move(KeywordSpotter::Create(ksc)));

            _init = True;
            _logger.trace("kws: init!");
        } while (false);

        return result;
    }

    auto _run_() {
        int32_t result{0};

        do {
            if (!_init) {
                _logger.error("kws: not initialized!");
                break;
            }

            auto __stream = _keyword_spotter->CreateStream();
            std::shared_ptr<vector_t<float32_t>> __samples{nullptr};
            while (!_exit) {
                {
                    std::unique_lock<std::mutex> __lock(_mutex);
                    while (_audio_datas.empty() && !_exit) {
                        _condition.wait(__lock);
                    }
                    if (_exit) {
                        break;
                    }
                    __samples =
                        any_cast<std::shared_ptr<vector_t<float32_t>>>(_audio_datas.front());
                    _audio_datas.pop();
                }

                _logger.debug("kws: handle enter:");
                __stream.AcceptWaveform(_in_sample_rate, __samples->data(), __samples->size());
                while (_keyword_spotter->IsReady(&__stream)) {
                    _keyword_spotter->Decode(&__stream);
                    auto r = _keyword_spotter->GetResult(&__stream);
                    if (!r.keyword.empty()) {
                        _publisher_wakeup->publish(std::make_shared<string_t>(r.keyword));
                        _logger.info("kws: sended {} to wakeup!", r.keyword);
                        _keyword_spotter->Reset(&__stream);
                    }
                }
                _logger.debug("kws: handle exit!");
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

    kws(slogger& __logger) : _logger(__logger) {}

    template <class _Yaml>
    kws(_Yaml const& __node, _DataManager& __manager, slogger& __logger) : _logger(__logger) {
        auto __result = _init_(__node, __manager);
        throw_if(__result != 0, "kws exception");
    }

    ~kws() noexcept {
        exit();
        _logger.trace("kws: deinit!");
    }

    template <class... _Args>
    auto init(_Args&&... args) {
        return _init_(qlib::forward<_Args>(args)...);
    }

    template <class... _Args>
    auto exec(_Args&&... __args) {
        return _run_(qlib::forward<_Args>(__args)...);
    }

    auto exit() {
        _exit = True;
        _condition.notify_one();
    }
};
};  // namespace qlib

#endif

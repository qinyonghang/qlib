#ifndef RECOGNIZER_HPP
#define RECOGNIZER_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

#include "sherpa-onnx/c-api/cxx-api.h"

#include "qlib/any.h"
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

protected:
    bool_t _init{False};
    bool_t _exit{False};
    std::mutex _mutex;
    std::condition_variable _cond;
    logger& _logger;
    uint32_t _in_sample_rate{0};
    uint32_t _in_max_size{0};
    std::unique_ptr<subscriber_type> _subscriber;
    std::unique_ptr<publisher_type> _publisher;
    std::unique_ptr<Recognizer> _model;
    std::queue<any_t<>> _in_datas;

    template <class _Str>
    _Str _file_(_Str const& __file) const {
        _Str __f{__file};
        if (!std::filesystem::exists(__f)) {
            char buf[1024u];
            auto len = readlink("/proc/self/exe", buf, sizeof(buf));
            if (len != -1) {
                buf[len] = '\0';
                __f = (std::filesystem::path(buf).parent_path() / "recognizer" / __f).string();
            }
        }
        return __f;
    }

    template <class _Yaml>
    auto _init_(_Yaml const& __yaml, _DataManager& __manager) {
        int32_t result{0};

        do {
            _subscriber = std::make_unique<subscriber_type>(
                __yaml["in_topic"].template get<std::string>(),
                [this](any_t<> const& __data) {
                    {
                        std::lock_guard<std::mutex> __lock(_mutex);
                        if (_in_datas.size() > _in_max_size) {
                            _in_datas.pop();
                        }
                        _in_datas.emplace(__data);
                    }
                    _cond.notify_one();
                },
                __manager);

            _publisher = std::make_unique<publisher_type>(
                __yaml["out_topic"].template get<std::string>(), __manager);

            _in_sample_rate = __yaml["in_sample_rate"].template get<uint32_t>();
            _in_max_size = __yaml["in_max_size"].template get<uint32_t>();

            RecognizerConfig __config;

            auto& __model = __yaml["model"];
            auto __encoder_path = _file_(__model["encoder"].template get<std::string>());
            _logger.debug("recognizer encoder: {}", __encoder_path);
            throw_if(!std::filesystem::exists(__encoder_path), "file not found");
            __config.model_config.fire_red_asr.encoder = __encoder_path;
            auto __decoder_path = _file_(__model["decoder"].template get<std::string>());
            _logger.debug("recognizer decoder: {}", __encoder_path);
            throw_if(!std::filesystem::exists(__decoder_path), "file not found");
            __config.model_config.fire_red_asr.decoder = __decoder_path;
            __config.model_config.tokens = __model["tokens"].template get<std::string>();
            __config.model_config.num_threads = __model["num_threads"].template get<uint32_t>(1);
            __config.model_config.provider = __model["provider"].template get<std::string>("cpu");
            __config.model_config.debug = __model["debug"].template get<bool_t>(False);
            _model = std::make_unique<Recognizer>(std::move(Recognizer::Create(__config)));

            _init = True;
            _logger.trace("recognizer init!");
        } while (false);

        return result;
    }

    auto _exec_() {
        int32_t result{0};

        do {
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
        throw_if(result != 0, "recognizer exception");
    }

    template <class _Yaml>
    auto init(_Yaml const& __yaml, _DataManager& __manager) {
        return _init_(__yaml, __manager);
    }

    template <class... _Args>
    auto exec(_Args&&... __args) {
        return _exec_(forward<_Args>(__args)...);
    }
};

};  // namespace qlib

#endif
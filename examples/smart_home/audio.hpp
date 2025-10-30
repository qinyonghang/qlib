#ifndef AUDIO_H
#define AUDIO_H

#include <alsa/asoundlib.h>
#include <cmath>

#include "portaudio.h"

#include "qlib/data.h"
#include "qlib/yaml.h"

#include "logger.hpp"

namespace qlib {

namespace audio {

template <class _DataManager>
class reader final : public object {
public:
    using self = reader;
    using publisher_type = data::publisher<_DataManager>;

protected:
    bool_t _init{False};
    unique_ptr_t<publisher_type> _publisher;
    PaStream* _pa_stream{nullptr};
    uint32_t _channels{1u};
    uint32_t _sample_rate{44100u};
    uint32_t _out_sample_rate{16000u};
    slogger& _logger;

    static void _alsa_logger_(
        char const* file, int line, const char* function, int err, const char* fmt, ...) {}

    static void _resample_(float32_t* __out,
                           int16_t const* __in,
                           uint32_t __count,
                           uint32_t __sample_rate,
                           uint32_t __out_sample_rate) {
        auto ratio = float32_t(__out_sample_rate) / __sample_rate;
        auto out_count = uint32_t(std::round(__count * ratio));

        for (auto out_idx = 0u; out_idx < out_count; ++out_idx) {
            auto in_pos = out_idx / ratio;
            auto in_idx = uint32_t(in_pos);
            auto frac = in_pos - in_idx;

            auto left = (in_idx < __count) ? float32_t(__in[in_idx]) / 32768.0f : 0.0f;
            auto right = (in_idx + 1 < __count) ? float32_t(__in[in_idx + 1]) / 32768.0f : 0.0f;

            __out[out_idx] = float32_t(left * (1.0 - frac) + right * frac);
        }
    }

    template <class _Yaml>
    auto _init_(_Yaml const& node, _DataManager& manager) {
        int32_t result{0u};

        do {
            if (node["out_sample_rate"] &&
                node["out_sample_rate"].template get<string_view_t>() != "auto") {
                _out_sample_rate = node["out_sample_rate"].template get<uint32_t>();
            }

            _publisher = qlib::make_unique<publisher_type>(
                node["out_topic"].template get<string_view_t>(), manager);

            snd_lib_error_set_handler(_alsa_logger_);

            auto pa_error = Pa_Initialize();
            if (pa_error != paNoError) {
                _logger.error("PortAudio::Initialize return {}!", pa_error);
                result = -1;
                break;
            }

            auto device = Pa_GetDefaultInputDevice();
            if (device == paNoDevice) {
                _logger.error("PortAudio::No default input device!");
                result = -1;
                break;
            }
            auto device_info = Pa_GetDeviceInfo(device);
            _logger.info("PortAudio::Default Input Device: Name:{} Sample Rate:{}",
                         device_info->name, device_info->defaultSampleRate);

            PaStreamParameters pa_parameters{
                .device = device,
                .channelCount = int32_t(_channels),
                .sampleFormat = paInt16,
                .suggestedLatency = device_info->defaultHighInputLatency,
                .hostApiSpecificStreamInfo = nullptr};

            _sample_rate = uint32_t(device_info->defaultSampleRate);
            if (node["sample_rate"] &&
                node["sample_rate"].template get<string_view_t>() != "auto") {
                _sample_rate = node["sample_rate"].template get<uint32_t>();
            }
            uint32_t frames_per_read = _sample_rate * 0.5;
            if (node["frames_per_read"] &&
                node["frames_per_read"].template get<string_view_t>() != "auto") {
                frames_per_read = node["frames_per_read"].template get<uint32_t>();
            }

            pa_error = Pa_OpenStream(
                &_pa_stream, &pa_parameters, nullptr, _sample_rate, frames_per_read, paClipOff,
                +[](void const* input, void* output, unsigned long __count,
                    PaStreamCallbackTimeInfo const* timeInfo, PaStreamCallbackFlags statusFlags,
                    void* userData) {
                    self* reader = (self*)(userData);
                    reader->_logger.debug("audio::reader: received {} samples!", uint64_t(__count));
                    if (reader->_publisher != nullptr) {
                        uint32_t __new_count =
                            __count * reader->_out_sample_rate / reader->_sample_rate;
                        vector_t<float32_t> __output;
                        __output.resize(__new_count);
                        _resample_(__output.data(), (int16_t const*)input, __count,
                                   reader->_sample_rate, reader->_out_sample_rate);
                        reader->_publisher->publish(
                            std::make_shared<vector_t<float32_t>>(std::move(__output)));
                        reader->_logger.debug("audio::reader: sended {} samples!", __new_count);
                    }

                    return int32_t(paContinue);
                },
                this);
            if (pa_error != paNoError) {
                Pa_Terminate();
                _logger.error("PortAudio::OpenStream return {}!", pa_error);
                result = -1;
                break;
            }

            _init = True;
            _logger.trace("audio::reader: init!");
        } while (false);

        return result;
    }

public:
    reader() = delete;
    reader(self const&) = delete;
    reader(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    reader(slogger& __logger) : _logger(__logger) {}

    template <class _Yaml>
    reader(_Yaml const& __node, _DataManager& __manager, slogger& __logger) : _logger(__logger) {
        auto __result = _init_(__node, __manager);
        throw_if(__result != 0, "audio::reader exception");
    }

    ~reader() {
        if (_init) {
            PaError pa_error{paNoError};

            if (_pa_stream) {
                pa_error = Pa_CloseStream(_pa_stream);
                if (pa_error != paNoError) {
                    _logger.error("PortAudio::CloseStream return {}!", pa_error);
                }
            }

            pa_error = Pa_Terminate();
            if (pa_error != paNoError) {
                _logger.error("PortAudio::Terminate return {}!", pa_error);
            }
        }

        _logger.trace("audio::reader: deinit!");
    }

    template <class... _Args>
    auto init(_Args&&... args) {
        return _init_(qlib::forward<_Args>(args)...);
    }

    auto start() { return Pa_StartStream(_pa_stream); }

    auto stop() { return Pa_StopStream(_pa_stream); }
};

};  // namespace audio

};  // namespace qlib

#endif
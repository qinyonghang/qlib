#ifndef AUDIO_H
#define AUDIO_H

#include <alsa/asoundlib.h>

#include "portaudio.h"
#include "spdlog/spdlog.h"
// #include "yaml-cpp/yaml.h"

#include "qlib/data.h"
#include "qlib/yaml.h"

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
    uint32_t _output_sample_rate{18000u};
    static void _alsa_logger_(
        char const* file, int line, const char* function, int err, const char* fmt, ...) {
        char buf[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        spdlog::error("{}:{}:{} {} {}", file, line, function, err, buf);
    }

    static void _resample_(float32_t* __out,
                           int16_t const* __in,
                           uint32_t __count,
                           uint32_t __sample_rate,
                           uint32_t __out_sample_rate) {
        float32_t ratio = float32_t(__out_sample_rate) / __sample_rate;
        uint32_t out_count = uint32_t(std::round(__count * ratio));

        for (auto out_idx = 0u; out_idx < out_count; ++out_idx) {
            float32_t in_pos = out_idx / ratio;
            uint32_t in_idx = uint32_t(in_pos);
            float32_t frac = in_pos - in_idx;

            float32_t left = (in_idx < __count) ? float32_t(__in[in_idx]) / 32768.0f : 0.0f;
            float32_t right =
                (in_idx + 1 < __count) ? float32_t(__in[in_idx + 1]) / 32768.0f : 0.0f;

            __out[out_idx] = float32_t(left * (1.0 - frac) + right * frac);
        }
    }

    auto _init_(yaml_view_t const& node, _DataManager& manager) {
        int32_t result{0u};

        do {
            // _channels = 1u;
            // if (node["channels"] && node["channels"].get<std::string>() != "auto") {
            //     _channels = node["channels"].get<uint32_t>();
            // }

            auto& __output_samples = node["output_samples"];
            if (__output_samples["sample_rate"] &&
                __output_samples["sample_rate"].get<std::string>() != "auto") {
                _output_sample_rate = __output_samples["sample_rate"].get<uint32_t>();
            }

            _publisher =
                qlib::make_unique<publisher_type>(node["publish"].get<std::string>(), manager);

            snd_lib_error_set_handler(_alsa_logger_);

            auto pa_error = Pa_Initialize();
            if (pa_error != paNoError) {
                spdlog::error("PortAudio::Initialize return {}!", pa_error);
                result = -1;
                break;
            }
            spdlog::trace("PortAudio Init!");

            auto device = Pa_GetDefaultInputDevice();
            if (device == paNoDevice) {
                spdlog::error("PortAudio::No default input device!");
                result = -1;
                break;
            }
            auto device_info = Pa_GetDeviceInfo(device);
            spdlog::info("PortAudio::Default Input Device: {} {} {} {} {} {} {} {}",
                         device_info->name, device_info->defaultSampleRate,
                         device_info->maxInputChannels, device_info->maxOutputChannels,
                         device_info->defaultLowInputLatency, device_info->defaultHighInputLatency,
                         device_info->defaultLowOutputLatency,
                         device_info->defaultHighOutputLatency);

            PaStreamParameters pa_parameters{
                .device = device,
                .channelCount = int32_t(_channels),
                .sampleFormat = paInt16,
                .suggestedLatency = device_info->defaultHighInputLatency,
                .hostApiSpecificStreamInfo = nullptr};

            _sample_rate = uint32_t(device_info->defaultSampleRate);
            if (node["sample_rate"] && node["sample_rate"].get<std::string>() != "auto") {
                _sample_rate = node["sample_rate"].get<uint32_t>();
            }
            uint32_t frames_per_read = _sample_rate * 0.5;
            if (node["frames_per_read"] && node["frames_per_read"].get<std::string>() != "auto") {
                frames_per_read = node["frames_per_read"].get<uint32_t>();
            }

            pa_error = Pa_OpenStream(
                &_pa_stream, &pa_parameters, nullptr, _sample_rate, frames_per_read, paClipOff,
                +[](void const* input, void* output, unsigned long __count,
                    PaStreamCallbackTimeInfo const* timeInfo, PaStreamCallbackFlags statusFlags,
                    void* userData) {
                    spdlog::debug("audio::reader received {} samples!", __count);
                    self* reader = (self*)(userData);
                    uint32_t __new_count =
                        __count * reader->_output_sample_rate / reader->_sample_rate;
                    vector_t<float32_t> __output;
                    __output.resize(__new_count);
                    _resample_(__output.data(), (int16_t const*)input, __count,
                               reader->_sample_rate, reader->_output_sample_rate);
                    reader->_publisher->publish(
                        std::make_shared<vector_t<float32_t>>(std::move(__output)));
                    spdlog::debug("audio::reader sended {} samples!", __new_count);
                    return int32_t(paContinue);
                },
                this);
            if (pa_error != paNoError) {
                Pa_Terminate();
                spdlog::error("PortAudio::OpenStream return {}!", pa_error);
                result = -1;
                break;
            }

            _init = True;
            spdlog::trace("audio::reader Init!");
        } while (false);

        return result;
    }

public:
    reader(self const&) = delete;
    reader(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    reader() = default;

    template <class... _Args>
    reader(_Args&&... args) {
        bool_t result{_init_(forward<_Args>(args)...)};
        throw_if(!result, "audio::reader::exception");
    }

    ~reader() {
        if (_init) {
            PaError pa_error{paNoError};

            if (_pa_stream) {
                pa_error = Pa_CloseStream(_pa_stream);
                if (pa_error != paNoError) {
                    spdlog::error("PortAudio::CloseStream return {}!", pa_error);
                }
            }

            pa_error = Pa_Terminate();
            if (pa_error != paNoError) {
                spdlog::error("PortAudio::Terminate return {}!", pa_error);
            }
        }

        spdlog::trace("audio::reader Deinit!");
    }

    template <class... _Args>
    auto init(_Args&&... args) {
        return _init_(forward<_Args>(args)...);
    }

    auto start() { return Pa_StartStream(_pa_stream); }

    auto stop() { return Pa_StopStream(_pa_stream); }
};

};  // namespace audio

};  // namespace qlib

#endif
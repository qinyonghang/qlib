#ifndef _QLIB_WAV_H_
#define _QLIB_WAV_H_

#include "qlib/object.h"
#include "qlib/string.h"

namespace qlib {

namespace wav {

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

};  // namespace wav

};  // namespace qlib

#endif

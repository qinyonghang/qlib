#ifndef _QLIB_WAV_H_
#define _QLIB_WAV_H_

#include "qlib/object.h"
#include "qlib/string.h"

namespace qlib {

namespace wav {

template <class Sample, class OutStream>
class writer final : public traits<OutStream>::reference {
protected:
    struct Header {
        uint8_t riff_id[4] = {'R', 'I', 'F', 'F'};
        uint32_t riff_size = 0;
        uint8_t wave_id[4] = {'W', 'A', 'V', 'E'};
        uint8_t fmt_id[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_size = 16u;                         // PCM格式固定为16字节
        uint16_t audio_format = 1;                       // 1 = PCM
        uint16_t num_channels = 0;                       // 声道数（1=单声道，2=立体声）
        uint32_t sample_rate = 0;                        // 采样率（如44100Hz）
        uint32_t byte_rate = 0;                          // 字节率 = 采样率 * 声道数 * 位深/8
        uint16_t block_align = 0;                        // 块对齐 = 声道数 * 位深/8
        uint16_t bits_per_sample = sizeof(Sample) * 8u;  // 位深（16位）
        uint8_t data_id[4] = {'d', 'a', 't', 'a'};
        uint32_t data_size = 0;
    };
    Header _header{};

    OutStream& _stream_() noexcept { return static_cast<base&>(*this); }

    OutStream& _stream_() const noexcept { return static_cast<base&>(const_cast<self&>(*this)); }

public:
    using base = typename traits<OutStream>::reference;
    using self = writer<OutStream>;

    writer(self const&) = delete;
    writer(self&&) = delete;
    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    template <class Stream>
    writer(Stream& __stream, size_t __channels, uint32_t __sample_rate) : base(__stream) {
        _header.num_channels = __channels;
        _header.sample_rate = __sample_rate;
        _header.byte_rate = __sample_rate * __channels * sizeof(Sample);
        _header.block_align = __channels * sizeof(Sample);
        _header.riff_size = 36;
        _header.data_size = 0;

        _stream_().write((char const*)(&_header), sizeof(Header));
    }

    ~writer() {
        
    }
};

};  // namespace wav

};  // namespace qlib

#endif

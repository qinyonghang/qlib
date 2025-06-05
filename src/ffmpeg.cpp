#include "qlib/rtsp.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <vector>

#include "qlib/singleton.h"

namespace qlib {
namespace rtsp {

class impl : public object {
public:
    using self = writer;

    class register2 : public object {
    public:
        using self = writer;
        using ptr = std::shared_ptr<self>;
        using base = object;

        template <class... Args>
        static ptr make(Args&&... args) {
            return ref_singleton<self>::make(std::forward<Args>(args)...);
        }

    protected:
        register2() {
            int32_t result{0u};
            result = avformat_network_init();
            THROW_EXCEPTION(0 == result, "avformat_network_init return {}!", result);
        }

        ~register2() {
            int32_t result{0u};
            result = avformat_network_deinit();
            if (0 != result) {
                qError("avformat_network_deinit result {}!", result);
            }
        }

        friend class ref_singleton<self>;
    };

    register2::ptr register_ptr;
    std::shared_ptr<AVFormatContext> context_ptr;
    AVStream* stream_ptr;
    std::mutex mutex;
    std::condition_variable condition;
    std::queue<self::frame> frames;
    std::future<void> future;
    std::atomic_bool exit{false};
    self::init_parameter parameter;

    ~impl() {
        if (future.valid()) {
            impl::exit = true;
            condition.notify_all();
            future.get();
        }
    }
};

int32_t writer::init(init_parameter const& parameter) {
    int32_t result{0u};

    do {
        qInfo("RTSP Writer Parameter: {}!", parameter);

        auto impl_ptr = std::make_shared<impl>();

        impl_ptr->register_ptr = impl::register2::make();

        AVFormatContext* context{nullptr};
        result = avformat_alloc_output_context2(&context, nullptr, "rtsp", parameter.url.c_str());
        if (result < 0) {
            qError("avformat_alloc_output_context2 return {}!", result);
            result = static_cast<int32_t>(error::unknown);
            break;
        }
        context->oformat->flags |= AVFMT_GLOBALHEADER;
        impl_ptr->context_ptr = std::shared_ptr<AVFormatContext>(
            context, [](AVFormatContext* ptr) { avformat_free_context(ptr); });

        auto stream = avformat_new_stream(context, nullptr);
        if (nullptr == stream) {
            qError("avformat_new_stream return nullptr!");
            result = static_cast<int32_t>(error::unknown);
            break;
        }

        // 设置视频流参数
        stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        stream->codecpar->codec_id = AV_CODEC_ID_H264;
        stream->codecpar->width = parameter.width;
        stream->codecpar->height = parameter.height;
        stream->codecpar->format = AV_PIX_FMT_YUV420P;
        stream->codecpar->bit_rate = parameter.bitrate;
        stream->time_base = {1, static_cast<int32_t>(parameter.fps)};

        impl_ptr->stream_ptr = stream;

        AVDictionary* format_opts = nullptr;
        av_dict_set(&format_opts, "rtsp_transport", "tcp", 0);
        result = avformat_write_header(context, &format_opts);
        if (result < 0) {
            qError("avformat_write_header return {}!", result);
            result = static_cast<int32_t>(error::unknown);
            break;
        }

        impl_ptr->parameter = parameter;
        impl_ptr->future = std::async(
            std::launch::async,
            [](impl* impl_ptr) {
                qTrace("RTSP Writer Thread Enter:");

                int32_t result{0};
                auto packet = av_packet_alloc();
                int64_t pts{0u};
                std::queue<frame> frames;
                while (!impl_ptr->exit) {
                    {
                        std::unique_lock<std::mutex> lock(impl_ptr->mutex);
                        impl_ptr->condition.wait(lock);
                        frames = std::move(impl_ptr->frames);
                    }
                    if (impl_ptr->exit) {
                        break;
                    }

                    while (!frames.empty()) {
                        auto frame = std::move(frames.front());
                        frames.pop();
                        qTrace("RTSP Receive Frame: {}", frame.size());

                        av_new_packet(packet, frame.size());
                        if (frame.size() > 5 && ((frame[4] & 0x1f) == 5)) {
                            packet->flags |= AV_PKT_FLAG_KEY;
                        } else {
                            packet->flags = 0;
                        }
                        memcpy(packet->data, frame.data(), frame.size());
                        packet->stream_index = impl_ptr->stream_ptr->index;
                        packet->pts = pts++;
                        packet->dts = packet->pts;
                        packet->duration = 1;
                        packet->pos = -1;
                        result = av_interleaved_write_frame(impl_ptr->context_ptr.get(), packet);
                        if (result < 0) {
                            qError("Write frame failed: {}", result);
                        }
                        av_packet_unref(packet);
                    }
                }

                av_packet_free(&packet);

                qTrace("RTSP Writer Thread Exit!");
            },
            impl_ptr.get());

        self::impl_ptr = impl_ptr;
    } while (false);

    return result;
}

int32_t writer::emplace(frame const& frame) {
    int32_t result{0};

    do {
        auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
        if (nullptr == impl_ptr) {
            result = static_cast<int32_t>(error::impl_nullptr);
            break;
        }

        std::lock_guard<std::mutex> lock(impl_ptr->mutex);
        impl_ptr->frames.emplace(frame);
        impl_ptr->condition.notify_one();
    } while (false);

    return result;
}

int32_t writer::emplace(frame&& frame) {
    int32_t result{0};

    do {
        auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
        if (nullptr == impl_ptr) {
            result = static_cast<int32_t>(error::impl_nullptr);
            break;
        }

        std::lock_guard<std::mutex> lock(impl_ptr->mutex);
        impl_ptr->frames.emplace(std::move(frame));
        impl_ptr->condition.notify_one();
    } while (false);

    return result;
}

};  // namespace rtsp
};  // namespace qlib

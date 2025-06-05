#pragma once

#include "qlib/exception.h"
#include "qlib/logger.h"

namespace qlib {
namespace rtsp {

class writer : public object {
public:
    using self = writer;
    using ptr = std::shared_ptr<self>;
    using base = object;

    struct init_parameter : public base::parameter {
        string url;
        uint32_t width{1920u};
        uint32_t height{1080u};
        uint32_t fps{30u};
        uint32_t bitrate{4096000u};

        auto to_string() const {
            return fmt::format("[{},{},{},{},{}]", url, width, height, fps, bitrate);
        }
    };

    using frame = std::vector<uint8_t>;

    template <class... Args>
    static ptr make(Args&&... args) {
        ptr result{nullptr};

        auto ptr = std::make_shared<self>();
        if (ptr->init(std::forward<Args>(args)...) == 0) {
            result = ptr;
        }

        return result;
    }

    writer() = default;

    template <class... Args>
    writer(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(init_parameter const& parameter);

    int32_t init(
        string_t const& url, uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
        return init(init_parameter{
            .url = url,
            .width = width,
            .height = height,
            .fps = fps,
            .bitrate = bitrate,
        });
    }

    int32_t emplace(frame const& frame);
    int32_t emplace(frame&& frame);

protected:
    object::ptr impl_ptr{nullptr};
};

};  // namespace rtsp
};  // namespace qlib

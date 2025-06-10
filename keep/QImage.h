#pragma once

#include <memory>
#include <string>

#include "object.h"

namespace qlib {
class image : public object<image> {
public:
    enum : uint32_t {
        QIMAGE_TYPE_NONE = 1 << 0,
        QIMAGE_TYPE_GRAY,
        QIMAGE_TYPE_BGR888,
        QIMAGE_TYPE_RGB888,
        QIMAGE_TYPE_NV12,
        QIMAGE_LAYOUT_NONE = 1 << 8,
        QIMAGE_LAYOUT_HWC,
        QIMAGE_LAYOUT_CHW,
        QIMAGE_CVT_NONE = 1 << 0,
        QIMAGE_CVT_BGR2RGB,
        QIMAGE_CVT_RGB2BGR,
        QIMAGE_CVT_HWC2CHW,
    };

    image(uint32_t width, uint32_t height, uint32_t type);
    image(std::string const&, uint32_t width, uint32_t height, uint32_t type);
    image(uint8_t* data, uint32_t width, uint32_t height, uint32_t type);

#ifdef WITH_OPENCV
    image(cv::Mat const& image);
#endif

    int32_t init(std::string const&, uint32_t width, uint32_t height, uint32_t type);
    int32_t init(uint8_t* data, uint32_t width, uint32_t height, uint32_t type);

    int32_t convert(uint32_t type);

    uint32_t width() const { return __width; }
    uint32_t height() const { return __height; }
    uint32_t type() const { return __type & 0xff; }
    uint8_t* data() { return __impl.get(); }
    uint8_t const* data() const { return const_cast<image*>(this)->data(); }

protected:
    std::shared_ptr<uint8_t> __impl;
    uint32_t __width;
    uint32_t __height;
    uint32_t __type;
};

template <typename QType1 = image, typename QType2 = QType1>
class QResize {
public:
    void operator()(QType2* dst, QType1 const& src);
};

template <>
void QResize<image, image>::operator()(image* dst, image const& src);

template <typename QType1 = image, typename QType2 = QType1>
class QCrop {
public:
    void operator()(QType2* dst, QType1 const& src, uint32_t x, uint32_t y);
};

template <>
void QCrop<image, image>::operator()(image* dst, image const& src, uint32_t x, uint32_t y);

};  // namespace qlib
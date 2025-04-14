// #include "QImage.h"

// #include <sys/stat.h>

// #include <fstream>

// #include "QException.h"
// #include "QLog.h"

// namespace qlib {
// namespace {

// int32_t read_nv12(std::shared_ptr<uint8_t>* image,
//                   std::string const& image_path,
//                   uint32_t width,
//                   uint32_t height) {
//     int32_t result = 0;

//     do {
//         struct stat st;
//         if (stat(image_path.c_str(), &st) != 0) {
//             qError("Get image info failed... image_path: {}", image_path);
//             result = -1;
//             break;
//         }

//         if (st.st_size != width * height * 3 / 2) {
//             qError("Image file is invalid nv12 format...");
//             result = -1;
//             break;
//         }

//         *image = std::shared_ptr<uint8_t>{new uint8_t[st.st_size], [](uint8_t* p) { delete[] p; }};

//         std::ifstream fp{image_path};
//         if (!fp.is_open()) {
//             qError("Open image failed... image_path = {}", image_path);
//             result = -1;
//             break;
//         }

//         fp.read(reinterpret_cast<char*>(image->get()), st.st_size);
//         if (fp.bad()) {
//             qError("Read image failed... image_path = {}", image_path);
//             result = -1;
//             break;
//         }
//     } while (0);

//     return result;
// }

// int32_t split_neon(std::array<std::vector<uint8_t>, 3>* dst, std::vector<uint8_t> const& src) {
//     if ((src.size() % 3) != 0) {
//         return -1;
//     }

//     auto ele_size = src.size() / 3u;
//     dst->at(0).resize(ele_size);
//     dst->at(1).resize(ele_size);
//     dst->at(2).resize(ele_size);

//     auto i = 0u;

// #ifdef WITH_NEON
//     for (; i < ele_size / 16u; ++i) {
//         auto intlv_rgb = vld3q_u8(src.data() + 3u * 16u * i);
//         vst1q_u8(dst->at(0).data() + 16u * i, intlv_rgb.val[0]);
//         vst1q_u8(dst->at(1).data() + 16u * i, intlv_rgb.val[1]);
//         vst1q_u8(dst->at(2).data() + 16u * i, intlv_rgb.val[2]);
//     }
// #endif

//     for (; i < src.size(); ++i) {
//         dst->at(0).at(i) = src.at(3u * i + 0u);
//         dst->at(1).at(i) = src.at(3u * i + 1u);
//         dst->at(2).at(i) = src.at(3u * i + 2u);
//     }

//     return 0;
// }

// template <typename DstType, typename SrcType = DstType>
// void rgb_hwc2chw(DstType* dst, SrcType const* src, size_t ele_size) {
//     auto r = dst;
//     auto g = dst + ele_size;
//     auto b = dst + ele_size * 2;

//     for (auto i = 0u; i < ele_size; ++i) {
//         r[i] = static_cast<DstType>(src[3u * i + 0u]);
//         g[i] = static_cast<DstType>(src[3u * i + 1u]);
//         b[i] = static_cast<DstType>(src[3u * i + 2u]);
//     }
// }

// template <>
// void rgb_hwc2chw(uint8_t* dst, uint8_t const* src, size_t ele_size) {
//     auto r = dst;
//     auto g = dst + ele_size;
//     auto b = dst + ele_size * 2;
//     auto i = 0u;

// #ifdef WITH_NEON
//     for (; i < ele_size / 16u * 16u; i += 16u) {
//         auto vec = vld3q_u8(src + 3u * i);
//         vst1q_u8(r + i, vec.val[0]);
//         vst1q_u8(g + i, vec.val[1]);
//         vst1q_u8(b + i, vec.val[2]);
//     }
// #endif

//     for (; i < ele_size; ++i) {
//         r[i] = src[3u * i + 0u];
//         g[i] = src[3u * i + 1u];
//         b[i] = src[3u * i + 2u];
//     }
// }

// template <>
// void rgb_hwc2chw(float32_t* dst, float32_t const* src, size_t ele_size) {
//     auto r = dst;
//     auto g = dst + ele_size;
//     auto b = dst + ele_size * 2;
//     auto i = 0u;

// #ifdef WITH_NEON
//     for (; i < ele_size / 4u * 4u; i += 4u) {
//         auto vec = vld3q_f32(src + 3u * i);
//         vst1q_f32(r + i, vec.val[0]);
//         vst1q_f32(g + i, vec.val[1]);
//         vst1q_f32(b + i, vec.val[2]);
//     }
// #endif

//     for (; i < ele_size; ++i) {
//         r[i] = src[3u * i + 0u];
//         g[i] = src[3u * i + 1u];
//         b[i] = src[3u * i + 2u];
//     }
// }

// int32_t cvt_rgb2bgr(uint8_t* data, uint32_t width, uint32_t height) {
//     auto ele_size = width * height;

//     auto i = 0u;

// #ifdef WITH_NEON
//     for (; i < ele_size / 16u * 16u; i += 16u) {
//         auto vec = vld3q_u8(data + 3u * i);
//         std::swap(vec.val[0], vec.val[2]);
//         vst3q_u8(data + 3u * i, vec);
//     }
// #endif

//     for (; i < ele_size; ++i) {
//         std::swap(data[3u * i + 0u], data[3u * i + 2u]);
//     }

//     return 0;
// }

// void resize(uint8_t* dst,
//             uint32_t dst_width,
//             uint32_t dst_height,
//             uint8_t const* src,
//             uint32_t src_width,
//             uint32_t src_height) {
//     const float src_to_dst_ratio_x = static_cast<float>(src_width) / dst_width;
//     const float src_to_dst_ratio_y = static_cast<float>(src_height) / dst_height;

//     for (uint32_t y_dst = 0; y_dst < dst_height; ++y_dst) {
//         const float y_src_f = y_dst * src_to_dst_ratio_y;
//         const int y_src_floor = y_src_f;
//         const float dy = y_src_f - y_src_floor;

//         const uint8_t* src_row0 = src + y_src_floor * src_width * 3;
//         const uint8_t* src_row1 = src_row0 + src_width * 3;

//         for (uint32_t x_dst = 0; x_dst < dst_width; ++x_dst) {
//             const float x_src_f = x_dst * src_to_dst_ratio_x;
//             const int x_src_floor = x_src_f;
//             const float dx = x_src_f - x_src_floor;

//             const float one_minus_dx = 1.0f - dx;
//             const float one_minus_dy = 1.0f - dy;

//             const uint8_t* src_pixel00 = src_row0 + x_src_floor * 3;
//             const uint8_t* src_pixel01 = src_pixel00 + 3;
//             const uint8_t* src_pixel10 = src_row1 + x_src_floor * 3;
//             const uint8_t* src_pixel11 = src_pixel10 + 3;

//             float val = one_minus_dx * one_minus_dy * static_cast<float>(*src_pixel00++) +
//                 one_minus_dx * dy * static_cast<float>(*src_pixel01++) +
//                 dx * dy * static_cast<float>(*src_pixel11++) +
//                 dx * one_minus_dy * static_cast<float>(*src_pixel10++);
//             dst[x_dst * 3 + 0] = val;

//             val = one_minus_dx * one_minus_dy * static_cast<float>(*src_pixel00++) +
//                 one_minus_dx * dy * static_cast<float>(*src_pixel01++) +
//                 dx * dy * static_cast<float>(*src_pixel11++) +
//                 dx * one_minus_dy * static_cast<float>(*src_pixel10++);
//             dst[x_dst * 3 + 1] = val;

//             val = one_minus_dx * one_minus_dy * static_cast<float>(*src_pixel00++) +
//                 one_minus_dx * dy * static_cast<float>(*src_pixel01++) +
//                 dx * dy * static_cast<float>(*src_pixel11++) +
//                 dx * one_minus_dy * static_cast<float>(*src_pixel10++);
//             dst[x_dst * 3 + 2] = val;
//         }

//         dst += dst_width * 3;
//     }
// }

// void crop(uint8_t* dst,
//           uint32_t dst_width,
//           uint32_t dst_height,
//           uint8_t const* src,
//           uint32_t src_width,
//           uint32_t src_height,
//           uint32_t x,
//           uint32_t y) {
//     src = src + y * src_width * 3 + x * 3;

//     for (auto _ = 0u; _ < dst_height; ++_) {
//         std::memcpy(dst, src, dst_width * 3);
//         src += src_width * 3;
//         dst += dst_width * 3;
//     }
// }

// };  // namespace

// QImage::QImage(uint32_t width, uint32_t height, uint32_t type)
//         : QImage{nullptr, width, height, type} {}

// QImage::QImage(std::string const& filename, uint32_t width, uint32_t height, uint32_t type) {
//     int32_t result{init(filename, width, height, type)};
//     QCMTHROW_EXCEPTION(result == 0,
//                        "init return {}... filename = {}, width = {}, height = {}, type = {}",
//                        result, filename, width, height, type);
// }

// QImage::QImage(uint8_t* data, uint32_t width, uint32_t height, uint32_t type) {
//     int32_t result{init(data, width, height, type)};
//     QCMTHROW_EXCEPTION(result == 0,
//                        "init return {}... data = {}, width = {}, height = {}, type = {}", result,
//                        static_cast<void*>(data), width, height, type);
// }

// #ifdef WITH_OPENCV
// QImage::QImage(cv::Mat const& image) {
//     int32_t result{init(image.data, image.cols, image.rows, QIMAGE_TYPE_BGR888)};
//     QCMTHROW_EXCEPTION(result == 0, "init return {}... data = {}, width = {}, height = {}", result,
//                        static_cast<void*>(image.data), image.cols, image.rows);
// }
// #endif

// int32_t QImage::init(std::string const& filename, uint32_t width, uint32_t height, uint32_t type) {
//     int32_t result{0};

//     do {
//         switch (type) {
//             case QIMAGE_TYPE_NV12: {
//                 std::shared_ptr<uint8_t> impl;
//                 result = read_nv12(&impl, filename, width, height);
//                 if (result != 0) {
//                     break;
//                 }
//                 __impl = impl;
//                 __width = width;
//                 __height = height;
//                 __type = type;
//                 break;
//             }

//             default: {
//                 result = -1;
//             }
//         }

//     } while (0);

//     return result;
// }

// int32_t QImage::init(uint8_t* data, uint32_t width, uint32_t height, uint32_t type) {
//     int32_t result{0};

//     do {
//         switch (type) {
//             case QIMAGE_TYPE_NV12: {
//                 auto size = width * height * 3 / 2;
//                 auto impl =
//                     std::shared_ptr<uint8_t>{new uint8_t[size], [](uint8_t* p) { delete[] p; }};
//                 if (data != nullptr) {
//                     memcpy(impl.get(), data, size);
//                 }
//                 __impl = impl;
//                 break;
//             }

//             case QIMAGE_TYPE_RGB888:
//             case QIMAGE_TYPE_BGR888: {
//                 auto size = width * height * 3;
//                 auto impl =
//                     std::shared_ptr<uint8_t>{new uint8_t[size], [](uint8_t* p) { delete[] p; }};
//                 if (data != nullptr) {
//                     memcpy(impl.get(), data, size);
//                 }
//                 __impl = impl;
//                 __width = width;
//                 __height = height;
//                 __type = type;
//                 break;
//             }

//             default: {
//                 result = -1;
//             }
//         }

//     } while (0);

//     return result;
// }

// int32_t QImage::convert(uint32_t type) {
//     int32_t result{0};

//     do {
//         switch (type) {
//             case QIMAGE_CVT_HWC2CHW: {
//                 if (this->type() != QIMAGE_TYPE_RGB888 && this->type() != QIMAGE_TYPE_BGR888) {
//                     result = -1;
//                     qError("QImage.convert(): type() is invalid... type() = {}", this->type());
//                     break;
//                 }

//                 auto impl =
//                     std::shared_ptr<uint8_t>{new uint8_t[this->width() * this->height() * 3]};
//                 rgb_hwc2chw(impl.get(), __impl.get(), this->width() * this->height());
//                 __impl = impl;
//                 break;
//             }

//             case QIMAGE_CVT_BGR2RGB: {
//                 if (this->type() != QIMAGE_TYPE_BGR888) {
//                     result = -1;
//                     qError("QImage.convert(): type() is invalid... type() = {}", this->type());
//                     break;
//                 }
//                 cvt_rgb2bgr(__impl.get(), this->width(), this->height());
//                 break;
//             }

//             case QIMAGE_CVT_RGB2BGR: {
//                 if (this->type() != QIMAGE_TYPE_RGB888) {
//                     result = -1;
//                     qError("QImage.convert(): type() is invalid... type() = {}", this->type());
//                     break;
//                 }
//                 cvt_rgb2bgr(__impl.get(), this->width(), this->height());
//                 break;
//             }

//             default: {
//                 result = -1;
//             }
//         }
//     } while (0);

//     return result;
// }

// template <>
// void QResize<QImage, QImage>::operator()(QImage* dst, QImage const& src) {
//     QCMTHROW_EXCEPTION(
//         (src.type() != QImage::QIMAGE_TYPE_RGB888 || dst->type() != QImage::QIMAGE_TYPE_RGB888),
//         "check parameter fail... src: ({},{},{}), dst: ({},{},{})", src.type(), src.width(),
//         src.height(), dst->type(), dst->width(), dst->height());

//     resize(dst->data(), dst->width(), dst->height(), src.data(), src.width(), src.height());
// }

// template <>
// void QCrop<QImage, QImage>::operator()(QImage* dst, QImage const& src, uint32_t x, uint32_t y) {
//     QCMTHROW_EXCEPTION(
//         (src.type() != QImage::QIMAGE_TYPE_RGB888 || dst->type() != QImage::QIMAGE_TYPE_RGB888 ||
//          x + dst->width() > src.width() || y + dst->height() > src.height()),
//         "check parameter fail... src: ({},{},{}), dst: ({},{},{})", src.type(), src.width(),
//         src.height(), dst->type(), dst->width(), dst->height());

//     crop(dst->data(), dst->width(), dst->height(), src.data(), src.width(), src.height(), x, y);
// }

// };  // namespace qlib
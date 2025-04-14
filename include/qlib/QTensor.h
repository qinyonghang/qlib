#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "QException.h"
#include "QImage.h"
#include "QLog.h"

namespace qlib {
template <typename QType = uint8_t>
class QTensor : public QObject {
public:
    QTensor() = default;
    QTensor(std::vector<size_t> const& shape);

    std::vector<size_t> shape() const;

    int32_t init(std::vector<size_t> const& shape);
    std::pair<std::vector<QTensor>, bool> split(int64_t dim = 0);
    std::pair<std::vector<QTensor>, bool> split(size_t split_size = 1, int64_t dim = 0);
    std::pair<std::vector<QTensor>, bool> split(std::vector<size_t> const& split_size,
                                                int64_t dim = 0);

protected:
    QObjectPtr __impl;
};

namespace {

template <typename QType = uint8_t>
struct QTensorImpl : public QObject {
    std::shared_ptr<QType> data;
    std::vector<size_t> shape;
};

};  // namespace

template <typename QType>
QTensor<QType>::QTensor(std::vector<size_t> const& shape) {
    int32_t result{init(shape)};
    if (result != 0) {
        QTHROW_EXCEPTION("QTensor::QTensor(shape): init() return {}... shape = {}", result,
                         serialize_arr(shape));
    }
}

template <typename QType>
std::vector<size_t> QTensor<QType>::shape() const {
    return __impl->shape;
}

template <typename QType>
int32_t QTensor<QType>::init(std::vector<size_t> const& shape) {
    int32_t result{0};

    do {
        auto elem_size = 1u;
        for (auto dim : shape) {
            elem_size *= dim;
        }

        auto data = std::shared_ptr<QType>(new QType[elem_size], std::default_delete<QType[]>());
        __impl = std::make_shared<QTensorImpl<QType>>(data, shape);
    } while (0);

    return result;
}

template <typename QType>
std::pair<std::vector<QTensor<QType>>, bool> QTensor<QType>::split(int64_t dim) {
    std::vector<QTensor<QType>> result;
    bool check{true};

    do {
        if (dim >= __impl->shape.size()) {
            check = false;
            qError("QTensor::split(dim): dim {} >= shape size {}", dim, __impl->shape.size());
            break;
        }

    } while (0);

    return std::make_pair(result, check);
}

};  // namespace qlib
#pragma once

#include <iomanip>
#include <iostream>

#include "QObject.h"

namespace qlib {
template <typename QOutStream>
class QProgressBar : public QObject {
public:
    QProgressBar(size_t total, size_t width, QOutStream& out)
            : __pos(0), __total(total), __width(width), __out(out) {
        update(0u);
    }

    void update(size_t step = 1u) {
        __pos += step;

        auto progress = static_cast<float32_t>(__pos) / __total;
        size_t pos = static_cast<size_t>(progress * __width);

        __out << "[";
        for (auto i = 0u; i < __width; ++i) {
            if (i < pos) {
                __out << "=";
            } else if (i == pos) {
                __out << ">";
            } else {
                __out << "-";
            }
        }
        __out << "] " << static_cast<size_t>(progress * 100.0) << "%\r";

        if (__pos == __total) {
            __out << "\n";
        }

        __out.flush();
    }

protected:
    size_t __pos;
    size_t __total;
    size_t __width;
    QOutStream& __out;
};

class QProgressBarStdStream : public QProgressBar<std::ostream> {
public:
    QProgressBarStdStream(size_t total, size_t width = 50)
            : QProgressBar(total, width, std::cout) {}
};

};  // namespace qlib
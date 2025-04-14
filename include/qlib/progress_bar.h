#pragma once

#include <iomanip>
#include <iostream>

#include "object.h"

namespace qlib {
template <class stream = std::ostream, size_t N = 50u>
class progress_bar {
public:
    template <class = std::enable_if_t<!std::is_same_v<stream, std::ostream>>>
    progress_bar(size_t _total, stream& _out) : total{_total}, out{_out} {
        update(0u);
    }

    template <class = std::enable_if_t<std::is_same_v<stream, std::ostream>>>
    progress_bar(size_t _total, std::ostream& _out = std::cout) : total{_total}, out{_out} {
        update(0u);
    }

    void update(size_t step = 1u) {
        pos += step;

        auto progress = static_cast<float32_t>(pos) / total;
        size_t pos = static_cast<size_t>(progress * N);

        out << "[";
        for (auto i = 0u; i < N; ++i) {
            if (i < pos) {
                out << "=";
            } else if (i == pos) {
                out << ">";
            } else {
                out << "-";
            }
        }
        out << "] " << static_cast<size_t>(progress * 100.0) << "%\r";

        if (pos == total) {
            out << "\n";
        }

        out.flush();
    }

protected:
    size_t pos{0u};
    size_t total;
    stream& out;
};

};  // namespace qlib
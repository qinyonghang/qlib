#pragma once

#include "object.h"

namespace qlib {
template <class stream, size_t N = 50u>
class progress_bar {
public:
    progress_bar(size_t __total, stream& __out) : _total{__total}, _out{__out} { update(0u); }

    void update(size_t step = 1u) {
        _pos += step;

        auto progress = static_cast<float32_t>(_pos) / _total;
        size_t _pos = static_cast<size_t>(progress * N);

        _out << "[";
        for (auto i = 0u; i < N; ++i) {
            if (i < _pos) {
                _out << "=";
            } else if (i == _pos) {
                _out << ">";
            } else {
                _out << "-";
            }
        }
        _out << "] " << static_cast<size_t>(progress * 100.0) << "%\r";

        if (_pos == _total) {
            _out << "\n";
        }

        _out.flush();
    }

protected:
    size_t _pos{0u};
    size_t _total;
    stream& _out;
};

};  // namespace qlib
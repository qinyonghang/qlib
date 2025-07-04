#pragma once

#include <list>
#include <optional>

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {
template <class T>
class smoother final : public object {
public:
    constexpr smoother(uint64_t n) noexcept : _n{n} {}

    template <class... Args>
    void emplace(Args&&... args) {
        if (likely(_buffer.size() > _n)) {
            _buffer.pop_front();
        }

        _buffer.emplace_back(std::forward<Args>(args)...);

        T average = _buffer.front();
        for (auto it = std::next(_buffer.begin()); it != _buffer.end(); ++it) {
            average += *it;
        }
        average /= _buffer.size();
        _average = std::move(average);
    }

    T const& get() const { return _average.value(); }

protected:
    std::list<T> _buffer;
    std::optional<T> _average;
    uint64_t _n{0u};
};
};  // namespace qlib

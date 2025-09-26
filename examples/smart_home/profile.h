#pragma once

#include <chrono>
#include <string>

namespace qlib {
template <class _Callback>
class profile final : public object {
public:
    using time_point = std::chrono::steady_clock::time_point;

    profile() = default;

    template <class _U>
    profile(_U&& __module) : _module{forward<_U>(__module)} {}

    ~profile() {
        auto tp =
            std::chrono::duration_cast<std::chrono::microseconds>(time_point::clock::now() - _tp);
        _Callback("{} cost {} us", _module, tp.count());
    }

protected:
    std::string _module{};
    time_point _tp{time_point::clock::now()};
};

};  // namespace qlib

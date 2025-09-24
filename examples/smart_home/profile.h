#pragma once

#include <chrono>

#include "qlib/string.h"

#include "spdlog/spdlog.h"

namespace qlib {
class profile final : public object {
public:
    using time_point = std::chrono::steady_clock::time_point;

    profile() = default;

    template <class String>
    profile(String&& __module) : _module{std::forward<String>(__module)} {}

    ~profile() {
        auto tp =
            std::chrono::duration_cast<std::chrono::microseconds>(time_point::clock::now() - _tp);
        spdlog::debug("{} cost {} us", _module, tp.count());
    }

protected:
    string_t _module{};
    time_point _tp{time_point::clock::now()};
};

};  // namespace qlib

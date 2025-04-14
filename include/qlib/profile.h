#pragma once

#include <chrono>

#include "logger.h"
#include "object.h"

namespace qlib {
class profile final {
public:
    using time_point = std::chrono::steady_clock::time_point;

    profile() = default;

    template <class String>
    profile(String&& _module) : module{std::forward<String>(_module)} {}

    ~profile() { qDebug("Module({}) Cost: {}!", module, time_point::clock::now() - tp); }

protected:
    string module{};
    time_point tp{time_point::clock::now()};
};

};  // namespace qlib
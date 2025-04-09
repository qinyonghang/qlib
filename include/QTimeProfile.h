#pragma once

#include <chrono>
#include <string>

#include "QLog.h"
#include "QObject.h"

namespace qlib {
class QTimeProfile : public QObject {
public:
    QTimeProfile(std::string const& module = "None")
            : __module{module}, __now{std::chrono::high_resolution_clock::now()} {}
    ~QTimeProfile() {
        auto end = std::chrono::high_resolution_clock::now();
        auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - __now);

        qDebug("Module({}) cost {} ms", __module, cost_ms.count());
    }

protected:
    std::string __module;
    std::chrono::high_resolution_clock::time_point __now;
};

};  // namespace qlib
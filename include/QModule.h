#pragma once

#include "QObject.h"

namespace qlib {
template <typename QParamters, typename QInputs, typename QOutputs>
class QModule : public QObject {
public:
    QModule() = default;
    ~QModule() override = default;

    virtual int init(QParamters const&) = 0;
    virtual int free() = 0;
    virtual int exec(QOutputs*, QInputs const&) = 0;
};
};  // namespace qlib

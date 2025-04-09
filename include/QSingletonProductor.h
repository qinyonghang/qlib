#pragma once

#include "QObject.h"

namespace qlib {
template <typename T>
class QSingletonProductor final : public QObject {
protected:
    T __impl;

    QSingletonProductor() = default;
    QSingletonProductor(QSingletonProductor const&) = default;
    QSingletonProductor(QSingletonProductor&&) = default;
    QSingletonProductor& operator=(QSingletonProductor const&) = default;
    QSingletonProductor& operator=(QSingletonProductor&&) = default;

    template <typename... QArgs>
    QSingletonProductor(QArgs&&... args) : __impl{std::forward<QArgs>(args)...} {}

public:
    static T& get_instance() {
        static QSingletonProductor<T> productor;
        return productor.__impl;
    }

    template <typename... QArgs>
    static T& get_instance(QArgs&&... args) {
        static QSingletonProductor<T> productor{std::forward<QArgs>(args)...};
        return productor.__impl;
    }

    ~QSingletonProductor() = default;
};

};  // namespace qlib
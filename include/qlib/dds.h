#pragma once

#define DDS_IMPLEMENTATION

#include <functional>

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {

namespace dds {

template <class T, class Enable = void>
struct convert;

class publisher final : public object {
public:
    using self = publisher;
    using ptr = sptr<self>;
    using base = object;

    template <class T>
    static ptr make(string_t const& topic) {
        return std::make_shared<self>(topic, std::in_place_type<T>);
    }

    publisher() = default;

    template <class T>
    publisher(string_t const& topic, std::in_place_type_t<T> const& = std::in_place_type<T>) {
        int32_t result{init<T>(topic)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    template <class T>
    int32_t init(string_t const& topic);

    template <class T>
    int32_t publish(T const& value);

protected:
    object::ptr impl{nullptr};
};

class subscriber final : public object {
public:
    using base = object;
    using self = subscriber;
    using ptr = sptr<self>;

    template <class T, class Callback>
    static ptr make(string_t const& topic, Callback&& callback) {
        return std::make_shared<self>(topic, std::forward<Callback>(callback),
                                      std::in_place_type<T>);
    }

    explicit subscriber() = default;

    template <class T, class Callback>
    explicit subscriber(string_t const& topic,
                        Callback&& callback,
                        std::in_place_type_t<T> const& = std::in_place_type<T>) {
        int32_t result{init<T>(topic, std::forward<Callback>(callback))};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    template <class T>
    int32_t init(string_t const& topic, std::function<void(T&&)> const& callback);

protected:
    object::ptr impl;
};

};  // namespace dds
};  // namespace qlib
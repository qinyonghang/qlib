#pragma once

#define DDS_IMPLEMENTATION

#include <any>
#include <functional>
#include <vector>

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {

namespace dds {

struct convert;

class type : public object {
public:
    using self = type;
    using ptr = std::shared_ptr<self>;
    using base = object;

    virtual ~type() = default;

protected:
    std::any value;

    friend struct convert;

    template <class... Args>
    type(Args&&... args) : value{std::forward<Args>(args)...} {}
};

#define REGISTER_TYPE(T)                                                                           \
    class T : public type {                                                                        \
    public:                                                                                        \
        using self = T;                                                                            \
        using ptr = sptr<self>;                                                                    \
        using base = type;                                                                         \
        using value_type = T##_t;                                                                  \
                                                                                                   \
        template <class... Args>                                                                   \
        static ptr make(Args&&... args) {                                                          \
            return std::make_shared<self>(std::forward<Args>(args)...);                            \
        }                                                                                          \
                                                                                                   \
        template <class... Args>                                                                   \
        T(Args&&... args) : base{value_type{std::forward<Args>(args)...}} {}                       \
                                                                                                   \
        self& operator=(value_type const& value) {                                                 \
            base::value = value;                                                                   \
            return *this;                                                                          \
        }                                                                                          \
        operator value_type() const {                                                              \
            return std::any_cast<value_type>(base::value);                                         \
        }                                                                                          \
                                                                                                   \
        auto to_string() const {                                                                   \
            return static_cast<value_type>(*this);                                                 \
        }                                                                                          \
    };

REGISTER_TYPE(int8)
REGISTER_TYPE(uint8)
REGISTER_TYPE(int16)
REGISTER_TYPE(uint16)
REGISTER_TYPE(int32)
REGISTER_TYPE(uint32)
REGISTER_TYPE(int64)
REGISTER_TYPE(uint64)
REGISTER_TYPE(float32)
REGISTER_TYPE(float64)
REGISTER_TYPE(string)

#undef REGISTER_TYPE

template <class T>
class sequence final : public type {
public:
    using self = sequence<T>;
    using ptr = sptr<self>;
    using base = type;
    using value_type = std::vector<T>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    template <class... Args>
    sequence(Args&&... args) : base{value_type{std::forward<Args>(args)...}} {}

    self& operator=(value_type const& value) {
        base::value = value;
        return *this;
    }

    operator value_type() const { return std::any_cast<value_type>(base::value); }

    auto to_string() const { return static_cast<value_type>(*this); }
};

class publisher : public object {
public:
    using self = publisher;
    using ptr = std::shared_ptr<self>;
    using base = object;

    static ptr make(type::ptr const& type_ptr, string_t const& topic) {
        return std::make_shared<self>(type_ptr, topic);
    }

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

    publisher(type::ptr const& type_ptr, string_t const& topic) {
        int32_t result{init(type_ptr, topic)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    template <class T>
    int32_t init(string_t const& topic) {
        return init(T::make(), topic);
    }

    int32_t init(type::ptr const& type_ptr, string_t const& topic);

    template <class T, class = std::enable_if_t<!std::is_convertible_v<std::decay_t<T>, type::ptr>>>
    int32_t publish(T const& value);
    int32_t publish(type::ptr const& type_ptr);

protected:
    object::ptr impl{nullptr};
};

class subscriber : public object {
public:
    using base = object;
    using self = subscriber;
    using ptr = std::shared_ptr<self>;

    static ptr make(type::ptr const& type_ptr,
                    string_t const& topic,
                    std::function<void(type::ptr const&)> const& callback) {
        return std::make_shared<self>(type_ptr, topic, callback);
    }

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

    subscriber(type::ptr const& type_ptr,
               string_t const& topic,
               std::function<void(type::ptr const&)> const& callback) {
        int32_t result{init(type_ptr, topic, callback)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    template <class T, class = std::enable_if_t<!std::is_convertible_v<std::decay_t<T>, type::ptr>>>
    int32_t init(string_t const& topic, std::function<void(T&&)> const& callback);

    template <class T, class = std::void_t<typename T::ptr>>
    int32_t init(string_t const& topic,
                 std::function<void(typename T::ptr const&)> const& callback) {
        return init(T::make(), topic, [callback](type::ptr const& type_ptr) {
            auto ptr = std::dynamic_pointer_cast<T>(type_ptr);
            THROW_EXCEPTION(ptr, "type is {}!", typeid(*type_ptr).name());
            callback(ptr);
        });
    }

    int32_t init(type::ptr const& type_ptr,
                 string_t const& topic,
                 std::function<void(type::ptr const&)> const& callback);

protected:
    object::ptr impl;
};

template <>
inline int32_t publisher::init<string_t>(string_t const& topic) {
    return init<string>(topic);
}

template <>
inline int32_t publisher::publish<string_t>(string_t const& value) {
    return publish(string::make(value));
}

template <>
inline int32_t subscriber::init<string_t>(string_t const& topic,
                                          std::function<void(string_t&&)> const& callback) {
    return init(string::make(), topic, [callback](type::ptr const& type_ptr) {
        auto value_ptr = std::dynamic_pointer_cast<string>(type_ptr);
        THROW_EXCEPTION(value_ptr, "value is {}!", typeid(*type_ptr).name());
        auto value = static_cast<string_t>(*value_ptr);
        callback(std::move(value));
    });
}

#define REGISTER_TYPE(T)                                                                           \
    template <>                                                                                    \
    inline int32_t publisher::init<T##_t>(string_t const& topic) {                                 \
        return init<T>(topic);                                                                     \
    }                                                                                              \
    template <>                                                                                    \
    inline int32_t publisher::init<std::vector<T##_t>>(string_t const& topic) {                    \
        return init<sequence<T##_t>>(topic);                                                       \
    }                                                                                              \
    template <>                                                                                    \
    inline int32_t publisher::publish<T##_t>(T##_t const& value) {                                 \
        return publish(T::make(value));                                                            \
    }                                                                                              \
    template <>                                                                                    \
    inline int32_t publisher::publish<std::vector<T##_t>>(std::vector<T##_t> const& value) {       \
        return publish(sequence<T##_t>::make(value));                                              \
    }                                                                                              \
    template <>                                                                                    \
    inline int32_t subscriber::init<T##_t>(string_t const& topic,                                  \
                                           std::function<void(T##_t&&)> const& callback) {         \
        return init(T::make(), topic, [callback](type::ptr const& type_ptr) {                      \
            auto value_ptr = std::dynamic_pointer_cast<T>(type_ptr);                               \
            THROW_EXCEPTION(value_ptr, "value is {}!", typeid(*type_ptr).name());                  \
            auto value = static_cast<T##_t>(*value_ptr);                                           \
            callback(std::move(value));                                                            \
        });                                                                                        \
    }                                                                                              \
    template <>                                                                                    \
    inline int32_t subscriber::init<std::vector<T##_t>>(                                           \
        string_t const& topic, std::function<void(std::vector<T##_t>&&)> const& callback) {        \
        return init(sequence<T##_t>::make(), topic, [callback](type::ptr const& type_ptr) {        \
            auto value_ptr = std::dynamic_pointer_cast<sequence<T##_t>>(type_ptr);                 \
            THROW_EXCEPTION(value_ptr, "value is {}!", typeid(*type_ptr).name());                  \
            auto value = static_cast<std::vector<T##_t>>(*value_ptr);                              \
            callback(std::move(value));                                                            \
        });                                                                                        \
    }

REGISTER_TYPE(int8)
REGISTER_TYPE(uint8)
REGISTER_TYPE(int16)
REGISTER_TYPE(uint16)
REGISTER_TYPE(int32)
REGISTER_TYPE(uint32)
REGISTER_TYPE(int64)
REGISTER_TYPE(uint64)
REGISTER_TYPE(float32)
REGISTER_TYPE(float64)

#undef REGISTER_TYPE

};  // namespace dds
};  // namespace qlib
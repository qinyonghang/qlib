#ifdef WIN32
#include <Windows.h>
#endif

#include <iostream>

#include "boost/python.hpp"
#include "boost/python/overloads.hpp"
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"
#include "qlib/dds.h"

namespace py = boost::python;

namespace qlib {

class id : public object {
public:
    enum class value_type : int32_t {
        unknown = -1,
        string = 0,
        int8,
        int16,
        int32,
        int64,
        uint8,
        uint16,
        uint32,
        uint64,
        float32,
        float64,
        sequence
    };

    using self = id;

    id(value_type value = value_type::unknown) : impl{value} {}

    bool operator==(self const& other) { return this->impl == other.impl; }

    bool operator!=(self const& other) { return !this->operator==(other); }

    auto to_string() const { return static_cast<int32_t>(this->impl); }

    template <class T>
    static id make();

protected:
    value_type impl{value_type::unknown};
};

const id id_sequence{id::value_type::sequence};

#define REGISTER_TYPE(T)                                                                           \
    template <>                                                                                    \
    id id::make<T##_t>() {                                                                         \
        return id{id::value_type::T};                                                              \
    }                                                                                              \
    const id id_##T{id::value_type::T};
REGISTER_TYPE(string)
REGISTER_TYPE(int8)
REGISTER_TYPE(int16)
REGISTER_TYPE(int32)
REGISTER_TYPE(int64)
REGISTER_TYPE(uint8)
REGISTER_TYPE(uint16)
REGISTER_TYPE(uint32)
REGISTER_TYPE(uint64)
REGISTER_TYPE(float32)
REGISTER_TYPE(float64)
#undef REGISTER_TYPE

class abstract_type : public object {
public:
    using self = abstract_type;
    using ptr = sptr<self>;

    virtual qlib::id id() const = 0;
};

template <class T>
class type : public abstract_type {
public:
    using base = abstract_type;
    using self = type<T>;
    using ptr = sptr<self>;
    using value_type = T;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    type() = default;

    template <class... Args>
    type(Args&&... args) : value{std::forward<Args>(args)...} {}

    T get() const { return this->value; }
    void set(T const& value) { this->value = value; }

    qlib::id id() const override { return qlib::id::make<T>(); }

    // static qlib::id id() { return qlib::id::make<T>(); }

protected:
    T value;
};

using string = type<string_t>;
using int8 = type<int8_t>;
using int16 = type<int16_t>;
using int32 = type<int32_t>;
using int64 = type<int64_t>;
using uint8 = type<uint8_t>;
using uint16 = type<uint16_t>;
using uint32 = type<uint32_t>;
using uint64 = type<uint64_t>;
using float32 = type<float32_t>;
using float64 = type<float64_t>;

class sequence : public abstract_type {
public:
    using base = abstract_type;
    using self = sequence;
    using ptr = std::shared_ptr<self>;

    template <class Vec>
    static ptr make(Vec&& vec) {
        return std::make_shared<self>(std::forward<Vec>(vec),
                                      std::in_place_type<typename Vec::value_type>);
    }

    sequence(abstract_type::ptr const& type) {
        if (type->id() == id_string) {
            THROW_EXCEPTION(False, "TypeError!");
        }

#define REGISTER_TYPE(T)                                                                           \
    else if (type->id() == id_##T) {                                                               \
        _obj = vector<T##_t>::make();                                                              \
    }
        REGISTER_TYPE(int8)
        REGISTER_TYPE(int16)
        REGISTER_TYPE(int32)
        REGISTER_TYPE(int64)
        REGISTER_TYPE(uint8)
        REGISTER_TYPE(uint16)
        REGISTER_TYPE(uint32)
        REGISTER_TYPE(uint64)
        REGISTER_TYPE(float32)
        REGISTER_TYPE(float64)
#undef REGISTER_TYPE
        else {
            THROW_EXCEPTION(False, "TypeError!");
        }

        self::_value_id = type->id();
    }

    template <class T>
    sequence(std::vector<T> const& vec, std::in_place_type_t<T> const& = std::in_place_type<T>) {
        self::_obj = vector<T>::make(vec);
        self::_value_id = id::make<T>();
    }

    void append(py::api::object obj) { return self::_obj->append(obj); }

    size_t size() const { return self::_obj->size(); }

    void set_item(size_t index, py::api::object obj) { self::_obj->set_item(index, obj); }

    py::api::object get_item(size_t index) const { return self::_obj->get_item(index); }

    qlib::id id() const override { return qlib::id{qlib::id::value_type::sequence}; }

    qlib::id value_id() const { return self::_value_id; }

    template <class T>
    std::vector<T> const& to() const;

protected:
    class object : public qlib::object {
    public:
        using self = object;
        using ptr = std::shared_ptr<self>;

        virtual void append(py::api::object) = 0;
        virtual size_t size() const = 0;
        virtual void set_item(size_t index, py::api::object obj) = 0;
        virtual py::api::object get_item(size_t index) const = 0;
    };

    template <class T>
    class vector : public object {
    public:
        using base = object;
        using self = vector<T>;
        using ptr = std::shared_ptr<self>;
        using value_type = T;

        template <class... Args>
        static ptr make(Args&&... args) {
            return std::make_shared<self>(std::forward<Args>(args)...);
        }

        vector() = default;

        template <class Vector>
        vector(Vector&& vec) : data{std::forward<Vector>(vec)} {}

        void append(py::api::object obj) {
            py::extract<T> elem(obj);
            THROW_EXCEPTION(elem.check(), "Can't convert to {}", typeid(T).name());
            self::data.emplace_back(elem());
        }

        size_t size() const { return self::data.size(); }

        void set_item(size_t index, py::api::object obj) {
            py::extract<T> elem(obj);
            THROW_EXCEPTION(elem.check(), "Can't convert to {}", typeid(T).name());
            self::data.at(index) = elem();
        }

        py::api::object get_item(size_t index) const {
            return py::api::object(self::data.at(index));
        }

        std::vector<T> const& to() const { return self::data; }

    protected:
        std::vector<T> data;
    };

    qlib::id _value_id;
    self::object::ptr _obj{nullptr};
};  // namespace dds

template <>
std::vector<int8_t> const& sequence::to<int8_t>() const {
    return std::static_pointer_cast<vector<int8_t>>(_obj)->to();
}

template <>
std::vector<int16_t> const& sequence::to<int16_t>() const {
    return std::static_pointer_cast<vector<int16_t>>(_obj)->to();
}

template <>
std::vector<int32_t> const& sequence::to<int32_t>() const {
    return std::static_pointer_cast<vector<int32_t>>(_obj)->to();
}

template <>
std::vector<int64_t> const& sequence::to<int64_t>() const {
    return std::static_pointer_cast<vector<int64_t>>(_obj)->to();
}

template <>
std::vector<uint8_t> const& sequence::to<uint8_t>() const {
    return std::static_pointer_cast<vector<uint8_t>>(_obj)->to();
}

template <>
std::vector<uint16_t> const& sequence::to<uint16_t>() const {
    return std::static_pointer_cast<vector<uint16_t>>(_obj)->to();
}

template <>
std::vector<uint32_t> const& sequence::to<uint32_t>() const {
    return std::static_pointer_cast<vector<uint32_t>>(_obj)->to();
}

template <>
std::vector<uint64_t> const& sequence::to<uint64_t>() const {
    return std::static_pointer_cast<vector<uint64_t>>(_obj)->to();
}

template <>
std::vector<float32_t> const& sequence::to<float32_t>() const {
    return std::static_pointer_cast<vector<float32_t>>(_obj)->to();
}

template <>
std::vector<float64_t> const& sequence::to<float64_t>() const {
    return std::static_pointer_cast<vector<float64_t>>(_obj)->to();
}

class publisher : public object {
public:
    publisher(abstract_type::ptr const& type, string_t const& topic) {
        if (type->id() == id_string) {
            impl = dds::publisher::make<string_t>(topic);
        }
#define REGISTER_TYPE(T)                                                                           \
    else if (type->id() == id_##T) {                                                               \
        impl = dds::publisher::make<T##_t>(topic);                                                 \
    }
        REGISTER_TYPE(int8)
        REGISTER_TYPE(int16)
        REGISTER_TYPE(int32)
        REGISTER_TYPE(int64)
        REGISTER_TYPE(uint8)
        REGISTER_TYPE(uint16)
        REGISTER_TYPE(uint32)
        REGISTER_TYPE(uint64)
        REGISTER_TYPE(float32)
        REGISTER_TYPE(float64)

#undef REGISTER_TYPE
        else if (type->id() == id_sequence) {
            auto seq = std::dynamic_pointer_cast<sequence>(type);
            if (seq->value_id() == id_string) {
                THROW_EXCEPTION(False, "Sequence of string is not supported");
            }
#define REGISTER_TYPE(T)                                                                           \
    else if (seq->value_id() == id_##T) {                                                          \
        impl = dds::publisher::make<std::vector<T##_t>>(topic);                                    \
    }
            REGISTER_TYPE(int8)
            REGISTER_TYPE(int16)
            REGISTER_TYPE(int32)
            REGISTER_TYPE(int64)
            REGISTER_TYPE(uint8)
            REGISTER_TYPE(uint16)
            REGISTER_TYPE(uint32)
            REGISTER_TYPE(uint64)
            REGISTER_TYPE(float32)
            REGISTER_TYPE(float64)

#undef REGISTER_TYPE
            else {
                THROW_EXCEPTION(False, "unknown type");
            }
        }
        else {
            THROW_EXCEPTION(false, "unsupported type!");
        }

        _id = type->id();
    }

    int32_t publish(abstract_type::ptr const& type) {
        int32_t result{0};

        do {
            if (type->id() != _id) {
                std::cout << fmt::format("type mismatch: expected {}, got {}", _id.to_string(),
                                         type->id().to_string())
                          << std::endl;
                result = -1;
                break;
            }

            if (auto _type = std::dynamic_pointer_cast<string>(type); _type != nullptr) {
                result = impl->publish(_type->get());
            }
#define REGISTER_TYPE(T)                                                                           \
    else if (auto _type = std::dynamic_pointer_cast<T>(type); _type != nullptr) {                  \
        result = impl->publish(_type->get());                                                      \
    }
            REGISTER_TYPE(int8)
            REGISTER_TYPE(int16)
            REGISTER_TYPE(int32)
            REGISTER_TYPE(int64)
            REGISTER_TYPE(uint8)
            REGISTER_TYPE(uint16)
            REGISTER_TYPE(uint32)
            REGISTER_TYPE(uint64)
            REGISTER_TYPE(float32)
            REGISTER_TYPE(float64)
#undef REGISTER_TYPE
            else if (auto _type = std::dynamic_pointer_cast<sequence>(type); _type != nullptr) {
                if (_type->value_id() == id_string) {
                    std::cout << "TypeError!" << std::endl;
                    result = -1;
                    break;
                }
#define REGISTER_TYPE(T)                                                                           \
    else if (_type->value_id() == id_##T) {                                                        \
        result = impl->publish(_type->to<T##_t>());                                                \
    }
                REGISTER_TYPE(int8)
                REGISTER_TYPE(int16)
                REGISTER_TYPE(int32)
                REGISTER_TYPE(int64)
                REGISTER_TYPE(uint8)
                REGISTER_TYPE(uint16)
                REGISTER_TYPE(uint32)
                REGISTER_TYPE(uint64)
                REGISTER_TYPE(float32)
                REGISTER_TYPE(float64)

#undef REGISTER_TYPE
            }
            else {
                std::cout << "TypeError!" << std::endl;
            }
        } while (false);

        return result;
    }

protected:
    qlib::id _id;
    dds::publisher::ptr impl;
};

class subscriber : public object {
public:
    subscriber(abstract_type::ptr const& type,
               string_t const& topic,
               std::function<void(abstract_type::ptr const&)> const& callback) {
        if (auto _type = std::dynamic_pointer_cast<string>(type); _type != nullptr) {
            impl = dds::subscriber::make<string_t>(topic, [callback](string_t&& value) {
                auto state = PyGILState_Ensure();
                if (likely(callback != nullptr)) {
                    callback(string::make(std::move(value)));
                }
                PyGILState_Release(state);
            });
        }
#define REGISTER_TYPE(T)                                                                           \
    else if (auto _type = std::dynamic_pointer_cast<T>(type); _type != nullptr) {                  \
        impl = dds::subscriber::make<T##_t>(topic, [callback](T##_t&& value) {                     \
            auto state = PyGILState_Ensure();                                                      \
            if (likely(callback != nullptr)) {                                                     \
                callback(T::make(std::move(value)));                                               \
            }                                                                                      \
            PyGILState_Release(state);                                                             \
        });                                                                                        \
    }

        REGISTER_TYPE(int8)
        REGISTER_TYPE(int16)
        REGISTER_TYPE(int32)
        REGISTER_TYPE(int64)
        REGISTER_TYPE(uint8)
        REGISTER_TYPE(uint16)
        REGISTER_TYPE(uint32)
        REGISTER_TYPE(uint64)
        REGISTER_TYPE(float32)
        REGISTER_TYPE(float64)
#undef REGISTER_TYPE
        else if (auto _type = std::dynamic_pointer_cast<sequence>(type); _type != nullptr) {
            if (_type->value_id() == id_string) {
                THROW_EXCEPTION(false, "TypeError!");
            }
#define REGISTER_TYPE(T)                                                                           \
    else if (_type->value_id() == id_##T) {                                                        \
        impl = dds::subscriber::make<std::vector<T##_t>>(                                          \
            topic, [callback](std::vector<T##_t>&& value) {                                        \
                auto state = PyGILState_Ensure();                                                  \
                if (likely(callback != nullptr)) {                                                 \
                    callback(sequence::make(std::move(value)));                                    \
                }                                                                                  \
                PyGILState_Release(state);                                                         \
            });                                                                                    \
    }

            REGISTER_TYPE(int8)
            REGISTER_TYPE(int16)
            REGISTER_TYPE(int32)
            REGISTER_TYPE(int64)
            REGISTER_TYPE(uint8)
            REGISTER_TYPE(uint16)
            REGISTER_TYPE(uint32)
            REGISTER_TYPE(uint64)
            REGISTER_TYPE(float32)
            REGISTER_TYPE(float64)
#undef REGISTER_TYPE
            else {
                THROW_EXCEPTION(False, "TypeError!");
            }
        }
        else {
            THROW_EXCEPTION(false, "unsupported type!");
        }
    }

protected:
    dds::subscriber::ptr impl;
};

};  // namespace qlib

BOOST_PYTHON_MODULE(qlib) {
#ifdef WIN32
    SetConsoleOutputCP(65001);
#endif

    py::scope scope(py::object(py::handle<>(py::borrowed(PyImport_AddModule("qlib.dds")))));

    py::register_ptr_to_python<qlib::abstract_type::ptr>();
    py::implicitly_convertible<qlib::abstract_type*, qlib::abstract_type::ptr>();
    py::class_<qlib::abstract_type, boost::noncopyable>("type", py::no_init);

#define REGISTER_TYPE(T)                                                                           \
    py::class_<qlib::T, py::bases<qlib::abstract_type>, boost::noncopyable>(#T, py::init<>())      \
        .def("set", &qlib::T::set)                                                                 \
        .def("get", &qlib::T::get);

    REGISTER_TYPE(int8)
    REGISTER_TYPE(int16)
    REGISTER_TYPE(int32)
    REGISTER_TYPE(int64)
    REGISTER_TYPE(uint8)
    REGISTER_TYPE(uint16)
    REGISTER_TYPE(uint32)
    REGISTER_TYPE(uint64)
    REGISTER_TYPE(float32)
    REGISTER_TYPE(float64)
    REGISTER_TYPE(string)

#undef REGISTER_TYPE

    py::class_<qlib::sequence, py::bases<qlib::abstract_type>, boost::noncopyable>(
        "sequence", py::init<qlib::abstract_type::ptr>())
        .def("__len__", &qlib::sequence::size)
        .def("__setitem__", &qlib::sequence::set_item)
        .def("__getitem__", &qlib::sequence::get_item)
        .def("append", &qlib::sequence::append);

    py::class_<qlib::publisher, boost::noncopyable>(
        "publisher", py::init<qlib::abstract_type::ptr, std::string>())
        .def("publish", &qlib::publisher::publish);

    py::class_<qlib::subscriber, boost::noncopyable>(
        "subscriber", py::init<qlib::abstract_type::ptr, std::string, py::object>());
};

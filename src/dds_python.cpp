#ifdef WIN32
#include <Windows.h>
#endif

#include "boost/python.hpp"
#include "boost/python/overloads.hpp"
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"
#include "qlib/dds.h"

namespace py = boost::python;

namespace qlib {

namespace dds {

class abstract_type : public qlib::object {
public:
    using self = abstract_type;
    using ptr = std::shared_ptr<self>;

    virtual type::ptr make() = 0;
};

template <class T>
class type_wrapper : public abstract_type {
public:
    using base = abstract_type;
    using self = type_wrapper<T>;
    using ptr = std::shared_ptr<self>;
    using value_type = T;

    void set(value_type value) { self::value = std::move(value); }
    value_type get() { return self::value; }

    type_wrapper() = default;

    type::ptr make() override;

    template <class _T,
              class = std::enable_if_t<std::is_same_v<value_type, typename _T::value_type>>>
    static ptr make(typename _T::ptr const& value_ptr) {
        ptr result = std::make_shared<self>();
        result->value = static_cast<value_type>(*value_ptr);
        return result;
    }

protected:
    value_type value;
};

#define REGISTER_TYPE(T)                                                                           \
    template <>                                                                                    \
    type::ptr type_wrapper<qlib::T##_t>::make() {                                                  \
        return dds::T::make(self::value);                                                          \
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
REGISTER_TYPE(string)

#undef REGISTER_TYPE

class sequence_wrapper : public abstract_type {
public:
    using base = abstract_type;
    using self = sequence_wrapper;
    using ptr = std::shared_ptr<self>;

    sequence_wrapper(abstract_type::ptr const& type_ptr) {
#define REGISTER_TYPE(T)                                                                           \
    if (auto _type_ptr = std::dynamic_pointer_cast<type_wrapper<qlib::T##_t>>(type_ptr);           \
        _type_ptr != nullptr) {                                                                    \
        self::obj = self::vector<qlib::T##_t>::make();                                             \
        self::type_id = self::T;                                                                   \
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

        THROW_EXCEPTION(obj, "Unknown Type!");
    }

    void append(py::api::object obj) { return self::obj->append(obj); }

    size_t size() const { return self::obj->size(); }

    void set_item(size_t index, py::api::object obj) { self::obj->set_item(index, obj); }

    py::api::object get_item(size_t index) const { return self::obj->get_item(index); }

    type::ptr make() override {
        type::ptr result{nullptr};

#define REGISTER_TYPE(T)                                                                           \
    if (self::type_id == T) {                                                                      \
        result = sequence<qlib::T##_t>::make(                                                      \
            std::static_pointer_cast<self::vector<qlib::T##_t>>(obj)->data);                       \
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

        THROW_EXCEPTION(result, "Unknown Type!");

        return result;
    }

    template <class T>
    sequence_wrapper(typename dds::sequence<T>::ptr const& value_ptr, std::in_place_type_t<T>) {
        auto obj = self::vector<T>::make();
        obj->data = static_cast<typename dds::sequence<T>::value_type>(*value_ptr);
        self::obj = obj;
        self::type_id = 0xff;
    }

    template <class T>
    static ptr make(typename dds::sequence<T>::ptr const& value_ptr) {
        return std::make_shared<self>(value_ptr, std::in_place_type<T>);
    }

protected:
    enum : uint32_t {
        int8 = 0,
        int16,
        int32,
        int64,
        uint8,
        uint16,
        uint32,
        uint64,
        float32,
        float64,
    };

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

        std::vector<T> data;
    };

    uint32_t type_id{};
    self::object::ptr obj{nullptr};
};  // namespace dds

class publisher_wrapper : public publisher {
public:
    using base = publisher;
    using self = publisher_wrapper;

    publisher_wrapper(abstract_type::ptr const& type, qlib::string const& topic)
            : base{type->make(), topic} {}

    int32_t publish(abstract_type::ptr const& type) { return base::publish(type->make()); }
};

class subscriber_wrapper : public subscriber {
public:
    using base = subscriber;
    using self = subscriber_wrapper;

    subscriber_wrapper(abstract_type::ptr const& type,
                       qlib::string const& topic,
                       std::function<void(abstract_type::ptr const&)> const& callback)
            : base{type->make(), topic, [callback](type::ptr const& type_ptr) {
                       PyGILState_STATE gstate = PyGILState_Ensure();
                       if (qlib::likely(callback != nullptr)) {
                           abstract_type::ptr result{nullptr};
#define REGISTER_TYPE(T)                                                                           \
    if (auto _type_ptr = std::dynamic_pointer_cast<dds::T>(type_ptr); _type_ptr != nullptr) {      \
        result = type_wrapper<qlib::T##_t>::make<dds::T>(_type_ptr);                               \
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
                           REGISTER_TYPE(string)
#undef REGISTER_TYPE

#define REGISTER_TYPE(T)                                                                           \
    if (auto _type_ptr = std::dynamic_pointer_cast<dds::sequence<qlib::T##_t>>(type_ptr);          \
        _type_ptr != nullptr) {                                                                    \
        result = sequence_wrapper::make<qlib::T##_t>(_type_ptr);                                   \
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

                           THROW_EXCEPTION(result, "unknown type");
                           callback(result);
                       }
                       PyGILState_Release(gstate);
                   }} {
    }
};

};  // namespace dds
};  // namespace qlib

BOOST_PYTHON_MODULE(qlib) {
#ifdef WIN32
    SetConsoleOutputCP(65001);
#endif

    py::scope scope(py::object(py::handle<>(py::borrowed(PyImport_AddModule("qlib.dds")))));
    using namespace qlib::dds;

    py::register_ptr_to_python<abstract_type::ptr>();
    py::implicitly_convertible<abstract_type*, abstract_type::ptr>();
    py::class_<abstract_type, boost::noncopyable>("type", py::no_init);

#define REGISTER_TYPE(T)                                                                           \
    py::class_<type_wrapper<qlib::T##_t>, py::bases<abstract_type>, boost::noncopyable>(           \
        #T, py::init<>())                                                                          \
        .def("set", &type_wrapper<qlib::T##_t>::set)                                               \
        .def("get", &type_wrapper<qlib::T##_t>::get);

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

    py::class_<sequence_wrapper, py::bases<abstract_type>, boost::noncopyable>(
        "sequence", py::init<abstract_type::ptr>())
        .def("__len__", &sequence_wrapper::size)
        .def("__setitem__", &sequence_wrapper::set_item)
        .def("__getitem__", &sequence_wrapper::get_item)
        .def("append", &sequence_wrapper::append);

    // py::class_<std::vector<uint8_t>>("vector_uint8")
    //     .def(py::vector_indexing_suite<std::vector<uint8_t>>());
    // py::class_<std::vector<uint32_t>>("vector_uint32")
    //     .def(py::vector_indexing_suite<std::vector<uint32_t>>());

    py::class_<publisher_wrapper, boost::noncopyable>("publisher",
                                                      py::init<abstract_type::ptr, std::string>())
        .def("publish", &publisher_wrapper::publish);

    py::class_<subscriber_wrapper, boost::noncopyable>(
        "subscriber", py::init<abstract_type::ptr, std::string, py::object>());
};

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
class string_wrapper : public string {
public:
    string_wrapper() : string() {}

    void set(std::string const& value) { string::operator=(value); }

    std::string get() { return static_cast<std::string>(*this); }
};

class sequence_wrapper : public type {
public:
    using base = type;
    using self = sequence_wrapper;
    using ptr = std::shared_ptr<self>;

    sequence_wrapper(type::ptr const& type_ptr) {
        constexpr auto N = static_cast<uint32_t>(LENGTH_UNLIMITED);

#define REGISTER_TYPE(T)                                                                           \
    if (std::dynamic_pointer_cast<T>(type_ptr) != nullptr) {                                       \
        base::_type = {base::create([N](DynamicTypeBuilderFactory::_ref_type type_factory) {       \
            return type_factory->create_sequence_type(base::make<qlib::T##_t>(type_factory), N)    \
                ->build();                                                                         \
        })};                                                                                       \
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

        THROW_EXCEPTION(base::_type, "Invalid type!");
        auto data_factory = DynamicDataFactory::get_instance();
        base::_data = data_factory->create_data(base::_type);
    }

#define REGISTER_TYPE(T)                                                                           \
    void set_##T##s(std::vector<qlib::T##_t> const& value) { base::set(value); }                   \
    std::vector<qlib::T##_t> get_##T##s() { return base::get<std::vector<qlib::T##_t>>(); }

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
};

class float32_wrapper : public float32 {
public:
    float32_wrapper() : float32() {}

    void set(float value) { float32::operator=(value); }
    float get() { return static_cast<float>(*this); }
};

class float64_wrapper : public float64 {
public:
    float64_wrapper() : float64() {}

    void set(double value) { float64::operator=(value); }
    double get() { return static_cast<double>(*this); }
};

class publisher_wrapper : public publisher {
public:
    publisher_wrapper(type::ptr const& type_ptr, std::string const& topic)
            : publisher(type_ptr, topic) {}

    int32_t publish(type::ptr const& type_ptr) { return publisher::publish(type_ptr); }
};

class subscriber_wrapper : public subscriber {
public:
    subscriber_wrapper(type::ptr const& type_ptr,
                       std::string const& topic,
                       std::function<void(type::ptr const&)> const& callback)
            : subscriber(type_ptr, topic, callback) {}
};

#define REGISTER_TYPE(T)                                                                           \
    class T##_wrapper : public T {                                                                 \
    public:                                                                                        \
        T##_wrapper() : T() {}                                                                     \
        void set(long value) {                                                                     \
            if (value > static_cast<long>(std::numeric_limits<T::value_type>::max())) {            \
                std::cout << "Warning: value " << value << " is out of range for type " << #T      \
                          << std::endl;                                                            \
                value = static_cast<long>(std::numeric_limits<T::value_type>::max());              \
            }                                                                                      \
            if (value < static_cast<long>(std::numeric_limits<T::value_type>::min())) {            \
                std::cout << "Warning: value " << value << " is out of range for type " << #T      \
                          << std::endl;                                                            \
                value = static_cast<long>(std::numeric_limits<T::value_type>::min());              \
            }                                                                                      \
            T::operator=(static_cast<T::value_type>(value));                                       \
        }                                                                                          \
        long get() {                                                                               \
            return static_cast<T::value_type>(*this);                                              \
        }                                                                                          \
    };                                                                                             \
    class vector_##T##_wrapper : public std::vector<qlib::T##_t> {                                 \
    public:                                                                                        \
        vector_##T##_wrapper() = default;                                                          \
        void set(size_t i, qlib::T##_t value) {                                                    \
            this->at(i) = value;                                                                   \
        }                                                                                          \
        qlib::T##_t get(size_t i) const {                                                          \
            return this->at(i);                                                                    \
        }                                                                                          \
        size_t size() const {                                                                      \
            return this->size();                                                                   \
        }                                                                                          \
    };

REGISTER_TYPE(int8)
REGISTER_TYPE(int16)
REGISTER_TYPE(int32)
REGISTER_TYPE(int64)
REGISTER_TYPE(uint8)
REGISTER_TYPE(uint16)
REGISTER_TYPE(uint32)
REGISTER_TYPE(uint64)

#undef REGISTER_TYPE

class vector_float32_wrapper : public std::vector<qlib::float32_t> {
public:
    vector_float32_wrapper() = default;
    void set(size_t i, qlib::float32_t value) { this->at(i) = value; }
    qlib::float32_t get(size_t i) const { return this->at(i); }
    size_t size() const { return this->size(); }
};

class vector_float64_wrapper : public std::vector<qlib::float64_t> {
public:
    vector_float64_wrapper() = default;
    void set(size_t i, qlib::float64_t value) { this->at(i) = value; }
    qlib::float64_t get(size_t i) const { return this->at(i); }
    size_t size() const { return this->size(); }
};

};  // namespace dds
};  // namespace qlib

BOOST_PYTHON_MODULE(qlib) {
#ifdef WIN32
    SetConsoleOutputCP(65001);
#endif

    py::register_ptr_to_python<qlib::dds::type::ptr>();
    py::implicitly_convertible<qlib::dds::type*, qlib::dds::type::ptr>();

    py::scope scope(py::object(py::handle<>(py::borrowed(PyImport_AddModule("qlib.dds")))));

    py::class_<qlib::dds::type, boost::noncopyable>("type", py::no_init);

    py::class_<qlib::dds::string_wrapper, py::bases<qlib::dds::type>, boost::noncopyable>(
        "string", py::init<>())
        .def("set", &qlib::dds::string_wrapper::set)
        .def("get", &qlib::dds::string_wrapper::get);

#define REGISTER_TYPE(T)                                                                           \
    py::class_<qlib::dds::T##_wrapper, py::bases<qlib::dds::type>, boost::noncopyable>(            \
        #T, py::init<>())                                                                          \
        .def("set", &qlib::dds::T##_wrapper::set)                                                  \
        .def("get", &qlib::dds::T##_wrapper::get);                                                 \
    py::class_<qlib::dds::vector_##T##_wrapper>("vector_" #T, py::init<>())                        \
        .def("__len__", &qlib::dds::vector_##T##_wrapper::size)                                    \
        .def("__getitem__", &qlib::dds::vector_##T##_wrapper::get)                                 \
        .def("__setitem__", &qlib::dds::vector_##T##_wrapper::set);

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

    py::class_<qlib::dds::publisher_wrapper, boost::noncopyable>(
        "publisher", py::init<qlib::dds::type::ptr, std::string>())
        .def("publish", &qlib::dds::publisher_wrapper::publish, py::args("value"));

    py::class_<qlib::dds::subscriber_wrapper, boost::noncopyable>(
        "subscriber", py::init<qlib::dds::type::ptr, std::string, py::object>());
};

#ifdef WIN32
#include <Windows.h>
#endif

#include "boost/python.hpp"
#include "boost/python/overloads.hpp"
#include "qlib/dds.h"

namespace py = boost::python;

// namespace boost {
// namespace python {

// template <>
// struct to_python_converter<qlib::dds::string, std::string> {
//     static PyObject* convert(qlib::dds::string const& value) {
//         return boost::python::incref(boost::python::object(static_cast<std::string>(value)).ptr());
//     }
// };

// };  // namespace python
// };  // namespace boost

namespace qlib {
namespace dds {
class string_wrapper : public string {
public:
    string_wrapper() : string() {}

    void set(std::string const& value) { string::operator=(value); }

    std::string get() { return static_cast<std::string>(*this); }
};

class publisher_wrapper : public publisher {
public:
    publisher_wrapper(type::ptr const& type_ptr, std::string const& topic)
            : publisher(type_ptr, topic) {}

    int32_t publish(type::ptr const& type_ptr) { return publisher::publish(type_ptr); }

    // int32_t publish(std::string const& value) {
    //     return publisher::publish(qlib::dds::string::make(value));
    // }
};

class subscriber_wrapper : public subscriber {
public:
    subscriber_wrapper(type::ptr const& type_ptr,
                       std::string const& topic,
                       std::function<void(type::ptr const&)> const& callback)
            : subscriber(type_ptr, topic, callback) {}
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

    // py::implicitly_convertible<std::string, qlib::dds::string>();

    py::class_<qlib::dds::publisher_wrapper, boost::noncopyable>(
        "publisher", py::init<qlib::dds::type::ptr, std::string>())
        .def("publish", &qlib::dds::publisher_wrapper::publish, py::args("value"));

    py::class_<qlib::dds::subscriber_wrapper, boost::noncopyable>(
        "subscriber", py::init<qlib::dds::type::ptr, std::string, py::object>());
};

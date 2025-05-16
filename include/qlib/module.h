#pragma once

#include "qlib/object.h"
#include "qlib/singleton.h"

namespace qlib {

template <class T>
class module : public qlib::object<module<T>> {
public:
    using base = qlib::object<module<T>>;
    using self = module<T>;
    using ptr = std::shared_ptr<self>;

    struct parameter {};

    T& derived() { return static_cast<T&>(*this); }

    T const& derived() const { return static_cast<T const&>(*this); }

    template <class... Args>
    int32_t init(Args&&... args) {
        return derived().init(std::forward<Args>(args)...);
    }

    template <class... Args>
    int32_t exec(Args&&... args) {
        return derived().exec(std::forward<Args>(args)...);
    }

protected:
    qlib::string name;

    friend class qlib::ref_singleton<T>;

    template <class String>
    module(String&& _name) : name{std::forward<String>(_name)} {}
};

};  // namespace qlib
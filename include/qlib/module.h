#pragma once

#include "qlib/object.h"

namespace qlib {

template <class T>
class module : public object {
public:
    using base = object;
    using self = module<T>;
    using ptr = std::shared_ptr<self>;

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
    string_t name;

    template <class String>
    module(String&& _name) : name{std::forward<String>(_name)} {}
};

};  // namespace qlib
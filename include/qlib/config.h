#pragma once

#include <filesystem>

#include "qlib/object.h"

namespace qlib {

template <class T, class Loader>
class config : public object {
public:
    using base = object;
    using self = config<T, Loader>;
    using ptr = std::shared_ptr<self>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    T& derived() { return static_cast<T&>(*this); }
    T const& derived() const { return static_cast<T const&>(*this); }

    template <class Path>
    int32_t init(Path&& path) {
        int32_t result{0};

        do {
            if (!std::filesystem::exists(path)) {
                result = static_cast<int32_t>(error::file_not_found);
                break;
            }

            try {
                derived().load(Loader::make(std::forward<Path>(path)));
            } catch (std::exception const& e) {
                result = static_cast<int32_t>(error::file_invalid);
            }
        } while (false);

        return result;
    }

    template <class _T, class... Args>
    static inline _T get(Args&&... args) {
        return Loader::template get<_T>(std::forward<Args>(args)...);
    }

    template <class _T, class... Args>
    static inline void get(_T* value_ptr, Args&&... args) {
        *value_ptr = get<_T>(std::forward<Args>(args)...);
    }
};

};  // namespace qlib

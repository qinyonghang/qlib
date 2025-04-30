#pragma once

#include <memory>
#include <mutex>

#include "qlib/object.h"

namespace qlib {
template <class T>
class ref_singleton final : public qlib::object<ref_singleton<T>> {
public:
    template <class... Args>
    static std::shared_ptr<T> make(Args&&... args) {
        if (value_ptr == nullptr) {
            std::lock_guard<std::mutex> lock{mutex};
            if (value_ptr == nullptr) {
                value_ptr = new T(std::forward<Args>(args)...);
            }
        }

        ++count;
        return std::shared_ptr<T>{value_ptr, [](T* ptr) {
                                      --count;
                                      if (count == 0) {
                                          delete value_ptr;
                                          value_ptr = nullptr;
                                      }
                                  }};
    }

protected:
    static inline std::mutex mutex{};
    static inline T* value_ptr{nullptr};
    static inline std::atomic<uint32_t> count{0u};
};

};
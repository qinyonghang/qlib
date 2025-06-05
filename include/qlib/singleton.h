#pragma once

#include <atomic>
#include <mutex>

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {
template <class T>
class ref_singleton final : public object {
public:
    using base = object;
    using self = ref_singleton<T>;
    using ptr = std::shared_ptr<self>;

    template <class... Args>
    static std::shared_ptr<T> make(Args&&... args) {
        if constexpr ((sizeof...(Args) != 0) || (std::is_default_constructible_v<T>)) {
            if (value_ptr == nullptr) {
                std::lock_guard<std::mutex> lock{mutex};
                if (value_ptr == nullptr) {
                    value_ptr = new T(std::forward<Args>(args)...);
                }
            }
        } else {
            THROW_EXCEPTION(value_ptr != nullptr, "Object do not constructed!");
        }

        ++count;
        return std::shared_ptr<T>{value_ptr, [](T* ptr) {
                                      --count;
                                      if (count == 0) {
                                          std::lock_guard<std::mutex> lock{mutex};
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

};  // namespace qlib
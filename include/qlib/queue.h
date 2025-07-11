#pragma once

#include <queue>
#include <mutex>

#include "object.h"

namespace qlib {

// constexpr auto dynamic = static_cast<size_t>(-1);

// template <class T, size_t N = dynamic>
// class queue;

// template <class T>
// class queue<T, dynamic> : public object {
// public:
//     using self = queue<T>;
//     using ptr = std::shared_ptr<self>;
//     using base = object;

//     template <class... Args>
//     static ptr make(Args&&... args) {
//         return std::make_shared<self>(std::forward<Args>(args)...);
//     }

//     constexpr queue() noexcept = default;

//     template <class... Args>
//     auto emplace(Args&&... args) {
//         std::lock_guard<std::mutex> lock{_mutex};
//         return _impl.emplace_back(std::forward<Args>(args)...);
//     }

//     template <class... Args>
//     auto push(Args&&... args) {
//         return emplace(std::forward<Args>(args)...);
//     }

//     auto pop() {
//         std::deque<T> result;
//         {
//             std::lock_guard<std::mutex> lock{_mutex};
//             result = std::move(_impl);
//             _impl = std::deque<T>();
//         }
//         return result;
//     }

// protected:
//     std::mutex _mutex;
//     std::deque<T> _impl;
// };

// template <class T, size_t N>
// class queue : public object {
// public:
//     using self = queue<T, N>;
//     using ptr = std::shared_ptr<self>;
//     using base = object;

//     template <class... Args>
//     static ptr make(Args&&... args) {
//         return std::make_shared<self>(std::forward<Args>(args)...);
//     }

//     constexpr queue() noexcept = default;

//     template <class... Args>
//     auto emplace(Args&&... args) {
//         std::lock_guard<std::mutex> lock{_mutex};
//         return _impl.emplace_back(std::forward<Args>(args)...);
//     }

// protected:
//     std::mutex _mutex;
//     std::array<T, N> _impl;
// };

template <class T>
class queue : public object {
public:
    using self = queue<T>;
    using ptr = std::shared_ptr<self>;
    using base = object;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    queue(size_t _size = std::numeric_limits<size_t>::max()) : size{_size} {}

    template <class... Args>
    void emplace(Args&&... args) {
        auto size = self::impl.size();
        if (size >= self::size) {
            qWarn("Queue: Full! Pop Data!");
            std::lock_guard<std::mutex> lock(self::mutex);
            self::impl.pop();
        }

        std::lock_guard<std::mutex> lock(self::mutex);
        self::impl.emplace(std::forward<Args>(args)...);
    }

    std::queue<T> pop() {
        std::queue<T> queue;
        {
            std::lock_guard<std::mutex> lock(self::mutex);
            queue = std::move(self::impl);
            self::impl = std::queue<T>();
        }
        return queue;
    }

protected:
    std::mutex mutex;
    std::queue<T> impl;
    size_t size;
};

};  // namespace qlib

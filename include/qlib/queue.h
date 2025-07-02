#pragma once

#include <mutex>
#include <queue>

#include "object.h"

namespace qlib {

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

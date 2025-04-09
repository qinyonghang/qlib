#pragma once

#include <fstream>

#include "QException.h"
#include "QObject.h"

namespace qlib {

template <typename T = std::string>
class QLoader final : public QObject {
protected:
    struct Impl : public QObject {
        std::vector<T> data;
    };

    QObjectPtr __impl;

public:
    using Base = QObject;
    using Self = QLoader<T>;
    using Base::Base;

    int32_t init(char const* path) {
        int32_t result{0};

        do {
            auto impl = std::make_shared<Impl>();

            std::ifstream file{path, std::ios::binary | std::ios::ate};
            if (!file.is_open()) {
                result = -2;
                break;
            }

            auto size = static_cast<size_t>(file.tellg());
            QTHROW_EXCEPTION(!(size % sizeof(T)), "size is not multiple of {}", sizeof(T));
            impl->data.resize(size / sizeof(T));

            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char*>(impl->data.data()), size);
            if (file.bad()) {
                result = -3;
                break;
            }

            __impl = impl;
        } while (0);

        return result;
    }

    template <class Path, class = std::enable_if_t<!std::is_convertible_v<Path, char const*>>>
    int32_t init(Path&& path) {
        int32_t result{-1};

        if constexpr (has_string_v<Path>) {
            result = init(path.string());
        } else if constexpr (has_c_str_v<Path>) {
            result = init(path.c_str());
        }

        return result;
    }

    template <class... Args>
    QLoader(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        QCMTHROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    auto& data() {
        auto impl = std::static_pointer_cast<Impl>(__impl);
        QTHROW_EXCEPTION(impl, "impl is nullptr");
        return impl->data;
    }

    auto const& data() const { return const_cast<Self*>(this)->data(); }

    template <class... Args>
    static auto load(Args&&... args) {
        return Self{std::forward<Args>(args)...}.data();
    }
};

template <>
struct QLoader<std::string>::Impl : public QObject {
    std::string data;
};

template <>
int32_t QLoader<std::string>::init(char const* path) {
    int32_t result{0};

    do {
        std::ifstream file{path};
        if (!file.is_open()) {
            result = -1;
            break;
        }

        auto impl = std::make_shared<Impl>();
        impl->data =
            std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        __impl = impl;
    } while (0);

    return result;
}

};  // namespace qlib

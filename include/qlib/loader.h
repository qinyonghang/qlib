#pragma once

#include <fstream>

#include "exception.h"
#include "object.h"

namespace qlib {

template <typename T = string>
class loader final : public object<loader<T>> {
public:
    using base = object<loader<T>>;
    using self = loader<T>;
    using ptr = std::shared_ptr<self>;

protected:
    struct impl : public object<void> {
        using self = impl;
        using ptr = std::shared_ptr<self>;
        std::vector<T> data;
    };

    object<void>::ptr impl_ptr;

public:
    int32_t init(char const* path) {
        int32_t result{0};

        do {
            auto impl = std::make_shared<self::impl>();

            std::ifstream file{path, std::ios::binary | std::ios::ate};
            if (!file.is_open()) {
                result = qlib::FILE_NOT_FOUND;
                break;
            }

            auto size = static_cast<size_t>(file.tellg());
            if (size % sizeof(T)) {
                result = qlib::FILE_INVALID;
                break;
            }

            impl->data.resize(size / sizeof(T));

            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char*>(impl->data.data()), size);
            if (file.bad()) {
                result = qlib::FILE_INVALID;
                break;
            }

            this->impl_ptr = impl;
        } while (0);

        return result;
    }

    template <class Path, class = std::void_t<decltype(std::decay_t<Path>().c_str())>>
    int32_t init(Path&& path) {
        return init(std::forward<Path>(path).c_str());
    }

    template <class... Args>
    loader(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    auto& data() {
        auto impl = std::static_pointer_cast<self::impl>(this->impl_ptr);
        THROW_EXCEPTION(impl, "impl is nullptr");
        return impl->data;
    }

    auto const& data() const { return const_cast<self&>(*this).data(); }

    operator bool() const { return this->impl_ptr; }

    operator T&() { return data(); }

    operator T const&() const { return const_cast<self&>(*this).operator T(); }

    template <class... Args>
    static auto load(Args&&... args) {
        return self{std::forward<Args>(args)...}.data();
    }
};

template <>
struct loader<std::string>::impl : public object<void> {
    using self = impl;
    using ptr = std::shared_ptr<self>;
    std::string data;
};

template <>
int32_t loader<std::string>::init(char const* path) {
    int32_t result{0};

    do {
        std::ifstream file{path};
        if (!file.is_open()) {
            result = -1;
            break;
        }

        auto impl_ptr = std::make_shared<self::impl>();
        impl_ptr->data =
            std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        this->impl_ptr = impl_ptr;
    } while (0);

    return result;
}

};  // namespace qlib

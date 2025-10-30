#ifndef QLIB_YAML_H
#define QLIB_YAML_H

#include "qlib/memory.h"
#include "qlib/string.h"
#include "qlib/vector.h"

namespace qlib {

namespace yaml {

using memory_policy = size_t;

enum : size_t { ok, null = 0u, object, array, copy, view, not_array, not_object };

template <class _Tp, class Enable = void>
struct converter : public object {
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        return _Tp{__first, __last};
    }
};

template <class _Tp>
class converter<_Tp, enable_if_t<is_unsigned_v<_Tp>>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        _Tp __val{0u};
        while (__first != __last && is_digit(*__first)) {
            __val = __val * 10u + (*__first - '0');
            ++__first;
        }

        throw_if(__first != __last, "not number");

        return __val;
    }
};

template <class _Tp>
class converter<_Tp, enable_if_t<is_signed_v<_Tp>>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 first, Iter2 last) {
        _Tp sign{1u};
        if (*first == '-') {
            sign = -1;
            ++first;
        }

        auto __val = converter<typename unsigned_traits<_Tp>::type>::decode(first, last);

        return sign * _Tp(__val);
    }
};

template <class _Tp>
class converter<_Tp, enable_if_t<is_floating_point_v<_Tp>>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 first, Iter2 last) {
        _Tp _value{0u};

        _Tp sign = 1;
        if (*first == '-') {
            sign = -1;
            ++first;
        }

        while (first < last && is_digit(*first)) {
            _value = _value * 10u + static_cast<_Tp>(*first - '0');
            ++first;
        }

        if (*first == '.') {
            ++first;
            _Tp base = 0.1;
            while (first < last && is_digit(*first)) {
                _value += static_cast<_Tp>(*first - '0') * base;
                base *= 0.1;
                ++first;
            }
        }

        throw_if(first != last, "not number");

        return sign * _value;
    }
};

template <>
class converter<bool_t> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        string_view_t __str(__first, __last);
        CONSTEXPR string_view_t __true{"true"};
        CONSTEXPR string_view_t __false{"false"};
        CONSTEXPR string_view_t __true2{"True"};
        CONSTEXPR string_view_t __false2{"False"};
        if (__str == __true || __str == __true2) {
            return True;
        } else if (__str == __false || __str == __false2) {
            return False;
        } else {
            __throw("not boolean");
        }
    }
};

template <class _Char>
class converter<string::view<_Char>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        using string_view_t = string::view<_Char>;
        if (*__first == '"' && *(__last - 1) == '"') {
            ++__first;
            --__last;
        }
        return string_view_t(__first, __last);
    }
};

template <class _Char, class _Allocator>
class converter<string::value<_Char, _Allocator>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        if (*__first == '"' && *(__last - 1) == '"') {
            ++__first;
            --__last;
        }
        return string::value<_Char, _Allocator>(__first, __last);
    }
};

#ifdef _BASIC_STRING_H
template <class _Char>
class converter<std::basic_string<_Char>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD ALWAYS_INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        if (*__first == '"' && *(__last - 1) == '"') {
            ++__first;
            --__last;
        }
        return std::basic_string<_Char>(__first, __last);
    }
};
#endif

template <class Yaml>
class parser;

template <class Char, memory_policy Policy, class Allocator = new_allocator_t>
class value final : public traits<Allocator>::reference {
public:
    using base = typename traits<Allocator>::reference;
    using self = value;
    using char_type = Char;
    using allocator_type = Allocator;
    using string_view = string::view<Char>;
    using string_type = string::value<Char, allocator_type>;
    using view_type = conditional_t<Policy == view, string_view, string_type>;
    using copy_type = string_type;
    using array_type = vector_t<self, allocator_type>;
    using key_type = conditional_t<Policy == view, string_view, string_type>;
    using object_type = vector_t<pair<key_type, self>, allocator_type>;

    // CONSTEXPR static memory_policy policy = Policy;

    const static self default_value;

protected:
    uint8_t _type{yaml::null};
    union_storage<array_type, object_type> _impl{};

    friend class parser<self>;

    ALWAYS_INLINE CONSTEXPR allocator_type& _allocator_() const noexcept {
        return static_cast<base&>(const_cast<self&>(*this));
    }

    ALWAYS_INLINE CONSTEXPR auto& _array_() noexcept { return *(array_type*)(&_impl); }
    ALWAYS_INLINE CONSTEXPR auto& _object_() noexcept { return *(object_type*)(&_impl); }
    ALWAYS_INLINE CONSTEXPR auto& _view_() noexcept { return *(view_type*)(&_impl); }
    ALWAYS_INLINE CONSTEXPR auto const& _array_() const noexcept { return *(array_type*)(&_impl); }
    ALWAYS_INLINE CONSTEXPR auto const& _object_() const noexcept { return *(object_type*)(&_impl); }
    ALWAYS_INLINE CONSTEXPR auto const& _view_() const noexcept { return *(view_type*)(&_impl); }

    template <class _uKey = key_type>
    ALWAYS_INLINE CONSTEXPR enable_if_t<is_same_v<_uKey, string_view>, string_view> _key_init_(
        string_view __key) {
        return __key;
    }

    template <class _uKey = key_type>
    ALWAYS_INLINE CONSTEXPR enable_if_t<is_same_v<_uKey, string_type>, string_type> _key_init_(
        string_view __key) {
        return string_type(__key.begin(), __key.end(), _allocator_());
    }

public:
    value(self const&) = delete;
    self& operator=(self const& __o) = delete;

    value() = default;

    ALWAYS_INLINE CONSTEXPR value(self&& __o) : base(qlib::move(__o)), _type(__o._type), _impl(__o._impl) {
        __o._type = yaml::null;
    }

    ALWAYS_INLINE CONSTEXPR value(allocator_type& __allocator) : base(__allocator) {}

    ~value() {
        switch (_type) {
            case yaml::object:
                _object_().~object_type();
                break;
            case yaml::array:
                _array_().~array_type();
                break;
            // case yaml::copy:
            //     _view_().~view_type();
            //     break;
            default:;
        }
    }

    self& operator=(self&& __o) {
        this->~value();
        new (this) self(qlib::move(__o));
        return *this;
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR bool_t empty() const noexcept { return _type == yaml::null; }

    NODISCARD ALWAYS_INLINE CONSTEXPR auto type() const noexcept { return _type; }

    template <class _Tp>
    NODISCARD ALWAYS_INLINE CONSTEXPR remove_cvref_t<_Tp> get() const {
        throw_if(_type != yaml::view, "invalid value");
        auto& __str_value = _view_();
        return converter<remove_cvref_t<_Tp>>::decode(__str_value.begin(), __str_value.end());
    }

    template <class _Tp>
    NODISCARD ALWAYS_INLINE CONSTEXPR remove_cvref_t<_Tp> get(_Tp const& default_value) const {
        if (empty()) {
            return default_value;
        }
        return get<_Tp>();
    }

    template <class _Tp>
    NODISCARD ALWAYS_INLINE CONSTEXPR remove_cvref_t<_Tp> get(_Tp&& default_value) const {
        if (empty()) {
            return qlib::move(default_value);
        }
        return get<_Tp>();
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR auto& operator[](string_view __key) {
        throw_if(_type != yaml::object, "not object");
        for (auto& __node : _object_()) {
            if (__node.key == __key) {
                return __node.value;
            }
        }
        _object_().emplace_back(_key_init_(__key), self());
        return _object_().back().value;
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR auto const& operator[](string_view __key) const {
        throw_if(_type != yaml::object, "not object");
        for (auto& __node : _object_()) {
            if (__node.key == __key) {
                return __node.value;
            }
        }
        return default_value;
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR auto& object() {
        throw_if(_type != yaml::object, "not object");
        return _object_();
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR auto const& object() const {
        return const_cast<self*>(this)->object();
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR auto& array() {
        throw_if(_type != yaml::array, "not array");
        return _array_();
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR auto const& array() const {
        return const_cast<self*>(this)->array();
    }

    NODISCARD ALWAYS_INLINE CONSTEXPR explicit operator bool_t() const noexcept {
        return _type != yaml::null;
    }

    template <class _OStream>
    static _OStream& to(_OStream& __out, self const& __node, size_t __indent = 0u) {
        constexpr string_view __str_indent = "  ";
        constexpr string_view __str_lf = "\n";
        constexpr string_view __str_colon = ": ";
        // constexpr string_view __str_comma = ", ";
        constexpr string_view __str_quote = "\"";
        constexpr string_view __str_null = "~";
        constexpr string_view __str_hyphen = "-";
        switch (__node._type) {
            case yaml::null: {
                __out << __str_null;
                break;
            }
            case yaml::view: {
                __out << __node._view_();
                break;
            }
            case yaml::object: {
                auto& __object = __node._object_();
                __out << __str_lf;
                for (auto i = 0u; i < __object.size(); ++i) {
                    auto& __key = __object[i].key;
                    auto& __n = __object[i].value;
                    for (auto i = 0u; i < __indent; ++i) {
                        __out << __str_indent;
                    }
                    __out << __str_quote << __key << __str_quote << __str_colon;
                    to(__out, __n, __indent + 1u);
                    if (i != __object.size() - 1) {
                        __out << __str_lf;
                    }
                }
                // __out << __str_lf;
                break;
            }
            case yaml::array: {
                auto& __array = __node._array_();
                __out << __str_lf;
                for (auto i = 0u; i < __array.size(); ++i) {
                    auto& __n = __array[i];
                    for (auto i = 0u; i < __indent; ++i) {
                        __out << __str_indent;
                    }
                    __out << __str_hyphen;
                    to(__out, __n, __indent + 1u);
                    if (i != __array.size() - 1) {
                        __out << __str_lf;
                    }
                }
                // __out << __str_lf;
                break;
            }
            default:
                __throw("exception");
        }
        return __out;
    }
};

template <class Char, memory_policy Policy, class Allocator>
const typename value<Char, Policy, Allocator>::self value<Char, Policy, Allocator>::default_value =
    value<Char, Policy, Allocator>{};

template <class _OStream, class Char, size_t Policy, class Allocator>
ALWAYS_INLINE _OStream& operator<<(_OStream& __out, value<Char, Policy, Allocator> const& __node) {
    return value<Char, Policy, Allocator>::to(__out, __node);
}

template <class Yaml>
class parser final : public object {
public:
    using self = object;
    using yaml_type = Yaml;
    using char_type = typename yaml_type::char_type;
    using allocator_type = typename yaml_type::allocator_type;
    using string_view = typename yaml_type::string_view;
    using string_type = typename yaml_type::string_type;
    using view_type = typename yaml_type::view_type;
    using array_type = typename yaml_type::array_type;
    using object_type = typename yaml_type::object_type;
    using key_type = typename yaml_type::key_type;
    // CONSTEXPR static memory_policy policy = yaml_type::policy;

protected:
    template <class _uView = key_type>
    ALWAYS_INLINE CONSTEXPR enable_if_t<is_same_v<_uView, string_view>, key_type> _key_init_(
        string_view __view, allocator_type& __allocator) {
        return __view;
    }

    template <class _uView = key_type>
    ALWAYS_INLINE CONSTEXPR enable_if_t<is_same_v<_uView, string_type>, key_type> _key_init_(
        string_view __view, allocator_type& __allocator) {
        return string_type(__view.begin(), __view.end(), __allocator);
    }

    template <class _uView = view_type>
    ALWAYS_INLINE CONSTEXPR enable_if_t<is_same_v<_uView, string_view>, yaml_type> _value_init_(
        string_view __view, allocator_type& __allocator) {
        yaml_type __node;
        __node._type = yaml::view;
        new (&__node._impl) string_view(__view);
        return __node;
    }

    template <class _uView = view_type>
    ALWAYS_INLINE CONSTEXPR enable_if_t<is_same_v<_uView, string_type>, yaml_type> _value_init_(
        string_view __view, allocator_type& __allocator) {
        yaml_type __node;
        __node._type = yaml::view;
        new (&__node._impl) string_type(__view.begin(), __view.end(), __allocator);
        return __node;
    }

    class char_helper final : public object {
    public:
        using self = char_helper;

        uint8_t _impl[256u]{};

        enum : uint8_t {
            hyphen = 0x01,
            hash = 0x02,
            skip = 0x10,
            space = skip | 0x01,
            tab = skip | 0x02,
            cr = skip | 0x03,
            lf = skip | 0x04,
        };

        ALWAYS_INLINE CONSTEXPR char_helper() {
            _impl['-'] = self::hyphen;
            _impl['#'] = self::hash;
            _impl[' '] = self::space;
            _impl['\t'] = self::tab;
            _impl['\r'] = self::cr;
            _impl['\n'] = self::lf;
        }

        ALWAYS_INLINE CONSTEXPR auto type(uint8_t __char) const noexcept { return _impl[__char]; };
    };

#if defined(_cpp17_)
    CONSTEXPR static char_helper _char_helper{};
#else  // C++14
    CONSTEXPR static char_helper _char_helper;
#endif

    template <class _Iter1, class _Iter2>
    ALWAYS_INLINE CONSTEXPR static auto _parse_value_(string_view* __view,
                                                     _Iter1 __first,
                                                     _Iter2 __last) {
        auto __begin = __first;
        if (*__first == '"') {
            ++__first;
            while (__first < __last) {
                if (*__first == '\\') {
                    ++__first;
                } else if (*__first == '"') {
                    break;
                }
                ++__first;
            }
            ++__first;
        } else {
            while (__first < __last && *__first != '\n' && *__first != '\r' && *__first != '#' &&
                   *__first != ' ') {
                ++__first;
            }
        }
        *__view = string_view(__begin, __first);
        while (__first < __last && *__first != '\n') {
            ++__first;
        }
        ++__first;
        return __first;
    }

public:
    template <class _Iter1, class _Iter2>
    ALWAYS_INLINE CONSTEXPR auto operator()(yaml_type* __yaml,
                                     _Iter1 __first,
                                     _Iter2 __last,
                                     size_t __capacity = 16u) {
        int32_t __result{0};

        do {
            auto& __allocator = __yaml->_allocator_();

            yaml_type __root{__allocator};
            __root._type = yaml::object;
            new (&__root._impl) object_type(__allocator);
            vector_t<pair<size_t, yaml_type*>> __stack(__capacity, __allocator);
            __stack.emplace_back(0u, &__root);

            while (__first < __last && (_char_helper.type(*__first) & char_helper::skip)) {
                ++__first;
            }
            string_view __key{};
            string_view __value{};
            while (__first < __last) {
                auto __begin = __first;
                while (__first < __last && _char_helper.type(*__first) == char_helper::space) {
                    ++__first;
                }
                switch (_char_helper.type(*__first)) {
                    case char_helper::cr: {
                        ++__first;
                        if (_char_helper.type(*__first) == char_helper::lf) {
                            ++__first;
                        }
                        break;
                    }
                    case char_helper::lf: {
                        ++__first;
                        break;
                    }
                    case char_helper::hash: {
                        while (__first < __last && _char_helper.type(*__first) != char_helper::lf) {
                            ++__first;
                        }
                        ++__first;
                        break;
                    }
                    case char_helper::hyphen: {
                        auto __space = size_t(distance(__begin, __first));
                        while (__stack.size()) {
                            if (__space < __stack.back().key) {
                                __stack.pop_back();
                            } else {
                                break;
                            }
                        }
                        auto& __last_layer = __stack.back().value;
                        if (__last_layer->_type == yaml::null) {
                            __last_layer->_type = yaml::array;
                            new (&__last_layer->_impl) array_type(__allocator);
                        }
                        if (__last_layer->_type != yaml::array) {
                            __result = yaml::not_array;
                            break;
                        }
                        ++__first;
                        while (__first < __last && *__first == ' ') {
                            ++__first;
                        }
                        __first = _parse_value_(&__value, __first, __last);
                        __last_layer->_array_().emplace_back(_value_init_(__value, __allocator));
                        break;
                    }
                    default: {
                        auto __space = size_t(distance(__begin, __first));
                        while (__stack.size()) {
                            if (__space < __stack.back().key) {
                                __stack.pop_back();
                            } else {
                                break;
                            }
                        }
                        auto& __last_layer = __stack.back().value;
                        if (__last_layer->_type == yaml::null) {
                            __last_layer->_type = yaml::object;
                            new (&__last_layer->_impl) object_type(__allocator);
                        }
                        if (__last_layer->_type != yaml::object) {
                            __result = yaml::not_object;
                            break;
                        }
                        auto __begin = __first;
                        while (__first < __last && *__first != ':') {
                            ++__first;
                        }
                        __key = string_view{__begin, __first};
                        ++__first;
                        while (__first < __last && *__first == ' ') {
                            ++__first;
                        }
                        switch (_char_helper.type(*__first)) {
                            case char_helper::cr: {
                                ++__first;
                                if (_char_helper.type(*__first) == char_helper::lf) {
                                    ++__first;
                                }
                                __last_layer->_object_().emplace_back(
                                    _key_init_(__key, __allocator), yaml_type{__allocator});
                                __stack.emplace_back(__space + 1u,
                                                     &__last_layer->_object_().back().value);
                                break;
                            }
                            case char_helper::lf: {
                                ++__first;
                                __last_layer->_object_().emplace_back(
                                    _key_init_(__key, __allocator), yaml_type{__allocator});
                                __stack.emplace_back(__space + 1u,
                                                     &__last_layer->_object_().back().value);
                                break;
                            }
                            default: {
                                __first = _parse_value_(&__value, __first, __last);
                                __last_layer->_object_().emplace_back(
                                    _key_init_(__key, __allocator),
                                    _value_init_(__value, __allocator));
                            }
                        }
                    }
                }
            }

            if (0 != __result) {
                break;
            }

            *__yaml = qlib::move(__root);
        } while (false);

        return __result;
    }
};

#if !defined(_cpp17_)
template <class Json>
typename parser<Json>::char_helper parser<Json>::_char_helper{};
#endif

template <class _Char, memory_policy _Policy, class _Allocator, class _Iter1, class _Iter2>
ALWAYS_INLINE CONSTEXPR auto parse(value<_Char, _Policy, _Allocator>* __yaml,
                            _Iter1 __first,
                            _Iter2 __last) {
    using yaml_type = value<_Char, _Policy, _Allocator>;
    parser<yaml_type> __parser;
    return __parser(__yaml, __first, __last);
}

};  // namespace yaml

namespace string {
template <class Char, yaml::memory_policy Policy, class Allocator>
ALWAYS_INLINE CONSTEXPR value<Char, Allocator> from_yaml(yaml::value<Char, Policy, Allocator> const& node,
                                                  size_t size = 1024u) {
    value<Char, Allocator> result(size);
    yaml::value<Char, Policy, Allocator>::to(result, node);
    return result;
}
};  // namespace string

using yaml_view_t = yaml::value<char, yaml::view>;
using yaml_t = yaml::value<char, yaml::copy>;

};  // namespace qlib

#endif

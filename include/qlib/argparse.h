#pragma once

#define ARGPARSE_IMPLEMENTATION

#include "qlib/any.h"
#include "qlib/string.h"
#include "qlib/vector.h"

namespace qlib {
namespace argparse {

class help final : public exception {
public:
    char const* what() const noexcept override { return "help"; }
};

class not_number final : public exception {
public:
    char const* what() const noexcept override { return "not number"; }
};

class not_boolean final : public exception {
public:
    char const* what() const noexcept override { return "not boolean"; }
};

class not_found : public exception {
public:
    char const* what() const noexcept override { return "not found"; }
};

class unknown : public exception {
public:
    char const* what() const noexcept override { return "unknown"; }
};

class missing_value : public exception {
public:
    char const* what() const noexcept override { return "missing value"; }
};

template <class _Tp, class Enable = void>
struct converter : public object {
    template <class Iter1, class Iter2>
    NODISCARD INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        return _Tp{__first, __last};
    }
};

template <class _Tp>
class converter<_Tp, enable_if_t<is_unsigned_v<_Tp>>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        _Tp __val{0u};
        while (__first != __last && is_digit(*__first)) {
            __val = __val * 10u + (*__first - '0');
            ++__first;
        }

        throw_if(__first != __last, not_number());

        return __val;
    }
};

template <class _Tp>
class converter<_Tp, enable_if_t<is_signed_v<_Tp>>> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD INLINE CONSTEXPR static auto decode(Iter1 first, Iter2 last) {
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
    NODISCARD INLINE CONSTEXPR static auto decode(Iter1 first, Iter2 last) {
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

        throw_if(first != last, not_number());

        return sign * _value;
    }
};

template <>
class converter<bool_t> : public object {
public:
    template <class Iter1, class Iter2>
    NODISCARD INLINE CONSTEXPR static auto decode(Iter1 __first, Iter2 __last) {
        string_view_t __str(__first, __last);
        CONSTEXPR string_view_t __true{"true"};
        CONSTEXPR string_view_t __false{"false"};
        if (__str == __true) {
            return True;
        } else if (__str == __false) {
            return False;
        } else {
            __throw(not_boolean{});
        }
    }
};

template <class Char = char, class Allocator = new_allocator_t>
class argument final : public traits<Allocator>::reference {
public:
    using base = typename traits<Allocator>::reference;
    using self = argument;
    using ptr = self*;
    using char_type = Char;
    using allocator_type = Allocator;
    using string_view = string::view<char_type>;
    using string_type = string::value<char_type, allocator_type>;
    using any_type = any::value;

protected:
    string_type _name;
    string_type _value;
    string_type _help;
    any_type _default_value;

public:
    template <class String>
    FORCE_INLINE CONSTEXPR argument(String&& name) : _name{forward<String>(name)} {}

    template <class Name, class _Tp>
    FORCE_INLINE CONSTEXPR argument(Name&& name, _Tp&& default_value)
            : _name{forward<Name&&>(name)}, _default_value{forward<_Tp&&>(default_value)} {}

    FORCE_INLINE CONSTEXPR void parse(string_view value) { _value = value; }

    NODISCARD FORCE_INLINE CONSTEXPR auto& name() const noexcept { return _name; }

    template <class... Args, class = enable_if_t<sizeof...(Args)>>
    FORCE_INLINE CONSTEXPR argument& name(Args&&... args) {
        _name = decltype(_name)(forward<Args>(args)...);
        return *this;
    }

    NODISCARD FORCE_INLINE CONSTEXPR auto& help() const noexcept { return _help; }

    template <class... Args, class = enable_if_t<sizeof...(Args)>>
    FORCE_INLINE CONSTEXPR argument& help(Args&&... args) {
        _help = decltype(_help)(forward<Args>(args)...);
        return *this;
    }

    template <typename _Tp>
    NODISCARD FORCE_INLINE CONSTEXPR _Tp get() const {
        if (_value.empty()) {
            return any_cast<_Tp>(_default_value);
        } else {
            return converter<remove_cvref_t<_Tp>>::decode(_value.begin(), _value.end());
        }
    }

    template <class... Args>
    FORCE_INLINE CONSTEXPR argument& default_value(Args&&... args) {
        _default_value = decltype(_default_value)(forward<Args>(args)...);
        return *this;
    }

    NODISCARD FORCE_INLINE CONSTEXPR bool has_value() const noexcept {
        return _default_value.has_value() || (!_value.empty());
    }
};

template <class Char = char, class Allocator = new_allocator_t>
class parser final : public traits<Allocator>::reference {
public:
    using base = typename traits<Allocator>::reference;
    using self = parser;
    using char_type = Char;
    using allocator_type = Allocator;
    using string_view = string::view<char_type>;
    using string_type = string::value<char_type, allocator_type>;
    using any_type = any::value;
    using argument_type = argument<Char, allocator_type>;
    using arguments_type = vector::value<typename argument_type::ptr, allocator_type>;

protected:
    string_type _name;
    arguments_type _positional_arguments;
    arguments_type _optional_arguments;

    constexpr static string_view _help_str{"help"};

    NODISCARD FORCE_INLINE CONSTEXPR static typename argument_type::ptr _find_(
        arguments_type const& args, string_view name) noexcept {
        typename argument_type::ptr result{nullptr};
        for (auto& arg : args) {
            if (arg->name() == name) {
                result = arg;
                break;
            }
        }
        return result;
    }

    NODISCARD FORCE_INLINE CONSTEXPR auto _help_() const {
        string_t out{1024u};

        out << "usage: " << _name << " ";

        size_t max_name_len{0u};
        for (auto const& arg : _positional_arguments) {
            out << arg->name() << " ";
            max_name_len = max(max_name_len, size_t(arg->name().size()));
        }
        for (auto const& arg : _optional_arguments) {
            out << "[" << arg->name() << "] ";
            max_name_len = max(max_name_len, size_t(arg->name().size()));
        }
        out << "\n";

        max_name_len = (max_name_len + 8) / 8 * 8 + 1;
        if (_positional_arguments.size() > 0) {
            out << "\npositional arguments:\n";
            for (auto const& arg : _positional_arguments) {
                out << "  " << arg->name();
                if (!arg->help().empty()) {
                    for (auto i = arg->help().size(); i < max_name_len; ++i) {
                        out << " ";
                    }
                    out << "\t\t" << arg->help();
                }
                out << "\n";
            }
        }

        if (_optional_arguments.size() > 0) {
            out << "\noptional arguments:\n";
            for (auto const& arg : _optional_arguments) {
                out << "  " << arg->name();
                if (!arg->help().empty()) {
                    for (auto i = arg->help().size(); i < max_name_len; ++i) {
                        out << " ";
                    }
                    out << "\t\t" << arg->help();
                }
                out << "\n";
            }
        }

        return out;
    }

public:
    template <class String>
    NODISCARD FORCE_INLINE CONSTEXPR parser(String name) noexcept : _name{forward<String>(name)} {}

    parser(self const&) = delete;
    parser(self&&) = delete;

    FORCE_INLINE ~parser() {
        for (auto& arg : _positional_arguments) {
            delete arg;
        }
        for (auto& arg : _optional_arguments) {
            delete arg;
        }
    }

    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    template <class... Args>
    NODISCARD FORCE_INLINE CONSTEXPR argument_type& add_argument(string_view name,
                                                                 Args&&... args) noexcept {
        typename argument_type::ptr arg;

        if (name.starts_with("--")) {
            arg = new argument_type(name.data() + 2, forward<Args>(args)...);
            _optional_arguments.emplace_back(arg);
        } else if (name.starts_with("-")) {
            arg = new argument_type(name.data() + 1, forward<Args>(args)...);
            _optional_arguments.emplace_back(arg);
        } else {
            arg = new argument_type(name, forward<Args>(args)...);
            _positional_arguments.emplace_back(arg);
        }

        return *arg;
    }

    template <typename _Tp>
    NODISCARD INLINE CONSTEXPR _Tp get(string_view name) const {
        string_type _name;
        if (name.starts_with("--")) {
            _name = name.data() + 2;
        } else if (name.starts_with("-")) {
            _name = name.data() + 1;
        } else {
            _name = name;
        }

        typename argument_type::ptr arg{nullptr};
        arg = _find_(_positional_arguments, _name);
        if (arg == nullptr) {
            arg = _find_(_optional_arguments, _name);
        }
        throw_if(arg == nullptr, not_found{});

        return arg->template get<_Tp>();
    }

    template <class It1, class It2>
    INLINE CONSTEXPR void parse_args(It1 begin, It2 end) {
        throw_if(distance(begin, end) == 1 &&
                     (string_view("--help") == *begin || string_view("-h") == *begin),
                 help());

        auto it = begin;
        for (auto& arg : _positional_arguments) {
            throw_if(it == end, missing_value{});
            arg->parse(*it);
            ++it;
        }

        for (; it != end; ++it) {
            typename argument_type::ptr arg{nullptr};
            if (*(*it) == '-' && *(*(it + 1)) == '-') {
                arg = _find_(_optional_arguments, *it + 2);
            } else if (*(*it) == '-') {
                arg = _find_(_optional_arguments, *it + 1);
            }
            throw_if(arg == nullptr, unknown());
            throw_if(it + 1 >= end, missing_value());

            arg->parse(*++it);
        }

        for (auto& arg : _optional_arguments) {
            throw_if(!arg->has_value(), missing_value());
        }
    }

    NODISCARD FORCE_INLINE CONSTEXPR auto help() const { return _help_(); }
};
};  // namespace argparse
};  // namespace qlib

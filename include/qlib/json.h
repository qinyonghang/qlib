#pragma once

#define JSON_IMPLEMENTATION

// #include <any>

#include "nlohmann/json.hpp"
// #include "qlib/converter.h"

// namespace qlib {
// namespace json {

// // class value final : public object {
// // public:
// //     using base = object;
// //     using self = value;

// //     constexpr value() noexcept = default;

// //     template <class T>
// //     constexpr explicit value(T&& value) noexcept : _impl{std::forward<T>(value)} {}

// //     template <class T>
// //     constexpr T get() const noexcept {
// //         return std::any_cast<T>(_impl);
// //     }

// // protected:
// //     std::any _impl;
// // };

// enum memory_policy { copy, view };

// template <memory_policy policy = memory_policy::copy>
// class value;

// template <>
// class value<memory_policy::copy> : public object {
// public:
//     template <class String, class T>
//     constexpr explicit value(String&& name, T&& value) noexcept
//             : _name{std::forward<String>(name)}, _impl{std::forward<T>(value)} {}

// protected:
//     string_t _name;
//     any_t _impl;
// };

// template <>
// class value<memory_policy::view> : public object {
// public:
//     template <class String, class T>
//     constexpr explicit value(String&& name, T&& value) noexcept
//             : _name{std::forward<String>(name)}, _impl{std::forward<T>(value)} {}

// protected:
//     string_view_t _name;
//     any_t _impl;
// };

// template <memory_policy policy = memory_policy::copy>
// class parser final : public object {
// public:
//     using base = object;
//     using self = parser;

//     constexpr parser() noexcept = default;

//     template <class Iter1, class Iter2>
//     auto parse(Iter1&& first, Iter2&& last) {
//         std::vector<char> buffer;

//         buffer.reserve(std::min(std::distance(first, last), 1024));
//         for (auto it = first; it != last; ++it) {
//             if (*it == ',') {
//             }
//         }
//     }

// protected:
//     string_t _
// };

// template <class Iter1, class Iter2>
// auto parse(Iter1&& first, Iter2&& last) {
//     return parser().parse(first, last);
// }

// };  // namespace json
// };  // namespace qlib

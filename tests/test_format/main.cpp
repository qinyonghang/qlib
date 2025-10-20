#include <gtest/gtest.h>
#include <chrono>
#include <iostream>

#include "qlib/format.h"

using namespace qlib;

TEST(format, str) {
    auto s = fmt::format("{} {}", "hello", "world");
    EXPECT_EQ(s, "hello world");
}

TEST(format, uint) {
    auto s = fmt::format("{} {}", 1, 2);
    EXPECT_EQ(s, "1 2");
}

TEST(format, int) {
    auto s = fmt::format("{} {}", -1, -2);
    EXPECT_EQ(s, "-1 -2");
}

TEST(format, float) {
    auto s = fmt::format("{} {}", 1.1f, 2.2f);
    EXPECT_EQ(s, "1.100000 2.200000");
}

TEST(format, double) {
    auto s = fmt::format("{} {}", 1.1, 2.2);
    EXPECT_EQ(s, "1.100000 2.200000");
}

TEST(format, bool) {
    auto s = fmt::format("{} {}", true, false);
    EXPECT_EQ(s, "true false");
}

TEST(format, char) {
    auto s = fmt::format("{} {}", 'a', 'b');
    EXPECT_EQ(s, "a b");
}

TEST(format, chrono) {
    std::tm tm{};              // 初始化所有字段（避免未定义行为）
    tm.tm_year = 2021 - 1900;  // 年份：2021
    tm.tm_mon = 1 - 1;         // 月份：1月（0-based）
    tm.tm_mday = 1;            // 日期：1日
    tm.tm_hour = 12;           // 小时：12
    tm.tm_min = 34;            // 分钟：34
    tm.tm_sec = 56;            // 秒：56
    tm.tm_isdst = 0;           // 禁用夏令时
    std::time_t rawtime = std::mktime(&tm);
    auto tp = std::chrono::system_clock::from_time_t(rawtime);
    auto s = fmt::format("{:%Y-%m-%d %H:%M:%S}", tp);
    EXPECT_EQ(s, "2021-01-01 12:34:56");
}

TEST(format, strFix) {
    constexpr auto b = fmt::bformat<13u>("{}", "hello world!");
    string_view_t s = b;
    EXPECT_EQ(s, "hello world!");
}

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    } while (false);

    return result;
}

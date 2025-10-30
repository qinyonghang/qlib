#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "qlib/logger.h"

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        using namespace qlib;
        using sink1_type = logger::ansicolor_sink<std::ostream>;
        using sink2_type = logger::file_sink<std::ofstream>;
        using sinks_type = vector::value<std::shared_ptr<logger::sink>>;
        using logger_type = logger::value<string_view_t, sinks_type>;

        sink1_type sink1{std::cout};
        sink1.set_level(logger::info);
        std::filesystem::path log_dir{"logs"};
        std::filesystem::create_directories(log_dir);
        auto name = fmt::format("{%Y-%m-%d-%H-%M-%S}.log", std::chrono::system_clock::now());
        auto log_path = log_dir / name.c_str();
        sink2_type sink2{log_path};
        sink2.set_level(logger::trace);
        vector::value<std::shared_ptr<logger::sink>> sinks{2};
        sinks.emplace_back(std::make_shared<sink1_type>(qlib::move(sink1)));
        sinks.emplace_back(std::make_shared<sink2_type>(qlib::move(sink2)));
        logger_type logger{"qlib", qlib::move(sinks)};
        logger.trace("this is a trace message!");
        logger.debug("this is a debug message!");
        logger.info("this is a info message!");
        logger.warn("this is a warn message!");
        logger.error("this is a error message!");
        logger.critical("this is a critical message!");
        logger.warn("uint: {}", 123);
        logger.warn("int: {}", -123);
        logger.warn("float: {}", 123.456);
        logger.warn("double: {}", -123.456);
        logger.warn("string: {}", "hello world!");
        logger.warn("bool: {}", true);
    } while (false);

    return result;
}

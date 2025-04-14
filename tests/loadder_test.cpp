#include <filesystem>

#include "argparse/argparse.hpp"
#include "qlib/exception.h"
#include "qlib/loader.h"
#include "qlib/logger.h"

template <>
std::filesystem::path argparse::ArgumentParser::get(std::string_view arg_name) const {
    THROW_EXCEPTION(m_is_parsed, "Nothing parsed, no arguments are available.");

    auto arg = (*this)[arg_name];

    std::filesystem::path result;
    if (arg.m_values.size()) {
        result = arg.get<std::string>();
    } else {
        result = arg.get<std::filesystem::path>();
    }
    return result;
}

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        argparse::ArgumentParser parser("loader");

        parser.add_argument("path")
            .default_value<std::filesystem::path>("config.yaml")
            .help("Path to config file.");

        try {
            parser.parse_args(argc, argv);
        } catch (const std::runtime_error& err) {
            qCritical("Error parsing arguments: {}", err.what());
            qCritical("Parser: {}", parser.help());
            result = -1;
            break;
        }

        auto path = parser.get<std::filesystem::path>("path");

        qInfo("File: {}", qlib::loader<>::load(path));
    } while (false);

    return result;
}

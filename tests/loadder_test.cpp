#include <filesystem>

#include "qlib/argparse.h"
#include "qlib/exception.h"
#include "qlib/loader.h"
#include "qlib/logger.h"

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        argparse::ArgumentParser program("loader");

        program.add_argument("path").help("Path to config file.");

        try {
            program.parse_args(argc, argv);
        } catch (const std::runtime_error& err) {
            std::cout << err.what() << std::endl;
            std::cout << program;
            result = -1;
            break;
        }

        auto path = program.get<std::filesystem::path>("path");

        std::cout << "Path: " << path << std::endl;
        std::cout << "File: " << qlib::loader<>::load(path) << std::endl;
    } while (false);

    return result;
}

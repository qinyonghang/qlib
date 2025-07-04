#pragma once

#include <signal.h>

#include <atomic>
#include <iostream>

#include "qlib/argparse.h"

namespace qlib {

class application : public object {
public:
    using self = application;
    using ptr = std::shared_ptr<self>;
    using base = object;

protected:
    static inline std::atomic_bool exit{false};

    application() {
        signal(SIGINT, +[](int signal) { self::exit = true; });
    }

    static bool parse_args(argparse::parser* parser, int argc, char* argv[]) {
        bool ok{true};

        try {
            parser->parse_args(argv + 1, argv + argc);
        } catch (std::exception const& e) {
            std::cout << e.what() << std::endl;
            std::cout << parser->help() << std::endl;
            ok = false;
        }

        return ok;
    }
};

};  // namespace qlib

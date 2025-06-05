#pragma once

#include <signal.h>

#include <atomic>
#include <iostream>

#include "qlib/argparse.h"
#include "qlib/exception.h"
#include "qlib/module.h"

namespace qlib {

class application : public module<application> {
public:
    using self = application;
    using ptr = std::shared_ptr<self>;
    using base = module<self>;

protected:
    static inline std::atomic_bool exit{false};

    template <class String>
    application(String&& name) : base(std::forward<String>(name)) {
        signal(SIGINT, +[](int signal) { self::exit = true; });
    }

    template <class Callback>
    static bool parse_args(argparse::ArgumentParser* parser,
                           int argc,
                           char* argv[],
                           Callback&& add_argument) {
        bool ok{true};

        add_argument(parser);

        try {
            parser->parse_args(argc, argv);
        } catch (std::exception const& e) {
            std::cout << e.what() << std::endl;
            std::cout << *parser << std::endl;
            ok = false;
        }

        return ok;
    }
};

};  // namespace qlib

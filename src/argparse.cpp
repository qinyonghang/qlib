#include "qlib/argparse.h"

#include <sstream>

namespace qlib {
namespace argparse {

string_t parser::help() const noexcept {
    std::stringstream out;

    out << "usage: " << self::_name << " ";

    for (auto const& arg : self::optional_arguments) {
        out << "[" << arg->name() << "] ";
    }

    for (auto const& arg : self::positional_arguments) {
        out << arg->name() << " ";
    }
    out << std::endl;

    out << std::endl << "positional arguments:" << std::endl;
    for (auto const& arg : self::positional_arguments) {
        out << "  " << arg->name();
        if (!arg->help().empty()) {
            out << "\t\t\t" << arg->help();
        }
        out << std::endl;
    }
    out << std::endl << "optional arguments:" << std::endl;
    for (auto const& arg : optional_arguments) {
        out << "  " << arg->name();
        if (!arg->help().empty()) {
            out << "\t\t\t" << arg->help();
        }
        out << std::endl;
    }

    return out.str();
}

};  // namespace argparse
};  // namespace qlib

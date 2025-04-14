#pragma once

#define ARGPARSE_IMPLEMENTATION

#include "argparse/argparse.hpp"
#include "exception.h"

#ifdef _GLIBCXX_FILESYSTEM
template <>
std::filesystem::path argparse::ArgumentParser::get<std::filesystem::path>(
    std::string_view arg_name) const {
    THROW_EXCEPTION(m_is_parsed, "Nothing parsed, no arguments are available.");
    return (*this)[arg_name].get<std::string>();
}
#endif

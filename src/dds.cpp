#include <iostream>

#include "fastcdr/FastCdr.h"
#include "foonathan/memory/memory_pool.hpp"
#include "tinyxml2.h"

namespace qlib {
namespace dds {

void __attribute__((used)) force_link_deps(void) {
    auto error = tinyxml2::XMLDocument::ErrorIDToName(tinyxml2::XML_SUCCESS);
    std::cout << error << std::endl;

    eprosima::fastcdr::FastBuffer fast_butter;
    std::cout << reinterpret_cast<void*>(&fast_butter) << std::endl;

    auto handler = foonathan::memory::out_of_memory::get_handler();
    std::cout << reinterpret_cast<void*>(&handler) << std::endl;
}
void (*g_dds)(void) = force_link_deps;
};  // namespace dds
};  // namespace qlib

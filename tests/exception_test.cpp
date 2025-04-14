#include "qlib/exception.h"

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        THROW_EXCEPTION(false, "Test exception");
    } while (false);

    return result;
}

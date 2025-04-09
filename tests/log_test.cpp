#include "QLog.h"

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        qMCritical("Hello, World!");
        qMError("Hello, World!");
        qMWarn("Hello, World!");
        qMInfo("Hello, World!");
        qMDebug("Hello, World!");
        qMTrace("Hello, World!");
    } while (false);

    return result;
}

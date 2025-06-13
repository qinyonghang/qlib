#include <filesystem>

#include "qlib/logger.h"

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        qCritical("Hello, World!");
        qError("Hello, World!");
        qWarn("Hello, World!");
        qInfo("Hello, World!");
        qDebug("Hello, World!");
        qTrace("Hello, World!");

        qError("Path: {}", std::filesystem::path{});
        qError("Current Path: {}", std::filesystem::path{std::filesystem::current_path()});
    } while (false);

    return result;
}

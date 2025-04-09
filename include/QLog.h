#pragma once

#ifdef QNODEBUG

#define qTrace(fmt, ...)
#define qDebug(fmt, ...)
#define qInfo(fmt, ...)
#define qWarn(fmt, ...)
#define qError(fmt, ...)
#define qCritical(fmt, ...)

#else

#include <typeinfo>

#include "QObject.h"
#include "QSingletonProductor.h"
#ifdef _WIN32
#include "spdlog/sinks/wincolor_sink.h"
#else
#include "spdlog/sinks/ansicolor_sink.h"
#endif
#include "spdlog/spdlog.h"

template <>
struct fmt::formatter<std::filesystem::path> : public fmt::formatter<std::string> {
    auto format(std::filesystem::path const& path, format_context& ctx) const {
        return fmt::formatter<std::string>::format(path.string(), ctx);
    }
};

namespace qlib {
class QLogger : public QObject {
public:
    template <typename... Args>
    using format_string_t = spdlog::format_string_t<Args...>;

    template <typename String>
    QLogger(String&& name) {
#ifdef _WIN32
        auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
        auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif

        __logger =
            std::make_shared<spdlog::logger>(std::forward<String>(name), std::move(color_sink));
    }

    void set_level(size_t level) {
        __logger->set_level(static_cast<spdlog::level::level_enum>(level));
    }

    template <typename... Args>
    void trace(format_string_t<Args...> fmt, Args&&... args) {
        __logger->trace(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(format_string_t<Args...> fmt, Args&&... args) {
        __logger->debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(format_string_t<Args...> fmt, Args&&... args) {
        __logger->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(format_string_t<Args...> fmt, Args&&... args) {
        __logger->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(format_string_t<Args...> fmt, Args&&... args) {
        __logger->error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(format_string_t<Args...> fmt, Args&&... args) {
        __logger->critical(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log(size_t level, format_string_t<Args...> fmt, Args&&... args) {
        __logger->log(static_cast<spdlog::level::level_enum>(level), fmt,
                      std::forward<Args>(args)...);
    }

protected:
    std::shared_ptr<spdlog::logger> __logger;

    friend class QSingletonProductor<QLogger>;
};

static inline QLogger& default_logger() {
    return QSingletonProductor<QLogger>::get_instance("default");
}

};  // namespace qlib

#define qTrace(fmt, ...)                                                                           \
    qlib::default_logger().trace("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qDebug(fmt, ...)                                                                           \
    qlib::default_logger().debug("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qInfo(fmt, ...)                                                                            \
    qlib::default_logger().info("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qWarn(fmt, ...)                                                                            \
    qlib::default_logger().warn("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qError(fmt, ...)                                                                           \
    qlib::default_logger().error("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define qCritical(fmt, ...)                                                                        \
    qlib::default_logger().critical("[{}:{}]" fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#endif

#define qCTrace(fmt, ...) qTrace("{}: " fmt, typeid(*this).name(), ##__VA_ARGS__)
#define qCDebug(fmt, ...) qDebug("{}: " fmt, typeid(*this).name(), ##__VA_ARGS__)
#define qCInfo(fmt, ...) qInfo("{}: " fmt, typeid(*this).name(), ##__VA_ARGS__)
#define qCWarn(fmt, ...) qWarn("{}: " fmt, typeid(*this).name(), ##__VA_ARGS__)
#define qCError(fmt, ...) qError("{}: " fmt, typeid(*this).name(), ##__VA_ARGS__)
#define qCCritical(fmt, ...) qCritical("{}: " fmt, typeid(*this).name(), ##__VA_ARGS__)

#define qMTrace(fmt, ...) qTrace("{}: " fmt, __func__, ##__VA_ARGS__)
#define qMDebug(fmt, ...) qDebug("{}: " fmt, __func__, ##__VA_ARGS__)
#define qMInfo(fmt, ...) qInfo("{}: " fmt, __func__, ##__VA_ARGS__)
#define qMWarn(fmt, ...) qWarn("{}: " fmt, __func__, ##__VA_ARGS__)
#define qMError(fmt, ...) qError("{}: " fmt, __func__, ##__VA_ARGS__)
#define qMCritical(fmt, ...) qCritical("{}: " fmt, __func__, ##__VA_ARGS__)

#define qCMTrace(fmt, ...) qTrace("{}::{}: " fmt, typeid(*this).name(), __func__, ##__VA_ARGS__)
#define qCMDebug(fmt, ...) qDebug("{}::{}: " fmt, typeid(*this).name(), __func__, ##__VA_ARGS__)
#define qCMInfo(fmt, ...) qInfo("{}::{}: " fmt, typeid(*this).name(), __func__, ##__VA_ARGS__)
#define qCMWarn(fmt, ...) qWarn("{}::{}: " fmt, typeid(*this).name(), __func__, ##__VA_ARGS__)
#define qCMError(fmt, ...) qError("{}::{}: " fmt, typeid(*this).name(), __func__, ##__VA_ARGS__)
#define qCMCritical(fmt, ...)                                                                      \
    qCritical("{}::{}: " fmt, typeid(*this).name(), __func__, ##__VA_ARGS__)

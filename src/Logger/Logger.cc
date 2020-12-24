// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "Logger.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"

#include <cstdarg>

#ifdef WIN32
#include "spdlog/sinks/wincolor_sink.h"
#elif LINUX
#include "spdlog/sinks/ansicolor_sink.h"
#elif ANDROID
#include "spdlog/sinks/android_sink.h"
#endif

#define LOG_BUFFER_SIZE 512

namespace RealSenseID
{
namespace Logger
{
class LoggerWrapper
{
public:
    LoggerWrapper(LoggerWrapper const&) = delete;
    void operator=(LoggerWrapper const&) = delete;

    static LoggerWrapper& getInstance(const std::string& logName = "",
                                      const Logger::LogLevel loggerLevel = Logger::LogLevel::Debug,
                                      const std::string& logPath = "")
    {
        static LoggerWrapper instance(logName, loggerLevel, logPath);
        return instance;
    }

    std::shared_ptr<spdlog::logger> logger;

private:
    LoggerWrapper(const std::string& loggerName = "", const Logger::LogLevel loggerLevel = Logger::LogLevel::Debug,
                  const std::string& logPath = "")
    {
        std::vector<spdlog::sink_ptr> sinks;

        // file sink
        if (!logPath.empty())
        {
            sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath + "/session.log"));
        }

        // runtime sink
#ifdef WIN32
        sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
#elif LINUX
        sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
#elif ANDROID
        sinks.push_back(std::make_shared<spdlog::sinks::android_sink_mt>(loggerName));
#else
        throw std::runtime_error("Unsupported OS detected while trying to create Logger");
#endif

        logger = std::make_shared<spdlog::logger>(loggerName, sinks.begin(), sinks.end());

        switch (loggerLevel)
        {
        case Logger::LogLevel::Debug:
            logger->set_level(spdlog::level::debug);
            break;
        case Logger::LogLevel::Info:
            logger->set_level(spdlog::level::info);
            break;
        case Logger::LogLevel::Warning:
            logger->set_level(spdlog::level::warn);
            break;
        case Logger::LogLevel::Error:
            logger->set_level(spdlog::level::err);
            break;
        case Logger::LogLevel::Critical:
            logger->set_level(spdlog::level::critical);
            break;
        case Logger::LogLevel::Off:
            logger->set_level(spdlog::level::off);
            break;
        }
        logger->flush_on(spdlog::level::trace); // always flush
    }
};

void Initialize(const std::string& loggerName, const LogLevel loggerLevel, const std::string& logPath)
{
    LoggerWrapper::getInstance(loggerName, loggerLevel, logPath);
    LOG_DEBUG("Logger", "logger initialized, level: %d, path: %s", loggerLevel, logPath.c_str());
}

// if log level is right, vsprintf the args to buffer and log it
#define LOG_IT_(LEVEL)                                                                                                 \
    va_list args;                                                                                                      \
    auto& logger = LoggerWrapper::getInstance().logger;                                                                \
    if (!logger->should_log(LEVEL))                                                                                    \
        return;                                                                                                        \
    va_start(args, format);                                                                                            \
    char buffer[LOG_BUFFER_SIZE];                                                                                      \
    auto Ok = vsnprintf(buffer, sizeof(buffer), format, args) >= 0;                                                    \
    if (!Ok)                                                                                                           \
        snprintf(buffer, sizeof(buffer), "(bad printf format \"%s\")", format);                                        \
    logger->log(LEVEL, "[{}] {}", tag, buffer);                                                                        \
    va_end(args)


void Debug(const std::string& tag, const char* format, ...)
{
    LOG_IT_(spdlog::level::debug);
}

void Info(const std::string& tag, const char* format, ...)
{
    LOG_IT_(spdlog::level::info);
}

void Warning(const std::string& tag, const char* format, ...)
{
    LOG_IT_(spdlog::level::warn);
}

void Error(const std::string& tag, const char* format, ...)
{
    LOG_IT_(spdlog::level::err);
}

void Critical(const std::string& tag, const char* format, ...)
{
    LOG_IT_(spdlog::level::critical);
}


void DebugBytes(const std::string& tag, const char* msg, const char* buf, size_t size)
{
    auto& logger = LoggerWrapper::getInstance().logger;
    logger->debug("[{}] {} {} bytes {:pa}\n", tag, msg, size, spdlog::to_hex(buf, &buf[size]));
}
} // namespace Logger
} // namespace RealSenseID

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace RealSenseID
{
namespace Logger
{
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Off
};
//***********************************************************************************
// Initializes the logger (once per process, subsequent calls will be ignored).
//
// Parameter: loggerName - Tag name for the logger. Will appear in every line.
// Parameter: loggerLevel - Minimum level of logging messages to display.
// Parameter: logPath - Path to log file. Leave empty for no log file.
//***********************************************************************************
void Initialize(const std::string& loggerName, const LogLevel loggerLevel, const std::string& logPath);

//***********************************************************************************
// Writes a log message with "Debug" priority./
//***********************************************************************************
void Debug(const std::string& tag, const char* format, ...);

//***********************************************************************************
// Writes a log message with "Info" priority.
//***********************************************************************************
void Info(const std::string& tag, const char* format, ...);

//***********************************************************************************
// Writes a log message with "Warning" priority.
//***********************************************************************************
void Warning(const std::string& tag, const char* format, ...);

//***********************************************************************************
// Writes a log message with "Error" priority.
//***********************************************************************************
void Error(const std::string& tag, const char* format, ...);

//***********************************************************************************
// Writes a log message with "Critical" priority.
//***********************************************************************************
void Critical(const std::string& tag, const char* format, ...);

//***********************************************************************************
// Debug bytes as hex
//***********************************************************************************
void DebugBytes(const std::string& tag, const char* msg, const char* buffer, size_t size);

} // namespace Logger
} // namespace RealSenseID

#ifdef RSID_DEBUG_MODE
#define LOG_DEBUG(...)         Logger::Debug(__VA_ARGS__)
#define LOG_INFO(...)          Logger::Info(__VA_ARGS__)
#define LOG_WARNING(...)       Logger::Warning(__VA_ARGS__)
#define LOG_ERROR(...)         Logger::Error(__VA_ARGS__)
#define LOG_CRITICAL(...)      Logger::Critical(__VA_ARGS__)
#define LOG_EXCEPTION(tag, ex) Logger::Error(tag, "%s", ex.what())
#else
#define LOG_DEBUG(...)         (void)0
#define LOG_INFO(...)          (void)0
#define LOG_WARNING(...)       (void)0
#define LOG_ERROR(...)         (void)0
#define LOG_CRITICAL(...)      (void)0
#define LOG_EXCEPTION(tag, ex) (void)ex
#endif

#ifdef RSID_DEBUG_SERIAL
#define DEBUG_SERIAL(tag, msg, buf, size) Logger::DebugBytes(tag, msg, buf, size)
#else
#define DEBUG_SERIAL(...) (void)0
#endif
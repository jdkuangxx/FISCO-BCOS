#include "Log.h"
#include <boost/log/core.hpp>
namespace base
{
std::string const FileLogger = "FileLogger";
boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level, std::string>
    FileLoggerHandler(boost::log::keywords::channel = FileLogger);

std::string const StatFileLogger = "StatFileLogger";
boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level, std::string>
    StatFileLoggerHandler(boost::log::keywords::channel = StatFileLogger);

LogLevel c_fileLogLevel = LogLevel::TRACE;
LogLevel c_statLogLevel = LogLevel::INFO;

void setFileLogLevel(LogLevel const& _level)
{
    c_fileLogLevel = _level;
}

void setStatLogLevel(LogLevel const& _level)
{
    c_statLogLevel = _level;
}
}
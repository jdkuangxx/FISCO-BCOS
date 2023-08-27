#ifndef __BOOST_LOG_H
#define __BOOST_LOG_H

#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/trivial.hpp>

#ifndef LOG_BADGE
#define LOG_BADGE(_NAME) "[" << (_NAME) << "]"
#endif

#ifndef LOG_TYPE
#define LOG_TYPE(_TYPE) (_TYPE) << "|"
#endif

#ifndef LOG_DESC
#define LOG_DESC(_DESCRIPTION) (_DESCRIPTION)
#endif

#ifndef LOG_KV
#define LOG_KV(_K, _V) "," << (_K) << "=" << (_V)
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace base
{
extern std::string const FileLogger;
/// the file logger
extern boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level,
    std::string>
    FileLoggerHandler;

// the statFileLogger
extern std::string const StatFileLogger;
extern boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level,
    std::string>
    StatFileLoggerHandler;

enum LogLevel
{
    TRACE = boost::log::trivial::trace,
    DEBUG = boost::log::trivial::debug,
    INFO = boost::log::trivial::info,
    WARNING = boost::log::trivial::warning,
    ERROR = boost::log::trivial::error,
    FATAL = boost::log::trivial::fatal,
};

extern LogLevel c_fileLogLevel;
extern LogLevel c_statLogLevel;

void setFileLogLevel(LogLevel const& _level);
void setStatLogLevel(LogLevel const& _level);

#define LOG(level)                                \
    if (base::LogLevel::level >= base::c_fileLogLevel) \
    BOOST_LOG_SEV(                                     \
        base::FileLoggerHandler, (boost::log::trivial::severity_level)(base::LogLevel::level))
}

#endif
/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief: setting log
 *
 * @file: BoostLogInitializer.h
 * @author: yujiechen
 */

#ifndef __BOOST_LOG_INITIALIZER_H
#define __BOOST_LOG_INITIALIZER_H

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <functional>
#include <atomic>

#include "BoostLog.h"

namespace alpha
{

struct LogOption {
    // 是否开启日志
    bool enable = true;
    // 日志等级
    std::string log_level = "info";
    // 日志路径
    std::string log_path = "./log";
    // 单个日志最大大小
    size_t max_log_size = 200;
    // 是否按小时切换日志
    bool enable_rotate_by_hour = true;
    // 是否输出到控制台
    bool enable_console_output = true;
    bool need_flush = true;
    bool print_tid = false;
};

class BoostLogInitializer
{
public:
    class Sink : public boost::log::sinks::text_file_backend {
    public:
        void consume(const boost::log::record_view& rec, const std::string& str) {
            boost::log::sinks::text_file_backend::consume(rec, str);
            auto severity =
                rec.attribute_values()[boost::log::aux::default_attribute_names::severity()]
                    .extract<boost::log::trivial::severity_level>();
            // bug fix: determine m_ptr before get the log level
            //          serverity.get() will call  BOOST_ASSERT(m_ptr)
            if (severity.get_ptr() && severity.get() == boost::log::trivial::severity_level::fatal) {
                // abort if encounter fatal, will generate coredump
                // must make sure only use LOG(FATAL) when encounter the most serious problem
                // forbid use LOG(FATAL) in the function that should exit normally
                std::abort();
            }
        }
    };
    class ConsoleSink : public boost::log::sinks::text_ostream_backend {
    public:
        void consume(const boost::log::record_view& rec, const std::string& str) {
            boost::log::sinks::text_ostream_backend::consume(rec, str);
            auto severity =
                rec.attribute_values()[boost::log::aux::default_attribute_names::severity()]
                    .extract<boost::log::trivial::severity_level>();
            // bug fix: determine m_ptr before get the log level
            //          serverity.get() will call  BOOST_ASSERT(m_ptr)
            if (severity.get_ptr() && severity.get() == boost::log::trivial::severity_level::fatal) {
                // abort if encounter fatal, will generate coredump
                // must make sure only use LOG(FATAL) when encounter the most serious problem
                // forbid use LOG(FATAL) in the function that should exit normally
                std::abort();
            }
        }
    };
    using Ptr = std::shared_ptr<BoostLogInitializer>;
    using sink_t = boost::log::sinks::asynchronous_sink<Sink>;
    using console_sink_t = boost::log::sinks::asynchronous_sink<ConsoleSink>;

private:
    BoostLogInitializer() {}

public:
    virtual ~BoostLogInitializer() { stopLogging(); }

    void initLog(const LogOption& option, std::string const& _logger = FileLogger,
        std::string const& _logPrefix = "log");
    
    void initLog(const std::string& _configFile, std::string const& _logger = FileLogger,
        std::string const& _logPrefix = "log");

    void initStatLog(const LogOption& option,
        std::string const& _logger = StatFileLogger, std::string const& _logPrefix = "stat");

    void stopLogging();

    static unsigned getLogLevel(std::string const& levelStr);

    void setLogPath(std::string const& _logPath) { m_logPath = _logPath; }
    std::string logPath() const { return m_logPath; }


    void setPraseLogOptionFunc(const std::function<void(const std::string& cfgFile, LogOption& option)>& func) {
        m_praseLogOptionFunc = func;
    }

    static BoostLogInitializer* getInstance();

    static void initializeLog(const LogOption& option, std::string const& _logger = FileLogger,
        std::string const& _logPrefix = "log");

    static void initializeLog(const std::string& cfgPath, std::string const& _logger = FileLogger,
        std::string const& _logPrefix = "log");

    static void registerPraseLogOptionFunc(const std::function<void(const std::string& cfgFile, LogOption& option)>& func) {
        getInstance()->setPraseLogOptionFunc(func);
    } 

    void praseOption(const std::string& cfgFile, LogOption& option) {
        if (m_praseLogOptionFunc) {
            m_praseLogOptionFunc(cfgFile, option);
        }
    }

private:
    bool canRotate(size_t const& _index);

    boost::shared_ptr<sink_t> initLogSink(const LogOption& option,
        unsigned const& _logLevel, std::string const& _logPath, std::string const& _logPrefix,
        std::string const& channel);

    // rotate the log file the log every hour
    boost::shared_ptr<sink_t> initHourLogSink(const LogOption& option,
        unsigned const& _logLevel, std::string const& _logPath, std::string const& _logPrefix,
        std::string const& channel);

    boost::shared_ptr<console_sink_t> initConsoleLogSink(const LogOption& option,
        unsigned const& _logLevel, std::string const& channel);

    template <typename T>
    void setLogFormatter(T _sink, bool show_tid) {
        /// set file format
        /// log-level|timestamp | message
        if (show_tid) {
            _sink->set_formatter(
                boost::log::expressions::stream
                << boost::log::expressions::attr<boost::log::trivial::severity_level>("Severity") 
                << "|"
                << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type >("ThreadID")
                << "|"
                << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                << "|" 
                << boost::log::expressions::smessage);
        } else {
            _sink->set_formatter(
                boost::log::expressions::stream
                << boost::log::expressions::attr<boost::log::trivial::severity_level>("Severity") 
                << "|"
                << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                << "|" 
                << boost::log::expressions::smessage);
        }
    }

    template <typename T>
    void stopLogging(boost::shared_ptr<T> sink) {
        if (!sink) {
            return;
        }
        // remove the sink from the core, so that no records are passed to it
        boost::log::core::get()->remove_sink(sink);
        // break the feeding loop
        sink->stop();
        // flush all log records that may have left buffered
        sink->flush();
        sink.reset();
    }

private:
    void stopLogging(boost::shared_ptr<sink_t> sink);
    std::vector<boost::shared_ptr<sink_t>> m_sinks;
    std::vector<boost::shared_ptr<console_sink_t>> m_consoleSinks;

    std::function<void(const std::string& cfgFile, LogOption& option)> m_praseLogOptionFunc;
    std::vector<int> m_currentHourVec;
    std::string m_logPath;
    std::atomic_bool m_running = {false};
    std::atomic<bool> isInited = {false};
};
}
#endif
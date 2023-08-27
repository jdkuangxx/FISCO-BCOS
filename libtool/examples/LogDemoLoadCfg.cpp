#include "libtool/Log.h"
#include "libtool/BoostLogInitializer.h"
#include <iostream>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage:\n"
                  << "./LogDemoLoadCfg [cfg]\n";
        return -1;
    }
    // 1. 設定讀取配置文件的方法
    base::BoostLogInitializer::registerPraseLogOptionFunc(
        [](const std::string& cfgFile, base::LogOption& option) {
            boost::property_tree::ptree _pt;
            boost::property_tree::read_ini(cfgFile, _pt);
            option.enable = _pt.get<bool>("log.enable", true);
            option.log_level = _pt.get<std::string>("log.level", "info");
            option.log_path = _pt.get<std::string>("log.log_path", "./log");
            option.max_log_size = _pt.get<uint64_t>("log.max_log_size", 200);
            option.enable_rotate_by_hour = _pt.get<bool>("enable_rotate_by_hour", true);
            option.enable_console_output = _pt.get<bool>("enable_console_output", false);
            option.need_flush = _pt.get<bool>("log.need_flush", true);
            option.print_tid = _pt.get<bool>("log.print_tid", true);
        });
    // 2. 使用配置文件来初始化Log
    base::BoostLogInitializer::initializeLog(std::string(argv[1]));
    // 3. 使用Log
    LOG(TRACE) << "trace msg";
    LOG(DEBUG) << "debug msg";
    LOG(INFO) << "info msg";
    LOG(WARNING) << "warnning msg";
    LOG(ERROR) << "error msg";
    // std::this_thread::sleep_for(std::chrono::seconds(30));
    // LOG(INFO) << "sleep.." << "444 jdd";
    // LOG(ERROR) << "sleep.." << "555 jdd";
    return 0;
}
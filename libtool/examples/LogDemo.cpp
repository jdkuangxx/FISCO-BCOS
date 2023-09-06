#include "libtool/Log.h"
#include "libtool/BoostLogInitializer.h"

using namespace alpha;

int main() {
    // 1. 在代码中定义Option
    alpha::LogOption option;
    option.print_tid = true;
    option.enable_console_output = true;
    // 2. 使用定义的Option来初始化Log
    alpha::BoostLogInitializer::initializeLog(option);
    // 3. 使用Log
    LOG(TRACE) << "trace msg";
    LOG(DEBUG) << "debug msg";
    LOG(INFO) << "info msg";
    LOG(WARNING) << "warnning msg";
    LOG(ERROR) << "error msg";
}
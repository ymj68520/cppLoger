#include"Logger.hpp"


int main(void){
    // 全局设置一次即可

    // 设置日志级别为DEBUG
    Logger::getInstance().setLevel(LogLevel::DEBUG);
    // 设置日志输出到控制台
    Logger::getInstance().setConsole(true);
    // 设置日志输出到文件
    Logger::getInstance().setFile(true, "app.log");

    LOG_DEBUG << "This is a debug message.";
    LOG_INFO << "This is an info message.";
    LOG_WARNING << "This is a warning message.";
    LOG_ERROR << "This is an error message.";
    LOG_FATAL << "This is a fatal message.";
    return 0;
}
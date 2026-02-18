/**
 * @file main.cpp
 * @brief CppLogger 示例程序
 * @author ymj68520
 * @date 2026-02-18
 */

#include "Logger.hpp"

int main(void) {
    // 全局配置（只需设置一次）

    // 设置日志级别为 DEBUG
    Logger::getInstance().setLevel(LogLevel::DEBUG);

    // 设置日志输出到控制台
    Logger::getInstance().setConsole(true);

    // 设置日志输出到文件（文件名会自动添加日期后缀）
    Logger::getInstance().setFile(true, "app.log");

    // 使用流式接口记录日志
    Logger::debug() << "这是调试信息";
    Logger::info() << "这是一般信息";
    Logger::warning() << "这是警告信息";
    Logger::error() << "这是错误信息";
    Logger::fatal() << "这是致命错误";

    // 支持多种类型
    int value = 42;
    double pi = 3.14159;
    bool flag = true;
    void* ptr = &value;

    Logger::info() << "整数: " << value << "\n"
                  << ", 浮点: " << pi   << "\n"
                  << ", 布尔: " << flag << "\n"
                  << ", 指针: " << ptr  << "\n";

    return 0;
}

#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <ctime>
#include <iomanip>

 /*
    * 日志级别枚举
    * DEBUG: 调试信息
    * INFO: 一般信息
    * WARNING: 警告信息
    * ERROR: 错误信息
    * @author: zaoweiceng
    * @date: 2025-03-28
    */
   enum LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR
};
LogLevel LogLevelFromString(const std::string& levelStr){
    if (levelStr == "DEBUG") return LogLevel::DEBUG;
    if (levelStr == "INFO") return LogLevel::INFO;
    if (levelStr == "WARNING") return LogLevel::WARNING;
    if (levelStr == "ERROR") return LogLevel::ERROR;
    return LogLevel::INFO;
}
std::string levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}
std::string levelToStringConsole(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "\033[34mDEBUG\033[0m";   // Blue
        case LogLevel::INFO:    return "\033[32mINFO\033[0m";    // Green
        case LogLevel::WARNING: return "\033[33mWARNING\033[0m"; // Yellow
        case LogLevel::ERROR:   return "\033[31mERROR\033[0m";   // Red
        default:                             return "UNKNOWN";  // Reset
    }
}
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    #ifdef _WIN32
    localtime_s(&tm, &time);
    #else
    localtime_r(&time, &tm);
    #endif
    std::stringstream ss;
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    ss << buffer;
    return ss.str();
}

std::string formatMessage(LogLevel level, const std::string& message, const std::string& file, int line) {
    std::stringstream ss;
    ss << getCurrentTime() << " [" << levelToString(level) << "] "
       << file << ":" << line << " - " << message;
    return ss.str();
}

std::string formatMessageConsole(LogLevel level, const std::string& message, const std::string& file, int line) {
    std::stringstream ss;
    ss << getCurrentTime() << " [" << levelToStringConsole(level) << "] "
       << file << ":" << line << " - " << message;
    return ss.str();
}

std::string timeToString(const unsigned long long& time, const std::string& format){
    std::time_t t = static_cast<std::time_t>(time);
    std::tm tm = *std::localtime(&t);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), format.c_str(), &tm);
    return std::string(buffer);
}

/*
* 日志类
* 用于日志的输出
* @author: zaoweiceng
* @data: 2025-03-28
*/
class Logger{
    public:
        // 获取单例对象
        // 全局均使用getInstance()获取Logger对象
        // 线程安全
        static Logger& getInstance(){
            static Logger instance;
            return instance;
        }
        // 设置日志级别
        // 传入日志级别
        void setLevel(LogLevel level){
            std::lock_guard<std::mutex> lock(__mutex);
            __level = level;
        }
        // 获取日志级别
        // 返回当前日志级别
        LogLevel getLevel() const{
            return __level;
        }
        // 设置日志输出方式
        // 传入是否输出到控制台
        void setConsole(bool console){
            std::lock_guard<std::mutex> lock(__mutex);
            __console = console;
        }
        // 设置日志输出方式
        // 传入是否输出到文件
        // 传入文件路径
        void setFile(bool file, const std::string& file_path){
            std::lock_guard<std::mutex> lock(__mutex);
            __file = file;
            __file_path = file_path;
            if (__file && __file_path != ""){
                __file_path = __file_path.find_last_not_of('.') == std::string::npos 
                ? __file_path + "-" + timeToString(std::time(nullptr), "%Y%m%d") + ".log"
                : __file_path.substr(0, __file_path.find_last_of('.')) + "-" +  timeToString(std::time(nullptr), "%Y%m%d") + ".log";
                __file_stream.open(__file_path, std::ios::out | std::ios::app);
            }

        }
        // 记录日志
        // 传入日志级别，日志内容，文件名，行号
        void log(LogLevel level, const std::string& message, const std::string& file, int line){
            std::lock_guard<std::mutex> lock(__mutex);
            if (__console) {
                std::cout << formatMessageConsole(level, message, file, line) << std::endl;
            }
            if (__file) {
                // 每天自动将日志保存到新的文件中
                if (std::time(nullptr) - __time > 60*60*24) {
                    __file_stream.close();
                    __time = std::time(nullptr);
                }
                if (!__file_stream.is_open()) {
                    __file_path = __file_path.find_last_not_of('-') == std::string::npos 
                                ? __file_path + "-" + timeToString(std::time(nullptr), "%Y%m%d") + ".log"
                                : __file_path.substr(0, __file_path.find_last_of('-')) + "-" +  timeToString(std::time(nullptr), "%Y%m%d") + ".log";
                    __file_stream.open(__file_path, std::ios::app);
                }
                if (__file_stream.good()) {
                    __file_stream << formatMessage(level, message, file, line) << std::endl;
                }
            }
        }
    
    private:
        // 互斥锁，用于文件输出的线程安全
        std::mutex __mutex;
        // 日志级别
        LogLevel __level = [](const std::string& levelStr) {
            if (levelStr == "DEBUG") return LogLevel::DEBUG;
            if (levelStr == "INFO") return LogLevel::INFO;
            if (levelStr == "WARNING") return LogLevel::WARNING;
            if (levelStr == "ERROR") return LogLevel::ERROR;
            return LogLevel::INFO;
        }("INFO");
        // 是否输出到控制台
        bool __console = true;
        
        // 是否输出到文件
        bool __file = false;
        // 文件路径
        std::string __file_path = "./app.log";
        // 文件流
        std::ofstream __file_stream;
        // 时间记录
        unsigned long long __time = std::time(nullptr);
        // 构造函数
        // 私有化构造函数，禁止外部创建对象
        // 使用单例模式
        // 初始化日志级别为INFO
        // 初始化输出方式为控制台
        // 初始化文件路径为空
        Logger():__level(LogLevel::INFO), __console(true), __file(false), __file_path(""){
            if (__file && __file_path != "")
                __file_stream.open(__file_path, std::ios::out | std::ios::app);
        }
        Logger(Logger const&) = delete;
        Logger& operator=(Logger const&) = delete;
        ~Logger(){}
};
/*
* 日志流类
* 用于日志的输出
* 结合宏定义，简化日志类的使用
* @author: zaoweiceng
* @date: 2025-03-28
*/
class LogStream {
    public:
        LogStream(LogLevel level, const std::string& file, int line)
            : level_(level), file_(file), line_(line) {}
        
        ~LogStream() {
            Logger::getInstance().log(level_, ss_.str(), file_, line_);
        }
        
        template<typename T>
        LogStream& operator<<(const T& msg) {
            ss_ << msg;
            return *this;
        }
        
    private:
        LogLevel level_;
        std::stringstream ss_;
        std::string file_;
        int line_;
};
// 日志宏定义
#define LOG(level) \
        if (level < Logger::getInstance().getLevel()) {} \
        else LogStream(level, __FILE__, __LINE__)
// Debug日志
#define LOG_DEBUG LOG(LogLevel::DEBUG)
// Info日志
#define LOG_INFO LOG(LogLevel::INFO)
// Warning日志
#define LOG_WARNING LOG(LogLevel::WARNING)
// Error日志
#define LOG_ERROR LOG(LogLevel::ERROR)
// Fatal日志
#define LOG_FATAL LOG(LogLevel::ERROR) << "FATAL ERROR: " << __FILE__ << ":" << __LINE__ << " - "

#endif
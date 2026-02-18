/**
 * @file Logger.hpp
 * @brief 轻量级线程安全的 C++ 日志库
 * @details 提供控制台彩色输出、文件日志记录、按日期自动轮转等功能
 *          使用单例模式，支持流式日志接口，性能优化避免频繁内存分配
 *          支持 Logger::endl 和 Logger::flush 操纵符
 *
 * @author ymj68520
 * @date 2026-02-18
 */

#ifndef C_LOGGER_HPP
#define C_LOGGER_HPP

#include <iostream>
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <memory>
#include <type_traits>
#include <charconv>
#include <source_location>

/**
 * @brief 日志级别枚举
 * @details 级别从低到高：DEBUG < INFO < WARNING < ERROR
 *          设置日志级别后，低于该级别的日志将被过滤
 */
enum LogLevel {
    DEBUG,   ///< 调试信息
    INFO,    ///< 一般信息
    WARNING, ///< 警告信息
    ERROR    ///< 错误信息
};

/**
 * @brief 将日志级别转换为字符串
 * @param level 日志级别
 * @return 级别对应的字符串常量
 */
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case DEBUG:   return "DEBUG";
        case INFO:    return "INFO";
        case WARNING: return "WARNING";
        case ERROR:   return "ERROR";
        default:      return "UNKNOWN";
    }
}

/**
 * @brief 获取日志级别对应的 ANSI 颜色代码
 * @param level 日志级别
 * @return ANSI 转义码字符串
 */
inline const char* logLevelToColorCode(LogLevel level) {
    switch (level) {
        case DEBUG:   return "\033[34m"; // 蓝色
        case INFO:    return "\033[32m"; // 绿色
        case WARNING: return "\033[33m"; // 黄色
        case ERROR:   return "\033[31m"; // 红色
        default:      return "";         // 无颜色
    }
}

// 前置声明
class LogStream;

/**
 * @brief 标准流操纵符包装类
 * @details 用于支持 endl、flush 等流操纵符
 */
class StdManipulator {
public:
    enum Type {
        Endl,   ///< 换行符
        Flush,  ///< 刷新（仅换行，实际刷新在析构时完成）
        None    ///< 无操作
    };

    constexpr StdManipulator(Type type = None) : type_(type) {}

    constexpr Type getType() const { return type_; }

private:
    Type type_;
};

/**
 * @brief 全局 endl 操纵符
 * @details 用法：Logger::info() << "message" << ::endl;
 */
constexpr StdManipulator endl(StdManipulator::Endl);

/**
 * @brief 全局 flush 操纵符
 * @details 用法：Logger::info() << "message" << ::flush;
 */
constexpr StdManipulator flush(StdManipulator::Flush);

/**
 * @brief Logger 日志类（单例模式）
 * @details 线程安全的日志记录器，支持：
 *          - 控制台彩色输出
 *          - 文件日志记录（按日期自动轮转）
 *          - 日志级别过滤
 *          - 使用 std::atomic 和 std::mutex 保证线程安全
 */
class Logger {
public:
    /**
     * @brief 获取 Logger 单例实例
     * @return Logger 实例的引用
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    /**
     * @brief 设置日志级别
     * @param level 日志级别，低于此级别的日志将被过滤
     */
    void setLevel(LogLevel level) {
        level_.store(level);
    }

    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    LogLevel getLevel() const {
        return level_.load();
    }

    /**
     * @brief 设置是否输出到控制台
     * @param console true 启用控制台输出，false 禁用
     */
    void setConsole(bool console) {
        std::lock_guard<std::mutex> lock(mutex_);
        console_ = console;
    }

    /**
     * @brief 设置文件日志输出
     * @param enable true 启用文件输出，false 禁用
     * @param filePath 日志文件路径，文件名会自动添加日期后缀（如：-20260218.log）
     */
    void setFile(bool enable, const std::string& filePath) {
        std::lock_guard<std::mutex> lock(mutex_);
        fileEnabled_ = enable;
        baseFilePath_ = filePath;

        if (enable && !baseFilePath_.empty()) {
            openLogFile();
        } else {
            closeLogFile();
        }
    }

    /**
     * @brief 核心日志记录函数
     * @param level 日志级别
     * @param message 日志消息内容
     * @param file 源文件名
     * @param line 源代码行号
     */
    void log(LogLevel level, const char* message, const char* file, int line) {
        // 快速检查：级别过低则直接返回（无锁）
        if (level < level_.load()) return;

        std::lock_guard<std::mutex> lock(mutex_);

        // 时间处理（缓存优化，同一秒内不重复格式化）
        std::time_t now = std::time(nullptr);
        if (now != lastTime_) {
            updateTimeStr(now);
            lastTime_ = now;
        }

        // 1. 控制台输出（带颜色）
        if (console_) {
            const char* color = logLevelToColorCode(level);
            const char* reset = "\033[0m";
            const char* levelStr = logLevelToString(level);

            // 格式：YYYY-MM-DD HH:MM:SS [LEVEL] file:line - message
            fprintf(stdout, "%s [%s%s%s] %s:%d - %s\n",
                    timeStr_, color, levelStr, reset, file, line, message);
        }

        // 2. 文件输出
        if (fileEnabled_ && fileHandle_) {
            // 检查是否需要轮转（超过 24 小时）
            if (now - fileOpenTime_ > 60 * 60 * 24) {
                openLogFile(); // 重新打开文件（触发轮转）
            }

            // 文件句柄无效时尝试重新打开
            if (!fileHandle_) {
                openLogFile();
            }

            if (fileHandle_) {
                fprintf(fileHandle_, "%s [%s] %s:%d - %s\n",
                        timeStr_, logLevelToString(level), file, line, message);
                fflush(fileHandle_); // 确保数据写入磁盘
            }
        }
    }

    // 静态辅助方法：创建流式日志接口
    static LogStream debug(const std::source_location& loc = std::source_location::current());
    static LogStream info(const std::source_location& loc = std::source_location::current());
    static LogStream warning(const std::source_location& loc = std::source_location::current());
    static LogStream error(const std::source_location& loc = std::source_location::current());
    static LogStream fatal(const std::source_location& loc = std::source_location::current());

    // 流操纵符
    static constexpr StdManipulator endl{StdManipulator::Endl};
    static constexpr StdManipulator flush{StdManipulator::Flush};

private:
    std::mutex mutex_;           ///< 互斥锁，保护共享状态
    std::atomic<LogLevel> level_; ///< 原子变量，日志级别（无锁读写）
    bool console_;               ///< 是否输出到控制台
    bool fileEnabled_;           ///< 是否启用文件输出
    std::string baseFilePath_;   ///< 基础文件路径
    FILE* fileHandle_;           ///< 文件句柄
    std::time_t fileOpenTime_;   ///< 文件打开时间
    std::time_t lastTime_;       ///< 上次更新时间字符串的时间
    char timeStr_[32];           ///< 格式化后的时间字符串缓存

    /**
     * @brief 私有构造函数（单例模式）
     */
    Logger()
        : level_(LogLevel::INFO), console_(true), fileEnabled_(false),
          fileHandle_(nullptr), fileOpenTime_(0), lastTime_(0) {
        std::memset(timeStr_, 0, sizeof(timeStr_));
    }

    /**
     * @brief 析构函数
     */
    ~Logger() {
        closeLogFile();
    }

    // 禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief 更新时间字符串缓存
     * @param t 时间值
     */
    void updateTimeStr(std::time_t t) {
        std::tm tm_buf;
        #ifdef _WIN32
        localtime_s(&tm_buf, &t);
        #else
        localtime_r(&t, &tm_buf);
        #endif
        std::strftime(timeStr_, sizeof(timeStr_), "%Y-%m-%d %H:%M:%S", &tm_buf);
    }

    /**
     * @brief 关闭日志文件
     */
    void closeLogFile() {
        if (fileHandle_) {
            fclose(fileHandle_);
            fileHandle_ = nullptr;
        }
    }

    /**
     * @brief 打开日志文件（支持按日期轮转）
     */
    void openLogFile() {
        closeLogFile(); // 先关闭已存在的文件

        std::time_t now = std::time(nullptr);
        std::tm tm_buf;
        #ifdef _WIN32
        localtime_s(&tm_buf, &now);
        #else
        localtime_r(&now, &tm_buf);
        #endif
        char dateSuffix[16];
        std::strftime(dateSuffix, sizeof(dateSuffix), "-%Y%m%d.log", &tm_buf);

        std::string finalPath = baseFilePath_;
        // 在扩展名前插入日期，或在末尾添加
        size_t dotPos = baseFilePath_.find_last_of('.');
        if (dotPos == std::string::npos) {
            finalPath += dateSuffix;
        } else {
            finalPath = baseFilePath_.substr(0, dotPos) + dateSuffix;
        }

        #ifdef _WIN32
        fopen_s(&fileHandle_, finalPath.c_str(), "a");
        #else
        fileHandle_ = fopen(finalPath.c_str(), "a");
        #endif

        if (fileHandle_) {
            fileOpenTime_ = now;
        }
    }
};

/**
 * @brief LogStream 流式日志类
 * @details 使用栈缓冲区避免内存分配，支持链式调用
 *          在析构时自动将日志内容输出到 Logger
 */
class LogStream {
public:
    /**
     * @brief 构造函数
     * @param level 日志级别
     * @param file 源文件名
     * @param line 源代码行号
     */
    LogStream(LogLevel level, const char* file, int line)
        : level_(level), file_(file), line_(line), offset_(0) {
        buffer_[0] = '\0';
    }

    /**
     * @brief 析构函数：将缓冲区内容输出到 Logger
     */
    ~LogStream() {
        Logger::getInstance().log(level_, buffer_, file_, line_);
    }

    /**
     * @brief 支持算术类型的流式输出
     * @tparam T 算术类型（int, float, double, bool, char 等）
     */
    template <typename T> requires std::is_arithmetic_v<T>
    LogStream& operator<<(T val) {
        if constexpr (std::is_same_v<T, bool>) {
            append(val ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, char>) {
            if (offset_ < BUFFER_SIZE - 1) {
                buffer_[offset_++] = val;
                buffer_[offset_] = '\0';
            }
        }
        else if constexpr (std::is_floating_point_v<T>) {
            char tmp[32];
            int len = snprintf(tmp, sizeof(tmp), "%.4f", val);
            if (len > 0) append(tmp);
        }
        else {
            // 使用 std::to_chars 高效转换整数
            if (offset_ < BUFFER_SIZE - 24) {
                auto res = std::to_chars(buffer_ + offset_, buffer_ + BUFFER_SIZE, val);
                if (res.ec == std::errc()) {
                    offset_ = res.ptr - buffer_;
                    buffer_[offset_] = '\0';
                }
            }
        }
        return *this;
    }

    /**
     * @brief C 字符串输出
     */
    LogStream& operator<<(const char* val) {
        if (val) append(val);
        return *this;
    }

    /**
     * @brief std::string 输出
     */
    LogStream& operator<<(const std::string& val) {
        append(val.c_str());
        return *this;
    }

    /**
     * @brief 指针输出（十六进制格式）
     */
    LogStream& operator<<(const void* val) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%p", val);
        append(tmp);
        return *this;
    }

    /**
     * @brief 支持流操纵符（Logger::endl, Logger::flush）
     * @param manip 操纵符包装器
     * @return 自身引用，支持链式调用
     * @details 提供 Logger::endl 和 Logger::flush 替代 std::endl 和 std::flush
     *          使用方式：Logger::info() << "message" << Logger::endl;
     */
    LogStream& operator<<(StdManipulator manip) {
        if (manip.getType() == StdManipulator::Endl ||
            manip.getType() == StdManipulator::Flush) {
            append("\n");
        }
        return *this;
    }

    /**
     * @brief 支持换行符字符（允许直接使用 '\n'）
     */
    LogStream& operator<<(char c) {
        if (offset_ < BUFFER_SIZE - 1) {
            buffer_[offset_++] = c;
            buffer_[offset_] = '\0';
        }
        return *this;
    }

private:
    static const int BUFFER_SIZE = 4096; ///< 缓冲区大小
    char buffer_[BUFFER_SIZE];           ///< 字符缓冲区
    int offset_;                         ///< 当前写入位置
    LogLevel level_;                     ///< 日志级别
    const char* file_;                   ///< 源文件名
    int line_;                           ///< 源代码行号

    /**
     * @brief 向缓冲区追加字符串
     * @param str 要追加的字符串
     */
    void append(const char* str) {
        int len = 0;
        while (str[len] && offset_ + len < BUFFER_SIZE - 1) {
            buffer_[offset_ + len] = str[len];
            len++;
        }
        offset_ += len;
        buffer_[offset_] = '\0';
    }
};

// Logger 静态方法的实现（需在 LogStream 定义之后）
inline LogStream Logger::debug(const std::source_location& loc) {
    return LogStream(DEBUG, loc.file_name(), loc.line());
}

inline LogStream Logger::info(const std::source_location& loc) {
    return LogStream(INFO, loc.file_name(), loc.line());
}

inline LogStream Logger::warning(const std::source_location& loc) {
    return LogStream(WARNING, loc.file_name(), loc.line());
}

inline LogStream Logger::error(const std::source_location& loc) {
    return LogStream(ERROR, loc.file_name(), loc.line());
}

inline LogStream Logger::fatal(const std::source_location& loc) {
    LogStream stream(ERROR, loc.file_name(), loc.line());
    stream << "FATAL ERROR: ";
    return stream;
}

#endif // C_LOGGER_HPP

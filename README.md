# CppLogger

<div align="center">

**一个轻量级、线程安全的 C++ 日志库**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Tests](https://img.shields.io/badge/Tests-72%20Passed-green.svg)](#测试)

</div>

---

## 简介

CppLogger 是一个轻量级的 C++ 日志库，采用 header-only 设计，开箱即用。它提供了线程安全的日志记录功能，支持控制台彩色输出、文件日志记录（按日期自动轮转）、流式日志接口等特性。

### 特性

- 线程安全 - 使用 `std::mutex` 和 `std::atomic` 保证多线程环境下的安全性
- 性能优化 - 使用栈缓冲区避免频繁内存分配
- 流式接口 - 支持 `Logger::info() << "message" << 123;` 风格
- 控制台彩色输出 - 不同日志级别使用不同颜色
- 文件日志 - 按日期自动轮转（每天生成新文件）
- 零依赖 - 仅依赖标准库
- Header-only - 单个头文件即可使用
- C++20 支持 - 使用 `std::source_location` 自动获取文件名和行号

---

## 编译要求

- **C++20** 或更高版本
- 支持 C++20 的编译器（GCC 10+, Clang 10+, MSVC 2019+）
- CMake 3.16+ （仅用于构建测试）

---

## 快速开始

### 基本使用

只需引入 `Logger.hpp` 头文件即可使用：

```cpp
#include "Logger.hpp"

int main() {
    // 全局配置（只需初始化一次）
    Logger::getInstance().setLevel(LogLevel::DEBUG);      // 设置日志级别
    Logger::getInstance().setConsole(true);                 // 启用控制台输出
    Logger::getInstance().setFile(true, "app.log");        // 启用文件输出

    // 使用流式接口记录日志
    Logger::debug() << "这是调试信息";
    Logger::info() << "这是一般信息";
    Logger::warning() << "这是警告信息";
    Logger::error() << "这是错误信息";
    Logger::fatal() << "这是致命错误";

    return 0;
}
```

### 日志级别

CppLogger 提供四个日志级别（由低到高）：

| 级别 | 说明 | 颜色 |
|:----:|:----:|:----:|
| `DEBUG` | 调试信息 | 蓝色 |
| `INFO` | 一般信息 | 绿色 |
| `WARNING` | 警告信息 | 黄色 |
| `ERROR` | 错误信息 | 红色 |

设置日志级别后，低于该级别的日志将被过滤：

```cpp
Logger::getInstance().setLevel(LogLevel::WARNING);
// DEBUG 和 INFO 级别的日志将不会输出
```

---

## API 文档

### 配置方法

```cpp
// 设置日志级别
Logger::getInstance().setLevel(LogLevel::INFO);

// 获取当前日志级别
LogLevel level = Logger::getInstance().getLevel();

// 设置是否输出到控制台
Logger::getInstance().setConsole(true);   // 启用
Logger::getInstance().setConsole(false);  // 禁用

// 设置文件日志输出
Logger::getInstance().setFile(true, "myapp.log");  // 启用
Logger::getInstance().setFile(false, "");         // 禁用
```

### 日志输出

```cpp
// 流式接口（推荐）
Logger::debug() << "调试信息: " << 42;
Logger::info() << "状态: " << "OK";
Logger::warning() << "警告: " << "内存不足";
Logger::error() << "错误码: " << 404;

// 支持多种类型
int value = 123;
double pi = 3.14159;
bool flag = true;
void* ptr = &value;

Logger::info() << "整数: " << value
              << " 浮点: " << pi
              << " 布尔: " << flag
              << " 指针: " << ptr;

// 支持换行符操纵符
Logger::info() << "第一行" << Logger::endl
              << "第二行" << Logger::endl;

// 或者使用全局版本
Logger::info() << "消息" << ::endl;
```

---

## 输出格式

日志输出格式为：

```
YYYY-MM-DD HH:MM:SS [LEVEL] file:line - message
```

### 控制台输出示例

```
2026-02-18 13:25:30 [DEBUG] main.cpp:15 - 这是调试信息
2026-02-18 13:25:30 [INFO] main.cpp:16 - 这是一般信息
2026-02-18 13:25:30 [WARNING] main.cpp:17 - 这是警告信息
2026-02-18 13:25:30 [ERROR] main.cpp:18 - 这是错误信息
```

### 文件日志

文件日志会在指定路径基础上自动添加日期后缀：

- 输入：`"app.log"`
- 生成：`app-20260218.log`

当日期变更或超过 24 小时，会自动创建新的日志文件。

---

## 编译与测试

### 编译示例

```bash
# 直接编译
g++ -std=c++20 main.cpp -o myapp

# 使用 CMake 构建
mkdir build && cd build
cmake ..
make
```

### 运行测试

项目包含完整的 GoogleTest 测试套件：

```bash
cd build
cmake ..
make
ctest
```

或者直接运行测试可执行文件：

```bash
./tests/logger_tests
```

测试覆盖：
- 单例模式测试
- 日志级别测试
- 控制台/文件输出测试
- LogStream 类型测试
- 线程安全测试
- 边界条件测试
- 性能测试

---

## 性能

在普通硬件上的性能表现（单线程）：

| 操作 | 性能 |
|:-----|:-----|
| 日志吞吐量 | ~1,000,000 msg/s |
| 过滤性能（被过滤日志） | ~5,000,000 msg/s |
| 平均单次调用开销 | ~3 μs |

---

## 项目结构

```
cppLoger/
├── include/
│   └── Logger.hpp          # 日志库头文件
├── tests/
│   ├── test_utils/         # 测试辅助工具
│   ├── main_test.cpp       # 测试入口
│   ├── test_*.cpp          # 各类测试文件
│   └── CMakeLists.txt      # 测试构建配置
├── main.cpp                # 示例程序
├── CMakeLists.txt          # CMake 配置
└── README.md               # 项目说明
```

---

## 许可证

MIT License

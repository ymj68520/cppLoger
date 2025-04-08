# Logger

一个简单的c++的Logger类实现
引入`Logger.hpp`即可使用
配置方式如下：

```c++
// 设置日志级别为DEBUG
Logger::getInstance().setLevel(LogLevel::DEBUG);
// 设置日志输出到控制台
Logger::getInstance().setConsole(true);
// 设置日志输出到文件
Logger::getInstance().setFile(true, "app.log");
```

配置全局初始化一次即可

使用示例：

```c++
LOG_DEBUG << "This is a debug message.";
LOG_INFO << "This is an info message.";
LOG_WARNING << "This is a warning message.";
LOG_ERROR << "This is an error message.";
LOG_FATAL << "This is a fatal message.";
```

输出：

```bash
2025-04-08 11:20:18 [DEBUG] .\main.cpp:14 - This is a debug message.
2025-04-08 11:20:18 [INFO] .\main.cpp:15 - This is an info message.
2025-04-08 11:20:18 [WARNING] .\main.cpp:16 - This is a warning message.
2025-04-08 11:20:18 [ERROR] .\main.cpp:17 - This is an error message.
2025-04-08 11:20:18 [ERROR] .\main.cpp:18 - FATAL ERROR: .\main.cpp:18 - This is a fatal message.
```

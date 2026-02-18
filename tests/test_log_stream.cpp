#include <gtest/gtest.h>
#include "Logger.hpp"
#include "test_utils/test_helpers.hpp"
#include <functional>
#include <sstream>
#include <limits>
#include <algorithm>

class LogStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::DEBUG);
        Logger::getInstance().setConsole(false);
    }

    void TearDown() override {
        Logger::getInstance().setConsole(true);
        Logger::getInstance().setFile(false, "");
    }

    // Helper: capture log output via file
    std::string capture_log(std::function<void()> log_func) {
        test_utils::TempFile temp_base("log_stream_capture.log");
        Logger::getInstance().setFile(true, temp_base.string());
        test_utils::short_sleep();

        log_func();
        test_utils::short_sleep();

        // Find the actual log file (with date suffix)
        auto temp_dir = std::filesystem::temp_directory_path();
        for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find("log_stream_capture") == 0) {
                std::ifstream file(entry.path());
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::filesystem::remove(entry.path());
                return buffer.str();
            }
        }
        return "";
    }

    // Helper: find and clean up log files with pattern
    void cleanup_logs_with_pattern(const std::string& pattern) {
        auto temp_dir = std::filesystem::temp_directory_path();
        for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find(pattern) == 0) {
                std::filesystem::remove(entry.path());
            }
        }
    }
};

// Test 1: Integer output
TEST_F(LogStreamTest, IntegerOutput) {
    auto content = capture_log([]() {
        Logger::debug() << 42;
        Logger::debug() << -12345;
        Logger::debug() << 0;
    });

    EXPECT_TRUE(content.find("42") != std::string::npos);
    EXPECT_TRUE(content.find("-12345") != std::string::npos);
    EXPECT_TRUE(content.find("0") != std::string::npos);
}

// Test 2: Floating point output
TEST_F(LogStreamTest, FloatingPointOutput) {
    auto content = capture_log([]() {
        Logger::debug() << 3.14159f;
        Logger::debug() << -2.5;
        Logger::debug() << 1.0;
    });

    // Float formatted as %.4f
    EXPECT_TRUE(content.find("3.1416") != std::string::npos ||  // rounded
                content.find("3.1415") != std::string::npos);
    EXPECT_TRUE(content.find("-2.5000") != std::string::npos);
    EXPECT_TRUE(content.find("1.0000") != std::string::npos);
}

// Test 3: Boolean output
TEST_F(LogStreamTest, BooleanOutput) {
    auto content = capture_log([]() {
        Logger::debug() << true;
        Logger::debug() << false;
    });

    EXPECT_TRUE(content.find("true") != std::string::npos);
    EXPECT_TRUE(content.find("false") != std::string::npos);
}

// Test 4: String output
TEST_F(LogStreamTest, StringOutput) {
    auto content = capture_log([]() {
        Logger::debug() << "Hello, World!";
        Logger::debug() << std::string("C++ string");
    });

    EXPECT_TRUE(content.find("Hello, World!") != std::string::npos);
    EXPECT_TRUE(content.find("C++ string") != std::string::npos);
}

// Test 5: Pointer output
TEST_F(LogStreamTest, PointerOutput) {
    int value = 42;
    void* ptr = &value;

    auto content = capture_log([ptr]() {
        Logger::debug() << ptr;
        Logger::debug() << static_cast<void*>(nullptr);
    });

    // Check for pointer format (0x prefix)
    EXPECT_TRUE(content.find("0x") != std::string::npos);
}

// Test 6: Character output
TEST_F(LogStreamTest, CharOutput) {
    auto content = capture_log([]() {
        Logger::debug() << 'A';
        Logger::debug() << 'z';
    });

    EXPECT_TRUE(content.find("A") != std::string::npos);
    EXPECT_TRUE(content.find("z") != std::string::npos);
}

// Test 7: Chained output
TEST_F(LogStreamTest, ChainedOutput) {
    auto content = capture_log([]() {
        Logger::debug() << "The answer is " << 42 << " and pi is " << 3.14f;
    });

    EXPECT_TRUE(content.find("The answer is") != std::string::npos);
    EXPECT_TRUE(content.find("42") != std::string::npos);
    EXPECT_TRUE(content.find("and pi is") != std::string::npos);
    EXPECT_TRUE(content.find("3.1400") != std::string::npos);
}

// Test 8: Different log level streams
TEST_F(LogStreamTest, DifferentLogLevels) {
    test_utils::TempFile temp_base("log_levels.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::debug() << "debug message";
    Logger::info() << "info message";
    Logger::warning() << "warning message";
    Logger::error() << "error message";
    Logger::fatal() << "fatal message";
    test_utils::short_sleep();

    auto temp_dir = std::filesystem::temp_directory_path();
    bool found = false;
    std::string content;

    for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        std::string filename = entry.path().filename().string();
        if (filename.find("log_levels") == 0) {
            std::ifstream file(entry.path());
            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
            found = true;
            std::filesystem::remove(entry.path());
            break;
        }
    }

    ASSERT_TRUE(found);
    EXPECT_TRUE(content.find("[DEBUG]") != std::string::npos);
    EXPECT_TRUE(content.find("[INFO]") != std::string::npos);
    EXPECT_TRUE(content.find("[WARNING]") != std::string::npos);
    EXPECT_TRUE(content.find("[ERROR]") != std::string::npos);
    EXPECT_TRUE(content.find("FATAL ERROR:") != std::string::npos);
}

// Test 9: Empty string handling
TEST_F(LogStreamTest, EmptyString) {
    auto content = capture_log([]() {
        Logger::debug() << "";
    });

    // Empty string should still produce log line (with timestamp etc)
    EXPECT_TRUE(!content.empty());
}

// Test 10: Large integer output
TEST_F(LogStreamTest, LargeInteger) {
    auto content = capture_log([]() {
        Logger::debug() << 2147483647;   // INT_MAX
        Logger::debug() << -2147483647;  // near INT_MIN
    });

    EXPECT_TRUE(content.find("2147483647") != std::string::npos);
}

// Test 11: Buffer boundary test
TEST_F(LogStreamTest, BufferBoundary) {
    // LogStream BUFFER_SIZE = 4096
    std::string long_message(2000, 'A');

    auto content = capture_log([long_message]() {
        Logger::debug() << long_message;
    });

    // Should contain the message
    EXPECT_TRUE(content.find("AAAAA") != std::string::npos);
}

// Test 12: Unicode string
TEST_F(LogStreamTest, UnicodeString) {
    auto content = capture_log([]() {
        Logger::debug() << "Hello World";
        Logger::debug() << "Test";
    });

    EXPECT_TRUE(!content.empty());
}

// Test 13: Negative numbers
TEST_F(LogStreamTest, NegativeNumbers) {
    auto content = capture_log([]() {
        Logger::debug() << -42;
        Logger::debug() << -3.14f;
        Logger::debug() << -100;
    });

    EXPECT_TRUE(content.find("-42") != std::string::npos);
    EXPECT_TRUE(content.find("-3.1400") != std::string::npos);
    EXPECT_TRUE(content.find("-100") != std::string::npos);
}

// Test 14: Zero values
TEST_F(LogStreamTest, ZeroValues) {
    auto content = capture_log([]() {
        Logger::debug() << 0;
        Logger::debug() << 0.0f;
        Logger::debug() << 0.0;
    });

    EXPECT_TRUE(content.find("0") != std::string::npos);
}

// Test 15: Multiple types in one stream
TEST_F(LogStreamTest, MultipleTypesInOneStream) {
    auto content = capture_log([]() {
        Logger::info() << "Int: " << 42 << ", Float: " << 3.14f << ", Bool: " << true;
    });

    EXPECT_TRUE(content.find("Int:") != std::string::npos);
    EXPECT_TRUE(content.find("42") != std::string::npos);
    EXPECT_TRUE(content.find("Float:") != std::string::npos);
    EXPECT_TRUE(content.find("3.1400") != std::string::npos);
    EXPECT_TRUE(content.find("Bool:") != std::string::npos);
    EXPECT_TRUE(content.find("true") != std::string::npos);
}

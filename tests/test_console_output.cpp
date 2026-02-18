#include <gtest/gtest.h>
#include "Logger.hpp"
#include "test_utils/test_helpers.hpp"
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>

class ConsoleOutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::DEBUG);
        Logger::getInstance().setConsole(true);
        Logger::getInstance().setFile(false, "");
    }

    void TearDown() override {
        Logger::getInstance().setConsole(true);
        Logger::getInstance().setFile(false, "");
    }

    // Helper to strip ANSI color codes from string
    std::string stripAnsiCodes(const std::string& str) {
        std::regex ansi_regex(R"(\x1b\[[0-9;]*m)");
        return std::regex_replace(str, ansi_regex, "");
    }
};

// Test 1: Disable console output
TEST_F(ConsoleOutputTest, DisableConsoleOutput) {
    Logger::getInstance().setConsole(false);

    EXPECT_NO_THROW({
        Logger::getInstance().log(LogLevel::INFO, "test message", __FILE__, __LINE__);
    });
}

// Test 2: Enable console output
TEST_F(ConsoleOutputTest, EnableConsoleOutput) {
    Logger::getInstance().setConsole(true);

    EXPECT_NO_THROW({
        Logger::getInstance().log(LogLevel::INFO, "test message", __FILE__, __LINE__);
    });
}

// Test 3: Toggle console output state
TEST_F(ConsoleOutputTest, ToggleConsoleOutput) {
    Logger::getInstance().setConsole(false);
    Logger::getInstance().setConsole(true);

    EXPECT_NO_THROW({
        Logger::getInstance().log(LogLevel::INFO, "test message", __FILE__, __LINE__);
    });
}

// Test 4: Verify output format using file redirection
TEST_F(ConsoleOutputTest, VerifyOutputFormat) {
    test_utils::TempFile temp_output("console_test.txt");

    // Save original stdout
    FILE* original_stdout = stdout;
    stdout = fopen(temp_output.string().c_str(), "w");

    ASSERT_NE(stdout, nullptr) << "Failed to redirect stdout";

    Logger::getInstance().log(LogLevel::INFO, "test message", "test_file.cpp", 42);

    // Restore stdout
    fclose(stdout);
    stdout = original_stdout;

    // Verify output format: YYYY-MM-DD HH:MM:SS [LEVEL] file:line - message
    std::string content = temp_output.read_content();
    std::string clean_content = stripAnsiCodes(content);

    // Check for timestamp pattern (YYYY-MM-DD HH:MM:SS)
    EXPECT_TRUE(temp_output.matches(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})"))
        << "Expected timestamp pattern in: " << content;

    // Check for [INFO] in stripped content
    EXPECT_TRUE(clean_content.find("[INFO]") != std::string::npos) << "Expected [INFO] in: " << clean_content;

    // Check for filename and line number
    EXPECT_TRUE(clean_content.find("test_file.cpp") != std::string::npos) << "Expected filename in: " << clean_content;
    EXPECT_TRUE(clean_content.find("42") != std::string::npos) << "Expected line number in: " << clean_content;

    // Check for message content
    EXPECT_TRUE(clean_content.find("test message") != std::string::npos) << "Expected message in: " << clean_content;
}

// Test 5: All log levels can be logged to console
TEST_F(ConsoleOutputTest, AllLevelsToConsole) {
    test_utils::TempFile temp_output("console_all_levels.txt");

    FILE* original_stdout = stdout;
    stdout = fopen(temp_output.string().c_str(), "w");
    ASSERT_NE(stdout, nullptr);

    Logger::getInstance().log(LogLevel::DEBUG, "debug msg", "test.cpp", 1);
    Logger::getInstance().log(LogLevel::INFO, "info msg", "test.cpp", 2);
    Logger::getInstance().log(LogLevel::WARNING, "warning msg", "test.cpp", 3);
    Logger::getInstance().log(LogLevel::ERROR, "error msg", "test.cpp", 4);

    fclose(stdout);
    stdout = original_stdout;

    std::string content = temp_output.read_content();
    std::string clean_content = stripAnsiCodes(content);

    EXPECT_TRUE(clean_content.find("[DEBUG]") != std::string::npos);
    EXPECT_TRUE(clean_content.find("[INFO]") != std::string::npos);
    EXPECT_TRUE(clean_content.find("[WARNING]") != std::string::npos);
    EXPECT_TRUE(clean_content.find("[ERROR]") != std::string::npos);
}

// Test 6: Color codes are present in output
TEST_F(ConsoleOutputTest, ColorCodesPresent) {
    test_utils::TempFile temp_output("console_colors.txt");

    FILE* original_stdout = stdout;
    stdout = fopen(temp_output.string().c_str(), "w");
    ASSERT_NE(stdout, nullptr);

    Logger::getInstance().log(LogLevel::ERROR, "error with color", "test.cpp", 1);

    fclose(stdout);
    stdout = original_stdout;

    // Check for ANSI color code (RED: \033[31m)
    std::string content = temp_output.read_content();
    EXPECT_TRUE(content.find("\033[31m") != std::string::npos ||
                content.find("\033[")) << "Expected ANSI color codes";
}

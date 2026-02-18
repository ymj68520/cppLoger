#include <gtest/gtest.h>
#include "Logger.hpp"
#include "test_utils/test_helpers.hpp"
#include <cfloat>
#include <cmath>
#include <sstream>
#include <fstream>

// Helper function to count log lines
// Note: Logger removes extension and adds date suffix like: base.log -> base-YYYYMMDD.log
size_t count_log_lines(const std::string& pattern) {
    size_t total = 0;
    auto temp_dir = std::filesystem::temp_directory_path();

    // Remove .log extension from pattern for matching
    std::string search_pattern = pattern;
    size_t dot_pos = search_pattern.find(".log");
    if (dot_pos != std::string::npos) {
        search_pattern = search_pattern.substr(0, dot_pos);
    }

    for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        std::string filename = entry.path().filename().string();
        if (filename.find(search_pattern) == 0) {
            std::ifstream file(entry.path());
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) total++;
            }
        }
    }
    return total;
}

class EdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::DEBUG);  // Set to DEBUG to capture all log levels
        Logger::getInstance().setConsole(false);
    }

    void TearDown() override {
        Logger::getInstance().setConsole(true);
        Logger::getInstance().setFile(false, "");
        cleanup_temp_logs();
    }

    void cleanup_temp_logs() {
        auto temp_dir = std::filesystem::temp_directory_path();
        for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find("empty") == 0 ||
                filename.find("long") == 0 ||
                filename.find("special") == 0 ||
                filename.find("negative") == 0 ||
                filename.find("extreme") == 0 ||
                filename.find("rapid") == 0 ||
                filename.find("boundary") == 0) {
                std::filesystem::remove(entry.path());
            }
        }
    }
};

// Test 1: Empty message
TEST_F(EdgeCaseTest, EmptyMessage) {
    test_utils::TempFile temp_base("empty_msg.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    EXPECT_NO_THROW({
        Logger::getInstance().log(LogLevel::INFO, "", __FILE__, __LINE__);
    });

    test_utils::short_sleep();
    EXPECT_GT(count_log_lines("empty_msg"), 0);
}

// Test 2: Very long message
TEST_F(EdgeCaseTest, VeryLongMessage) {
    test_utils::TempFile temp_base("long_msg.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    std::string long_message(10000, 'A');  // 10KB message

    EXPECT_NO_THROW({
        Logger::getInstance().log(LogLevel::INFO, long_message.c_str(), __FILE__, __LINE__);
    });

    test_utils::short_sleep();

    // Check some content was written
    size_t line_count = count_log_lines("long_msg");
    EXPECT_GT(line_count, 0);
}

// Test 3: Special characters in message
TEST_F(EdgeCaseTest, SpecialCharactersInMessage) {
    test_utils::TempFile temp_base("special_chars.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const char* special_messages[] = {
        "Message with \"quotes\"",
        "Message with 'apostrophes'",
        "Message with % percent",
        "Message with & ampersand",
        "Message with <html> tags</html>",
        "Message with {braces}",
        "Message with [brackets]",
        "Message with (parentheses)",
    };

    for (const auto* msg : special_messages) {
        EXPECT_NO_THROW({
            Logger::getInstance().log(LogLevel::INFO, msg, __FILE__, __LINE__);
        });
    }

    test_utils::short_sleep();
    EXPECT_GT(count_log_lines("special_chars"), 0);
}

// Test 4: Invalid file name
TEST_F(EdgeCaseTest, InvalidFileName) {
    EXPECT_NO_THROW({
        Logger::getInstance().setFile(true, "");  // Empty path
    });

    EXPECT_NO_THROW({
        Logger::getInstance().setFile(true, "/nonexistent/directory/path/file.log");
    });
}

// Test 5: nullptr string handling
TEST_F(EdgeCaseTest, NullStringHandling) {
    // LogStream's operator<<(const char*) should handle nullptr
    EXPECT_NO_THROW({
        Logger::debug() << static_cast<const char*>(nullptr);
    });
}

// Test 6: Negative floating point
TEST_F(EdgeCaseTest, NegativeFloatingPoint) {
    test_utils::TempFile temp_base("negative_float.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::debug() << -0.0f;
    Logger::debug() << -1.5;
    Logger::debug() << -3.14159;

    test_utils::short_sleep();

    size_t line_count = count_log_lines("negative_float");
    EXPECT_GE(line_count, 3);
}

// Test 7: Extreme floating point values
TEST_F(EdgeCaseTest, ExtremeFloatingPoint) {
    test_utils::TempFile temp_base("extreme_float.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::debug() << FLT_MAX;
    Logger::debug() << FLT_MIN;
    Logger::debug() << DBL_MAX;
    Logger::debug() << DBL_MIN;

    test_utils::short_sleep();

    size_t line_count = count_log_lines("extreme_float");
    EXPECT_GE(line_count, 4);
}

// Test 8: Rapid file toggle
TEST_F(EdgeCaseTest, RapidFileToggle) {
    test_utils::TempFile temp_base("rapid_toggle.log");

    for (int i = 0; i < 50; ++i) {
        Logger::getInstance().setFile(true, temp_base.string());
        Logger::getInstance().log(LogLevel::INFO, "test", __FILE__, __LINE__);
        Logger::getInstance().setFile(false, "");
    }

    // Verify no crash
    SUCCEED();
}

// Test 9: Multiple getInstance calls
TEST_F(EdgeCaseTest, MultipleGetInstanceCalls) {
    for (int i = 0; i < 1000; ++i) {
        Logger& logger = Logger::getInstance();
        EXPECT_NO_THROW({
            logger.setLevel(LogLevel::DEBUG);
            logger.getLevel();
        });
    }
}

// Test 10: All outputs enabled simultaneously
TEST_F(EdgeCaseTest, AllOutputsEnabled) {
    test_utils::TempFile temp_base("all_outputs.log");

    Logger::getInstance().setConsole(true);
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    for (int i = 0; i < 4; ++i) {
        Logger::getInstance().log(static_cast<LogLevel>(i), "test", __FILE__, __LINE__);
    }

    test_utils::short_sleep();

    size_t line_count = count_log_lines("all_outputs");
    EXPECT_GE(line_count, 4);
}

// Test 11: Maximum integer values
TEST_F(EdgeCaseTest, MaxIntegerValues) {
    test_utils::TempFile temp_base("boundary_int.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::debug() << INT32_MAX;
    Logger::debug() << INT32_MIN;
    Logger::debug() << UINT32_MAX;
    Logger::debug() << INT64_MAX;
    Logger::debug() << INT64_MIN;

    test_utils::short_sleep();

    size_t line_count = count_log_lines("boundary_int");
    EXPECT_GE(line_count, 5);
}

// Test 12: Very long LogStream chain
TEST_F(EdgeCaseTest, VeryLongLogStreamChain) {
    test_utils::TempFile temp_base("boundary_chain.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    // Chain many operations together
    Logger::debug() << "A" << 1 << "B" << 2 << "C" << 3 << "D" << 4
                    << "E" << 5 << "F" << 6 << "G" << 7 << "H" << 8
                    << "I" << 9 << "J" << 10;

    test_utils::short_sleep();

    size_t line_count = count_log_lines("boundary_chain");
    EXPECT_GE(line_count, 1);
}

// Test 13: Toggle all log levels rapidly
TEST_F(EdgeCaseTest, RapidLogLevelChange) {
    for (int i = 0; i < 100; ++i) {
        Logger::getInstance().setLevel(static_cast<LogLevel>(i % 4));
        Logger::info() << "Level change " << i;
    }

    // Verify no crash
    SUCCEED();
}

// Test 14: Nullptr in various contexts
TEST_F(EdgeCaseTest, NullptrInVariousContexts) {
    EXPECT_NO_THROW({
        Logger::debug() << static_cast<void*>(nullptr);
        Logger::info() << "Ptr: " << static_cast<void*>(nullptr);
    });
}

// Test 15: Whitespace messages
TEST_F(EdgeCaseTest, WhitespaceMessages) {
    test_utils::TempFile temp_base("boundary_space.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::debug() << "   ";
    Logger::debug() << "\t\t\t";
    Logger::debug() << "spaces between words";

    test_utils::short_sleep();

    size_t line_count = count_log_lines("boundary_space");
    EXPECT_GE(line_count, 3);
}

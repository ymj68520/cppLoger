#include <gtest/gtest.h>
#include "Logger.hpp"
#include "test_utils/test_helpers.hpp"
#include <thread>
#include <regex>
#include <sstream>

class FileOutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::DEBUG);
        Logger::getInstance().setConsole(false);  // Disable console for file tests
    }

    void TearDown() override {
        Logger::getInstance().setFile(false, "");
        Logger::getInstance().setConsole(true);
    }

    // Helper to find files with specific pattern
    // Note: Logger removes extension and adds date suffix like: base.log -> base-YYYYMMDD.log
    std::vector<std::string> findFilesWithPattern(const std::string& base_name) {
        std::vector<std::string> found;
        auto temp_dir = std::filesystem::temp_directory_path();

        // Remove .log extension from base_name for matching
        std::string pattern = base_name;
        size_t dot_pos = pattern.find(".log");
        if (dot_pos != std::string::npos) {
            pattern = pattern.substr(0, dot_pos);
        }

        for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find(pattern) == 0) {
                found.push_back(entry.path().string());
            }
        }
        return found;
    }

    // Helper to read file content
    std::string readFileContent(const std::string& path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

// Test 1: Enable file output
TEST_F(FileOutputTest, EnableFileOutput) {
    test_utils::TempFile temp_file("test_logger.log");

    EXPECT_NO_THROW({
        Logger::getInstance().setFile(true, temp_file.string());
    });

    test_utils::short_sleep();

    // Log a message
    Logger::getInstance().log(LogLevel::INFO, "test message", __FILE__, __LINE__);

    test_utils::short_sleep();

    // Find the actual log file (with date suffix)
    auto found_files = findFilesWithPattern("test_logger.log");
    bool file_created = !found_files.empty();

    EXPECT_TRUE(file_created) << "Log file should be created";
}

// Test 2: Disable file output
TEST_F(FileOutputTest, DisableFileOutput) {
    test_utils::TempFile temp_file("test_logger_disable.log");

    Logger::getInstance().setFile(true, temp_file.string());
    Logger::getInstance().setFile(false, "");

    EXPECT_NO_THROW({
        Logger::getInstance().log(LogLevel::INFO, "test message", __FILE__, __LINE__);
    });
}

// Test 3: File content format verification
TEST_F(FileOutputTest, VerifyFileContentFormat) {
    test_utils::TempFile temp_base("test_logger_format.log");

    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::INFO, "test message", "test.cpp", 123);
    test_utils::short_sleep();

    // Find the actual log file
    auto found_files = findFilesWithPattern("test_logger_format.log");
    ASSERT_FALSE(found_files.empty()) << "Log file should exist";

    // Read the first (and should be only) matching file
    std::string content = readFileContent(found_files[0]);

    // Verify content components
    EXPECT_TRUE(content.find("[INFO]") != std::string::npos) << "Should contain [INFO]";
    EXPECT_TRUE(content.find("test.cpp") != std::string::npos) << "Should contain filename";
    EXPECT_TRUE(content.find("123") != std::string::npos) << "Should contain line number";
    EXPECT_TRUE(content.find("test message") != std::string::npos) << "Should contain message";

    // Cleanup
    for (const auto& f : found_files) {
        std::filesystem::remove(f);
    }
}

// Test 4: Multiple logs to file
TEST_F(FileOutputTest, MultipleLogsToFile) {
    test_utils::TempFile temp_base("test_logger_multi.log");

    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::INFO, "message 1", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::WARNING, "message 2", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::ERROR, "message 3", __FILE__, __LINE__);
    test_utils::short_sleep();

    auto found_files = findFilesWithPattern("test_logger_multi.log");
    ASSERT_FALSE(found_files.empty());

    std::string content = readFileContent(found_files[0]);

    EXPECT_TRUE(content.find("message 1") != std::string::npos);
    EXPECT_TRUE(content.find("message 2") != std::string::npos);
    EXPECT_TRUE(content.find("message 3") != std::string::npos);

    // Count lines
    std::stringstream ss(content);
    std::string line;
    int line_count = 0;
    while (std::getline(ss, line)) {
        if (!line.empty()) line_count++;
    }
    EXPECT_GE(line_count, 3);

    for (const auto& f : found_files) {
        std::filesystem::remove(f);
    }
}

// Test 5: All log levels write to file
TEST_F(FileOutputTest, AllLevelsWriteToFile) {
    test_utils::TempFile temp_base("test_all_levels.log");

    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::DEBUG, "debug msg", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::INFO, "info msg", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::WARNING, "warning msg", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::ERROR, "error msg", __FILE__, __LINE__);
    test_utils::short_sleep();

    auto found_files = findFilesWithPattern("test_all_levels.log");
    ASSERT_FALSE(found_files.empty());

    std::string content = readFileContent(found_files[0]);

    EXPECT_TRUE(content.find("[DEBUG]") != std::string::npos);
    EXPECT_TRUE(content.find("[INFO]") != std::string::npos);
    EXPECT_TRUE(content.find("[WARNING]") != std::string::npos);
    EXPECT_TRUE(content.find("[ERROR]") != std::string::npos);

    for (const auto& f : found_files) {
        std::filesystem::remove(f);
    }
}

// Test 6: Log level filtering applies to file output
TEST_F(FileOutputTest, LevelFilteringInFile) {
    test_utils::TempFile temp_base("test_filter.log");

    Logger::getInstance().setFile(true, temp_base.string());
    Logger::getInstance().setLevel(LogLevel::WARNING);
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::DEBUG, "debug msg", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::INFO, "info msg", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::WARNING, "warning msg", __FILE__, __LINE__);
    Logger::getInstance().log(LogLevel::ERROR, "error msg", __FILE__, __LINE__);
    test_utils::short_sleep();

    auto found_files = findFilesWithPattern("test_filter.log");
    ASSERT_FALSE(found_files.empty());

    std::string content = readFileContent(found_files[0]);

    // DEBUG and INFO should be filtered out
    EXPECT_FALSE(content.find("debug msg") != std::string::npos) << "DEBUG should be filtered";
    EXPECT_FALSE(content.find("info msg") != std::string::npos) << "INFO should be filtered";

    // WARNING and ERROR should appear
    EXPECT_TRUE(content.find("warning msg") != std::string::npos);
    EXPECT_TRUE(content.find("error msg") != std::string::npos);

    for (const auto& f : found_files) {
        std::filesystem::remove(f);
    }
}

// Test 7: File path has date suffix
TEST_F(FileOutputTest, FilePathWithDateSuffix) {
    test_utils::TempFile temp_base("test_date_logger.log");

    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::INFO, "test", __FILE__, __LINE__);
    test_utils::short_sleep();

    // Check for date pattern in filename: -YYYYMMDD.log
    auto found_files = findFilesWithPattern("test_date_logger.log");

    bool has_date_suffix = false;
    for (const auto& f : found_files) {
        std::string filename = std::filesystem::path(f).filename().string();
        // Check for date pattern: -20250218.log or similar
        std::regex date_pattern(R"(-\d{8}\.log)");
        if (std::regex_search(filename, date_pattern)) {
            has_date_suffix = true;
        }
        std::filesystem::remove(f);
    }

    EXPECT_TRUE(has_date_suffix) << "File should have date suffix";
}

// Test 8: Console and file simultaneously
TEST_F(FileOutputTest, ConsoleAndFileSimultaneously) {
    test_utils::TempFile temp_base("test_both.log");

    Logger::getInstance().setConsole(true);
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::INFO, "both output", __FILE__, __LINE__);
    test_utils::short_sleep();

    // Verify file output
    auto found_files = findFilesWithPattern("test_both.log");
    EXPECT_FALSE(found_files.empty());

    if (!found_files.empty()) {
        std::string content = readFileContent(found_files[0]);
        EXPECT_TRUE(content.find("both output") != std::string::npos);

        for (const auto& f : found_files) {
            std::filesystem::remove(f);
        }
    }
}

// Test 9: Empty file path handling
TEST_F(FileOutputTest, EmptyFilePath) {
    EXPECT_NO_THROW({
        Logger::getInstance().setFile(true, "");
    });
}

// Test 10: Reopen file (simulate rotation trigger)
TEST_F(FileOutputTest, ReopenFile) {
    test_utils::TempFile temp_base("test_reopen.log");

    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::INFO, "before reopen", __FILE__, __LINE__);

    // Re-enable file (should trigger reopen)
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    Logger::getInstance().log(LogLevel::INFO, "after reopen", __FILE__, __LINE__);
    test_utils::short_sleep();

    auto found_files = findFilesWithPattern("test_reopen.log");
    ASSERT_FALSE(found_files.empty());

    std::string content = readFileContent(found_files[0]);

    EXPECT_TRUE(content.find("before reopen") != std::string::npos);
    EXPECT_TRUE(content.find("after reopen") != std::string::npos);

    for (const auto& f : found_files) {
        std::filesystem::remove(f);
    }
}

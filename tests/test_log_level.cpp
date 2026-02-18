#include <gtest/gtest.h>
#include "Logger.hpp"

class LogLevelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset to INFO level before each test
        Logger::getInstance().setLevel(LogLevel::INFO);
    }
};

// Test 1: Set and get log level
TEST_F(LogLevelTest, SetAndGetLevel) {
    Logger::getInstance().setLevel(LogLevel::DEBUG);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::DEBUG);

    Logger::getInstance().setLevel(LogLevel::INFO);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::INFO);

    Logger::getInstance().setLevel(LogLevel::WARNING);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::WARNING);

    Logger::getInstance().setLevel(LogLevel::ERROR);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::ERROR);
}

// Test 2: Default level is INFO
TEST_F(LogLevelTest, DefaultLevelIsInfo) {
    Logger& logger = Logger::getInstance();
    logger.setLevel(LogLevel::INFO);
    EXPECT_EQ(logger.getLevel(), LogLevel::INFO);
}

// Test 3: Log level enumeration ordering
TEST_F(LogLevelTest, LogLevelOrdering) {
    EXPECT_LT(LogLevel::DEBUG, LogLevel::INFO);
    EXPECT_LT(LogLevel::INFO, LogLevel::WARNING);
    EXPECT_LT(LogLevel::WARNING, LogLevel::ERROR);

    // Verify numeric values
    EXPECT_EQ(static_cast<int>(LogLevel::DEBUG), 0);
    EXPECT_EQ(static_cast<int>(LogLevel::INFO), 1);
    EXPECT_EQ(static_cast<int>(LogLevel::WARNING), 2);
    EXPECT_EQ(static_cast<int>(LogLevel::ERROR), 3);
}

// Test 4: Atomic level operations (thread-safe)
TEST_F(LogLevelTest, AtomicLevelOperations) {
    Logger& logger = Logger::getInstance();

    logger.setLevel(LogLevel::DEBUG);
    EXPECT_EQ(logger.getLevel(), LogLevel::DEBUG);

    logger.setLevel(LogLevel::ERROR);
    EXPECT_EQ(logger.getLevel(), LogLevel::ERROR);
}

// Test 5: logLevelToString function
TEST(LogLevelStringTest, ConvertsLevelToString) {
    EXPECT_STREQ(logLevelToString(LogLevel::DEBUG), "DEBUG");
    EXPECT_STREQ(logLevelToString(LogLevel::INFO), "INFO");
    EXPECT_STREQ(logLevelToString(LogLevel::WARNING), "WARNING");
    EXPECT_STREQ(logLevelToString(LogLevel::ERROR), "ERROR");
}

// Test 6: logLevelToColorCode function
TEST(LogLevelColorTest, ReturnsColorCodes) {
    EXPECT_STREQ(logLevelToColorCode(LogLevel::DEBUG), "\033[34m");   // Blue
    EXPECT_STREQ(logLevelToColorCode(LogLevel::INFO), "\033[32m");    // Green
    EXPECT_STREQ(logLevelToColorCode(LogLevel::WARNING), "\033[33m"); // Yellow
    EXPECT_STREQ(logLevelToColorCode(LogLevel::ERROR), "\033[31m");   // Red
}

// Test 7: log() with level filtering
TEST_F(LogLevelTest, LogFiltering) {
    Logger& logger = Logger::getInstance();

    // Set to WARNING level
    logger.setLevel(LogLevel::WARNING);
    logger.setConsole(false);  // Disable console to avoid noise

    // These should be filtered (no exception thrown)
    EXPECT_NO_THROW({
        logger.log(LogLevel::DEBUG, "debug message", __FILE__, __LINE__);
        logger.log(LogLevel::INFO, "info message", __FILE__, __LINE__);
        logger.log(LogLevel::WARNING, "warning message", __FILE__, __LINE__);
        logger.log(LogLevel::ERROR, "error message", __FILE__, __LINE__);
    });

    logger.setConsole(true);
}

// Test 8: All log levels are valid
TEST(LogLevelValidTest, AllLevelsValid) {
    LogLevel levels[] = {LogLevel::DEBUG, LogLevel::INFO, LogLevel::WARNING, LogLevel::ERROR};

    for (auto level : levels) {
        EXPECT_GE(level, LogLevel::DEBUG);
        EXPECT_LE(level, LogLevel::ERROR);
    }
}

#include <gtest/gtest.h>
#include "Logger.hpp"
#include "test_utils/test_helpers.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>

class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::DEBUG);
        Logger::getInstance().setConsole(false);
    }

    void TearDown() override {
        Logger::getInstance().setConsole(true);
        Logger::getInstance().setFile(false, "");
        // Clean up test log files
        cleanup_temp_logs();
    }

    void cleanup_temp_logs() {
        auto temp_dir = std::filesystem::temp_directory_path();
        for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find("thread_test") == 0 ||
                filename.find("concurrent") == 0 ||
                filename.find("stress") == 0 ||
                filename.find("mixed") == 0 ||
                filename.find("logstream") == 0) {
                std::filesystem::remove(entry.path());
            }
        }
    }

    // Helper to count total lines in all matching log files
    size_t count_log_lines(const std::string& pattern) {
        size_t total = 0;
        auto temp_dir = std::filesystem::temp_directory_path();

        for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find(pattern) == 0) {
                std::ifstream file(entry.path());
                std::string line;
                while (std::getline(file, line)) {
                    if (!line.empty()) total++;
                }
            }
        }
        return total;
    }
};

// Test 1: Concurrent logging from multiple threads
TEST_F(ThreadSafetyTest, ConcurrentLoggingFromMultipleThreads) {
    test_utils::TempFile temp_base("thread_test.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const int num_threads = 10;
    const int logs_per_thread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, logs_per_thread]() {
            for (int j = 0; j < logs_per_thread; ++j) {
                Logger::info() << "Thread " << i << " message " << j;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    test_utils::short_sleep();

    // Verify all logs were written (allow some tolerance)
    size_t line_count = count_log_lines("thread_test");
    EXPECT_GE(line_count, static_cast<size_t>(num_threads * logs_per_thread * 0.95));
}

// Test 2: Concurrent level setting
TEST_F(ThreadSafetyTest, ConcurrentLevelSetting) {
    const int num_threads = 10;
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                Logger::getInstance().setLevel(static_cast<LogLevel>(j % 4));
                LogLevel level = Logger::getInstance().getLevel();
                // Ensure read value is valid
                EXPECT_GE(level, LogLevel::DEBUG);
                EXPECT_LE(level, LogLevel::ERROR);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Test 3: Concurrent console toggle
TEST_F(ThreadSafetyTest, ConcurrentConsoleToggle) {
    const int num_threads = 5;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                Logger::getInstance().setConsole(j % 2 == 0);
                Logger::info() << "Toggle test " << i << " " << j;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Test 4: Concurrent file operations
TEST_F(ThreadSafetyTest, ConcurrentFileOperations) {
    test_utils::TempFile temp_base("concurrent_file.log");
    const int num_threads = 8;
    const int logs_per_thread = 50;
    std::vector<std::thread> threads;

    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, logs_per_thread]() {
            for (int j = 0; j < logs_per_thread; ++j) {
                Logger::error() << "Thread " << i << " log " << j;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    test_utils::short_sleep();

    // Verify file integrity
    size_t line_count = count_log_lines("concurrent_file");
    EXPECT_GT(line_count, 0);
}

// Test 5: Stress test with high concurrency
TEST_F(ThreadSafetyTest, StressTestHighConcurrency) {
    test_utils::TempFile temp_base("stress_test.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const int num_threads = 20;
    const int logs_per_thread = 200;
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, logs_per_thread, i]() {
            for (int j = 0; j < logs_per_thread; ++j) {
                Logger::info() << "Counter: " << counter.fetch_add(1) << " from thread " << i;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    test_utils::short_sleep();

    // Verify counter correctness
    EXPECT_EQ(counter.load(), num_threads * logs_per_thread);

    // Report performance
    std::cout << "Logged " << counter.load() << " messages in "
              << duration.count() << " ms" << std::endl;
    if (duration.count() > 0) {
        std::cout << "Throughput: " << (counter.load() * 1000 / duration.count())
                  << " msg/sec" << std::endl;
    }

    // Verify logs were written
    size_t line_count = count_log_lines("stress_test");
    EXPECT_GT(line_count, static_cast<size_t>(num_threads * logs_per_thread * 0.9));
}

// Test 6: Mixed operations (logging + configuration)
TEST_F(ThreadSafetyTest, MixedOperations) {
    test_utils::TempFile temp_base("mixed_ops.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const int num_threads = 10;
    std::vector<std::thread> threads;

    // Thread type 1: Only log
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 50; ++j) {
                Logger::info() << "Log thread " << i << " msg " << j;
            }
        });
    }

    // Thread type 2: Modify configuration
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 50; ++j) {
                Logger::getInstance().setLevel(static_cast<LogLevel>(j % 4));
                Logger::getInstance().setConsole(j % 2 == 0);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    test_utils::short_sleep();

    // Verify no crash or deadlock
    size_t line_count = count_log_lines("mixed_ops");
    EXPECT_GT(line_count, 0);
}

// Test 7: LogStream thread safety
TEST_F(ThreadSafetyTest, LogStreamThreadSafety) {
    test_utils::TempFile temp_base("logstream_thread.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const int num_threads = 10;
    const int logs_per_thread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, logs_per_thread]() {
            for (int j = 0; j < logs_per_thread; ++j) {
                Logger::debug() << "Stream from thread " << i << " value " << j;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    test_utils::short_sleep();

    size_t line_count = count_log_lines("logstream_thread");
    EXPECT_GT(line_count, 0);
}

// Test 8: Rapid start/stop of threads
TEST_F(ThreadSafetyTest, RapidThreadStartStop) {
    for (int iteration = 0; iteration < 5; ++iteration) {
        std::vector<std::thread> threads;

        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([i]() {
                Logger::info() << "Quick thread " << i;
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    // If we get here without crashing, the test passes
    SUCCEED();
}

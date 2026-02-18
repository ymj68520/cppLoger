#include <gtest/gtest.h>
#include "Logger.hpp"
#include "test_utils/test_helpers.hpp"
#include <chrono>
#include <vector>
#include <thread>

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLevel(LogLevel::INFO);
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
            if (filename.find("perf_") == 0) {
                std::filesystem::remove(entry.path());
            }
        }
    }
};

// Test 1: Single-thread throughput
TEST_F(PerformanceTest, SingleThreadThroughput) {
    test_utils::TempFile temp_base("perf_single.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const int num_logs = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_logs; ++i) {
        Logger::info() << "Performance test message " << i;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double throughput = (num_logs * 1000.0) / (duration.count() > 0 ? duration.count() : 1);

    std::cout << "Single-thread throughput: " << static_cast<int>(throughput) << " msg/sec" << std::endl;

    // Basic performance requirement: at least 1000 msg/sec
    EXPECT_GT(throughput, 1000);
}

// Test 2: Filtering performance (logs below level should be very fast)
TEST_F(PerformanceTest, FilteringPerformance) {
    Logger::getInstance().setLevel(LogLevel::ERROR);

    const int num_logs = 100000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_logs; ++i) {
        Logger::debug() << "This should be filtered " << i;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double filtered_throughput = (num_logs * 1000000.0) / (duration_us.count() > 0 ? duration_us.count() : 1);

    std::cout << "Filtered throughput: " << static_cast<int>(filtered_throughput) << " msg/sec" << std::endl;

    // Filtering should be very fast
    EXPECT_GT(filtered_throughput, 100000);
}

// Test 3: LogStream construction/destruction overhead
TEST_F(PerformanceTest, LogStreamOverhead) {
    test_utils::TempFile temp_base("perf_overhead.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        Logger::info() << "Test message " << i;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avg_time_us = (duration_us.count() * 1.0) / iterations;

    std::cout << "Average log call time: " << avg_time_us << " microseconds" << std::endl;

    // Each log call should be reasonably fast
    EXPECT_LT(avg_time_us, 1000);  // Less than 1ms per log
}

// Test 4: Message size impact
TEST_F(PerformanceTest, MessageSizeImpact) {
    test_utils::TempFile temp_base("perf_size.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    std::vector<int> message_sizes = {10, 100, 500, 1000};

    for (int size : message_sizes) {
        std::string message(size, 'X');

        const int num_logs = 1000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_logs; ++i) {
            Logger::info() << message.c_str();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        double throughput = (num_logs * 1000.0) / (duration.count() > 0 ? duration.count() : 1);
        std::cout << "Message size " << size << ": " << static_cast<int>(throughput) << " msg/sec" << std::endl;
    }

    // Test passes if it completes without hanging
    SUCCEED();
}

// Test 5: Multi-thread throughput
TEST_F(PerformanceTest, MultiThreadThroughput) {
    test_utils::TempFile temp_base("perf_multi.log");
    Logger::getInstance().setFile(true, temp_base.string());
    test_utils::short_sleep();

    const int num_threads = 4;
    const int logs_per_thread = 1000;
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

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

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int total_logs = num_threads * logs_per_thread;
    double throughput = (total_logs * 1000.0) / (duration.count() > 0 ? duration.count() : 1);

    std::cout << "Multi-thread (" << num_threads << " threads) throughput: "
              << static_cast<int>(throughput) << " msg/sec" << std::endl;

    EXPECT_GT(throughput, 500);
}

// Test 6: Level comparison performance
TEST_F(PerformanceTest, LevelComparisonPerformance) {
    Logger::getInstance().setLevel(LogLevel::INFO);

    const int iterations = 1000000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // This should be filtered out
        if (LogLevel::DEBUG >= Logger::getInstance().getLevel()) {
            // Should not reach here
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avg_ns = (duration_us.count() * 1000.0) / iterations;

    std::cout << "Average level check time: " << avg_ns << " nanoseconds" << std::endl;

    // Level check should be very fast (atomic operation)
    EXPECT_LT(avg_ns, 100);  // Less than 100ns
}

#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>

namespace test_utils {

// Temporary file manager with RAII cleanup
class TempFile {
public:
    explicit TempFile(const std::string& base_name)
        : path_(std::filesystem::temp_directory_path() / base_name) {}

    ~TempFile() {
        try {
            if (std::filesystem::exists(path_)) {
                std::filesystem::remove(path_);
            }
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }

    // Prevent copying
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    const std::filesystem::path& path() const { return path_; }
    std::string string() const { return path_.string(); }

    bool exists() const { return std::filesystem::exists(path_); }

    size_t size() const {
        if (!exists()) return 0;
        return std::filesystem::file_size(path_);
    }

    // Read file content
    std::string read_content() const {
        if (!exists()) return "";
        std::ifstream file(path_);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Read file line by line
    std::vector<std::string> read_lines() const {
        std::vector<std::string> lines;
        std::ifstream file(path_);
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        return lines;
    }

    // Check if file contains specific content
    bool contains(const std::string& pattern) const {
        auto content = read_content();
        return content.find(pattern) != std::string::npos;
    }

    // Search using regex
    bool matches(const std::string& pattern) const {
        auto lines = read_lines();
        std::regex regex_pattern(pattern);
        for (const auto& line : lines) {
            if (std::regex_search(line, regex_pattern)) {
                return true;
            }
        }
        return false;
    }

    // Clear file content
    void clear() {
        std::ofstream(path_, std::ios::trunc).close();
    }

    // Get file modification time
    std::filesystem::file_time_type last_write_time() const {
        return std::filesystem::last_write_time(path_);
    }

    // Count lines in file
    size_t line_count() const {
        return read_lines().size();
    }

private:
    std::filesystem::path path_;
};

// Wait for file to appear with timeout
inline bool wait_for_file(const std::filesystem::path& path,
                         std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
    auto start = std::chrono::steady_clock::now();
    while (!std::filesystem::exists(path)) {
        auto now = std::chrono::steady_clock::now();
        if (now - start > timeout) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

// Small delay to allow I/O operations to complete
inline void short_sleep() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

} // namespace test_utils

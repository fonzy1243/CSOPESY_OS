//
// Created by Alfon on 6/25/2025.
//

#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <optional>
#include <expected>
#include <format>
#include <ranges>
#include <algorithm>
#include <cctype>
#include <memory>

enum class ConfigError
{
    FileNotFound,
    InvalidFormat,
    InvalidValue,
    MissingParameter,
};

inline std::string to_string(ConfigError err)
{
    switch (err) {
        case ConfigError::FileNotFound:     return "File Not Found";
        case ConfigError::InvalidFormat:    return "Invalid Format";
        case ConfigError::InvalidValue:     return "Invalid Value";
        case ConfigError::MissingParameter: return "Missing Parameter";
        default:                            return "Unknown Error";
    }
}

template<>
struct std::formatter<ConfigError> : std::formatter<std::string>
{
    auto format(ConfigError err, format_context& ctx) const
    {
        return formatter<std::string>::format(to_string(err), ctx);
    }
};

struct CPUConfig
{
    int num_cpu{};
    std::string scheduler;
    int quantum_cycles{};
    int batch_process_freq{};
    int min_ins{};
    int max_ins{};
    int delays_per_exec{};
    int max_overall_mem{};
    int mem_per_frame{};
    int min_mem_per_proc{};
    int max_mem_per_proc{};

    [[nodiscard]] bool validate() const
    {
        return num_cpu >= 1 && num_cpu <= 128 &&
               (scheduler == "fcfs" || scheduler == "rr") &&
               quantum_cycles >= 1 && quantum_cycles <= std::numeric_limits<int>::max() &&
               batch_process_freq >= 1 && batch_process_freq <= (1 << 24) &&
               min_ins >= 1 && min_ins <= std::numeric_limits<int>::max() &&
               max_ins >= 1 && max_ins <= std::numeric_limits<int>::max() &&
               min_mem_per_proc >= 64 && max_mem_per_proc <= 65536 &&
               min_mem_per_proc <= max_mem_per_proc;
    }
};

class ConfigReader {
    std::unordered_map<std::string, std::string> config_data;

    static constexpr auto trim = [](std::string_view str) -> std::string_view {
        auto start = std::ranges::find_if_not(str, ::isspace);
        auto end = std::ranges::find_if_not(str | std::views::reverse, ::isspace).base();
        return {start, end};
    };

    std::expected<std::pair<std::string, std::string>, ConfigError> parse_line(std::string_view line)
    {
        line = trim(line);

        if (line.empty() || line.starts_with('#') || line.starts_with("//")) {
            return std::unexpected(ConfigError::InvalidFormat);
        }

        auto space_pos = line.find(' ');
        if (space_pos == std::string_view::npos) {
            return std::unexpected(ConfigError::InvalidFormat);
        }

        auto key = trim(line.substr(0, space_pos));
        auto value = trim(line.substr(space_pos + 1));

        return std::make_pair(std::string(key), std::string(value));
    }

    std::expected<int, ConfigError> parse_int(const std::string& value)
    {
        int result = 0;
        auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);

        if (ec != std::errc() || ptr != value.data() + value.size()) {
            return std::unexpected(ConfigError::InvalidValue);
        }
        return result;
    }

    template <typename T>
    std::expected<T, ConfigError> get_value(const std::string& key);

    // Add specific type implementations here if needed, but I don't think we will add any
public:
    std::expected<void, ConfigError> load_file(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return std::unexpected(ConfigError::FileNotFound);
        }

        std::string line;
        while (std::getline(file, line)) {
            auto result = parse_line(line);
            if (result.has_value()) {
                config_data[result->first] = result->second;
            }
        }

        return {};
    }

    std::expected<CPUConfig, ConfigError> parse_config();
};


#endif //CONFIG_READER_H

//
// Created by Alfon on 6/25/2025.
//

#include "config_reader.h"

template<>
std::expected<int, ConfigError> ConfigReader::get_value<int>(const std::string &key)
{
    auto it = config_data.find(key);
    if (it == config_data.end()) {
        return std::unexpected(ConfigError::MissingParameter);
    }
    return parse_int(it->second);
}

template<>
std::expected<std::string, ConfigError> ConfigReader::get_value<std::string>(const std::string &key)
{
    auto it = config_data.find(key);
    if (it == config_data.end()) {
        return std::unexpected(ConfigError::MissingParameter);
    }

    std::string value = it->second;

    if (value.length() >= 2 && value.starts_with('"') && value.ends_with('"')) {
        return value.substr(1, value.size() - 2);
    }

    return value;
}

std::expected<CPUConfig, ConfigError> ConfigReader::parse_config()
{
    CPUConfig config;

    if (auto num_cpu = get_value<int>("num-cpu")) {
        config.num_cpu = *num_cpu;
    }

    if (auto scheduler = get_value<std::string>("scheduler")) {
        config.scheduler = *scheduler;
    }

    if (auto quantum = get_value<int>("quantum-cycles")) {
        config.quantum_cycles = *quantum;
    }

    if (auto batch_freq = get_value<int>("batch-process-freq")) {
        config.batch_process_freq = *batch_freq;
    }

    if (auto min_ins = get_value<int>("min-ins")) {
        config.min_ins = *min_ins;
    }

    if (auto max_ins = get_value<int>("max-ins")) {
        config.max_ins = *max_ins;
    }

    if (auto delays = get_value<int>("delays-per-exec")) {
        config.delays_per_exec = *delays;
    }

    if (!config.validate()) {
        return std::unexpected(ConfigError::InvalidValue);
    }

    return config;
}

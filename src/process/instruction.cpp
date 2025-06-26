//
// Created by Alfon on 6/12/2025.
//

#include <iostream>
#include "instruction.h"
#include <fstream>
#include "../cpu_tick.h"

void PrintInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S%p");
    std::string timestamp = oss.str();


    std::string log_entry = std::format("({}) Core {}: \"{}\"", timestamp, core_id, message);
    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back("[PRINT] " + message);
}

std::string PrintInstruction::get_type_name() const
{
    return "PRINT";
}

void DeclareInstruction::execute(Process &process)
{
    size_t address = process.memory->get_var_address(process.symbol_table, var_name);
    process.memory->write_word(address, value);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"DECLARE {} = {}\"",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        process.get_assigned_core(), var_name, value);

    // Add to output buffer for viewing
    // process.output_buffer.push_back(log_entry);

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

std::string DeclareInstruction::get_type_name() const
{
    return "DECLARE";
}

void AddInstruction::execute(Process &process)
{
    size_t var1_address = process.memory->get_var_address(process.symbol_table, var1);

    std::string val2_str, val3_str;

    if (use_val2) {
        val2_str = std::to_string(val2);
    } else {
        const size_t var2_address = process.memory->get_var_address(process.symbol_table, var2);
        val2 = process.memory->read_word(var2_address);
        val2_str = std::format("{}({})", var2, val2);
    }

    if (use_val3) {
        val3_str = std::to_string(val3);
    } else {
        const size_t var3_address = process.memory->get_var_address(process.symbol_table, var3);
        val3 = process.memory->read_word(var3_address);
        val3_str = std::format("{}({})", var3, val3);
    }

    uint32_t result = static_cast<uint32_t>(val2) + static_cast<uint32_t>(val3);
    if (result > UINT16_MAX) {
        result = UINT16_MAX; // clamp result
    }

    process.memory->write_word(var1_address, static_cast<uint16_t>(result));

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"ADD {} = {} + {} = {}\"",
    tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
    process.get_assigned_core(), var1, val2_str, val3_str, static_cast<uint16_t>(result));

    // Add to output buffer for viewing
    // process.output_buffer.push_back(log_entry);

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

std::string AddInstruction::get_type_name() const
{
    return "ADD";
}

void SubtractInstruction::execute(Process &process)
{
    size_t var1_address = process.memory->get_var_address(process.symbol_table, var1);

    std::string val2_str, val3_str;

    if (use_val2) {
        val2_str = std::to_string(val2);
    } else {
        const size_t var2_address = process.memory->get_var_address(process.symbol_table, var2);
        val2 = process.memory->read_word(var2_address);
        val2_str = std::format("{}({})", var2, val2);
    }

    if (use_val3) {
        val3_str = std::to_string(val3);
    } else {
        const size_t var3_address = process.memory->get_var_address(process.symbol_table, var3);
        val3 = process.memory->read_word(var3_address);
        val3_str = std::format("{}({})", var3, val3);
    }

    uint16_t result;
    if (val2 >= val3) {
        result = val2 - val3;
    } else {
        result = 0;
    }

    process.memory->write_word(var1_address, result);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"SUBTRACT {} = {} - {} = {}\"",
     tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
     process.get_assigned_core(), var1, val2_str, val3_str, result);

    // Add to output buffer for viewing
    // process.output_buffer.push_back(log_entry);

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

std::string SubtractInstruction::get_type_name() const
{
    return "SUBTRACT";
}

void SleepInstruction::execute(Process &process)
{
    uint32_t sleep_start = get_cpu_tick();
    uint32_t sleep_until = sleep_start + x;
    process.set_state(ProcessState::eWaiting);
    process.sleep_until_tick.store(sleep_until);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"SLEEP  start: {} end: {}\"",
     tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
     process.get_assigned_core(), sleep_start, sleep_until);

    // Add to output buffer for viewing
    // process.output_buffer.push_back(log_entry);
}

std::string SleepInstruction::get_type_name() const
{
    return "SLEEP";
}

void ForInstruction::execute(Process &process)
{
    for (uint64_t i = 0; i < repeats; i++) {
        for (const auto& instruction : sub_instructions) {
            instruction->execute(process);
        }
    }
    std::cerr << "FATAL ERROR in ForInstruction::execute: A ForInstruction was executed directly instead of being expanded. Program will terminate." << std::endl;
    std::abort();
}

std::string ForInstruction::get_type_name() const
{
    return "FOR";
}
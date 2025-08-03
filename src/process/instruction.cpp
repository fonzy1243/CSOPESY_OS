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

    std::string final_message = message;
    if (has_variable) {
        size_t var_address = process.get_var_address(variable_name);
        uint16_t var_value = process.read_memory_word(var_address);

        final_message = message + " " + variable_name + " = " + std::to_string(var_value);
    }
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"PRINT {}\"",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        core_id, final_message);

    std::lock_guard lock(process.log_mutex);

    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back("[PRINT] " + final_message);
}

std::string PrintInstruction::get_type_name() const
{
    return "PRINT";
}

void DeclareInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();

    size_t address = process.get_var_address(var_name);
    process.write_memory_word(address, value);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"DECLARE {} = {}\"",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        core_id, var_name, value);

    std::lock_guard lock(process.log_mutex);

    // Add to print log for viewing with process-smi
    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back(log_entry);
}

std::string DeclareInstruction::get_type_name() const
{
    return "DECLARE";
}

void AddInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();
    size_t var1_address = process.get_var_address(var1);

    std::string val2_str, val3_str;

    if (use_val2) {
        val2_str = std::to_string(val2);
    } else {
        const size_t var2_address = process.get_var_address(var2);
        val2 = process.read_memory_word(var2_address);
        val2_str = std::format("{}({})", var2, val2);
    }

    if (use_val3) {
        val3_str = std::to_string(val3);
    } else {
        const size_t var3_address = process.get_var_address(var3);
        val3 = process.read_memory_word(var3_address);
        val3_str = std::format("{}({})", var3, val3);
    }

    uint32_t result = static_cast<uint32_t>(val2) + static_cast<uint32_t>(val3);
    if (result > UINT16_MAX) {
        result = UINT16_MAX; // clamp result
    }

    process.write_memory_word(var1_address, static_cast<uint16_t>(result));

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"ADD {} = {} + {} = {}\"",
    tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
    core_id, var1, val2_str, val3_str, static_cast<uint16_t>(result));

    std::lock_guard lock(process.log_mutex);

    // Add to print_logs for viewing
    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back(log_entry);
}

std::string AddInstruction::get_type_name() const
{
    return "ADD";
}

void SubtractInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();
    size_t var1_address = process.get_var_address(var1);

    std::string val2_str, val3_str;

    if (use_val2) {
        val2_str = std::to_string(val2);
    } else {
        const size_t var2_address = process.get_var_address(var2);
        val2 = process.read_memory_word(var2_address);
        val2_str = std::format("{}({})", var2, val2);
    }

    if (use_val3) {
        val3_str = std::to_string(val3);
    } else {
        const size_t var3_address = process.get_var_address(var3);
        val3 = process.read_memory_word(var3_address);
        val3_str = std::format("{}({})", var3, val3);
    }

    uint16_t result;
    if (val2 >= val3) {
        result = val2 - val3;
    } else {
        result = 0;
    }

    process.write_memory_word(var1_address, result);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"SUBTRACT {} = {} - {} = {}\"",
     tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
     core_id, var1, val2_str, val3_str, result);

    std::lock_guard lock(process.log_mutex);

    // Add to print_logs for viewing
    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back(log_entry);
}

std::string SubtractInstruction::get_type_name() const
{
    return "SUBTRACT";
}

void SleepInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();
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
     core_id, sleep_start, sleep_until);

    std::lock_guard lock(process.log_mutex);

    // Add to print logs for viewing
    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back(log_entry);
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

void ReadInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();
    uint16_t val = process.read_memory_word(address);
    uint32_t var_address = process.get_var_address(var);
    process.write_memory_word(var_address, val);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"READ {} @ 0x{:04X} -> {}\"",
    tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
    core_id, var, address, val);

    std::lock_guard lock(process.log_mutex);

    // Add to print_logs for viewing
    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back(log_entry);
}

std::string ReadInstruction::get_type_name() const
{
    return "READ";
}
//
// Created by Alfon on 6/12/2025.
//

#include "instruction.h"
#include "../cpu_tick.h"

void PrintInstruction::execute(Process &process)
{

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

}

std::string SleepInstruction::get_type_name() const
{
    return "SLEEP";
}

void ForInstruction::execute(Process &process) {  }

std::string ForInstruction::get_type_name() const
{
    return "FOR";
}
//
// Created by Alfon on 6/12/2025.
//

#include <iostream>
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
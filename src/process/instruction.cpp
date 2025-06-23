//
// Created by Alfon on 6/12/2025.
//

#include "instruction.h"

void Instruction::execute(Process &process)
{
    switch (type) {
        case InstructionType::ePrint:
            print(process, message);
            break;
        case InstructionType::eDeclare:
            declare(process, variable_name, value);
            break;
        case InstructionType::eAdd:
            add(process);
            break;
        case InstructionType::eSubtract:
            subtract(process);
            break;
        case InstructionType::eSleep:
            sleep(sleep_duration);
            break;
        case InstructionType::eFor:
            _for(sub_instructions, repeat_count);
            break;
        default:
            break;
    }
    process.current_instruction.fetch_add(1);
}

void Instruction::print(Process &process, std::string msg)
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\")",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, process.get_assigned_core(), msg
        );

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

// helper function just to fetch variable from memory or to create variable
size_t get_or_create_variable_index(Process &process, const std::string &var_name) {
    // iterate through and check if variable exists
    auto it = process.variable_indices.find(var_name);
    if (it != process.variable_indices.end()) {
        return it->second;  // return existing index
    }

    // Create new variable
    size_t new_index = process.next_free_index++;
    process.variable_indices[var_name] = new_index;

    // Expand memory array if needed
    if (new_index >= process.memory.size()) {
        process.memory.resize(new_index + 1, 0);
    }

    return new_index;
}


void Instruction::declare(Process &process, std::string var, uint16_t value) {
    size_t index = get_or_create_variable_index(process, var);
    process.memory[index] = value;

    // same format as print logging but just log what values get declared
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"DECLARE {} = {}\"",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        process.get_assigned_core(), var, value);

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

void Instruction::add(Process &process) {
    // Get or create index for var1 (result variable)
    size_t var1_index = get_or_create_variable_index(process, var1);

    uint16_t val2, val3;
    std::string val2_str, val3_str;

    // Check if using value or variable for operand 2
    if (use_value2) {
        val2 = value2;
        val2_str = std::to_string(value2);
    } else {
        size_t var2_index = get_or_create_variable_index(process, var2);
        val2 = process.memory[var2_index];
        val2_str = std::format("{}({})", var2, val2);
    }

    // Check if using value or variable for operand 3
    if (use_value3) {
        val3 = value3;
        val3_str = std::to_string(value3);
    } else {
        size_t var3_index = get_or_create_variable_index(process, var3);
        val3 = process.memory[var3_index];
        val3_str = std::format("{}({})", var3, val3);
    }

    // cast to uint32 to avoid overflow then we clamp after, not sure with specs yet
    uint32_t result = static_cast<uint32_t>(val2) + static_cast<uint32_t>(val3);
    if (result > UINT16_MAX) {
        result = UINT16_MAX;
    }

    // Store result in memory array
    process.memory[var1_index] = static_cast<uint16_t>(result);

    // logging part
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"ADD {} = {} + {} = {}\"",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        process.get_assigned_core(), var1, val2_str, val3_str, static_cast<uint16_t>(result));

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

void Instruction::subtract(Process &process) {
    // Get or create index for var1 (result variable)
    size_t var1_index = get_or_create_variable_index(process, var1);

    uint16_t val2, val3;
    std::string val2_str, val3_str;

    // Check if using value or variable for operand 2
    if (use_value2) {
        val2 = value2;
        val2_str = std::to_string(value2);
    } else {
        size_t var2_index = get_or_create_variable_index(process, var2);
        val2 = process.memory[var2_index];
        val2_str = std::format("{}({})", var2, val2);
    }

    // Check if using value or variable for operand 3
    if (use_value3) {
        val3 = value3;
        val3_str = std::to_string(value3);
    } else {
        size_t var3_index = get_or_create_variable_index(process, var3);
        val3 = process.memory[var3_index];
        val3_str = std::format("{}({})", var3, val3);
    }

    uint16_t result;
    if (val2 >= val3) {
        result = val2 - val3;
    } else {
        result = 0;  // Clamp to 0
    }

    // Store result in memory array
    process.memory[var1_index] = result;

    // logging
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"SUBTRACT {} = {} - {} = {}\"",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        process.get_assigned_core(), var1, val2_str, val3_str, result);

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

void Instruction::sleep(uint8_t x) {  }

void Instruction::_for(std::vector<std::shared_ptr<Instruction>> instructions, uint64_t repeats) {  }



//
// Created by Alfon on 6/12/2025.
//

#include <iostream>
#include "instruction.h"
#include <fstream>
#include "../cpu_tick.h"
#include <limits>

constexpr size_t INVALID_ADDRESS = 9999999;


void PrintInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();

    std::string final_message = message;
    if (has_variable) {
        size_t var_address = process.get_var_address(variable_name);
        if (var_address == -1000) {
            // Error: Could not get variable address (symbol table full)
            std::string error_log = std::format("PRINT: Cannot access variable '{}' - symbol table full", variable_name);
            std::lock_guard lock(process.log_mutex);
            process.print_logs.push_back(error_log);
            process.output_buffer.push_back("[ERROR] " + error_log);
            return;
        }
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
    if (address == INVALID_ADDRESS) {
        // Error: Could not get variable address (symbol table full)
        std::string error_log = std::format("DECLARE: Cannot declare variable '{}' - symbol table full (max 32 variables)", var_name);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &time_t);

        std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\"",
            tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
            core_id, error_log);

        std::lock_guard lock(process.log_mutex);
        process.print_logs.push_back(log_entry);
        process.output_buffer.push_back("[ERROR] " + error_log);
        return;
    }
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
    if (var1_address == INVALID_ADDRESS) {
        std::string error_log = std::format("ADD: Cannot access variable '{}' - symbol table full (max 32 variables)", var1);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &time_t);

        std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\"",
            tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
            core_id, error_log);

        std::lock_guard lock(process.log_mutex);
        process.print_logs.push_back(log_entry);
        process.output_buffer.push_back("[ERROR] " + error_log);
        return;
    }

    std::string val2_str, val3_str;

    if (use_val2) {
        val2_str = std::to_string(val2);
    } else {
        const size_t var2_address = process.get_var_address(var2);
        if (var2_address == INVALID_ADDRESS) {
            std::string error_log = std::format("ADD: Cannot access variable '{}' - symbol table full (max 32 variables)", var2);

            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &time_t);

            std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\"",
                tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
                core_id, error_log);

            std::lock_guard lock(process.log_mutex);
            process.print_logs.push_back(log_entry);
            process.output_buffer.push_back("[ERROR] " + error_log);
            return;
        }
        val2 = process.read_memory_word(var2_address);
        val2_str = std::format("{}({})", var2, val2);
    }

    if (use_val3) {
        val3_str = std::to_string(val3);
    } else {
        const size_t var3_address = process.get_var_address(var3);
        if (var3_address == INVALID_ADDRESS) {
            std::string error_log = std::format("ADD: Cannot access variable '{}' - symbol table full (max 32 variables)", var3);

            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &time_t);

            std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\"",
                tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
                core_id, error_log);

            std::lock_guard lock(process.log_mutex);
            process.print_logs.push_back(log_entry);
            process.output_buffer.push_back("[ERROR] " + error_log);
            return;
        }
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
    if (var1_address == INVALID_ADDRESS) {
        std::string error_log = std::format("SUBTRACT: Cannot access variable '{}' - symbol table full (max 32 variables)", var1);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &time_t);

        std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\"",
            tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
            core_id, error_log);

        std::lock_guard lock(process.log_mutex);
        process.print_logs.push_back(log_entry);
        process.output_buffer.push_back("[ERROR] " + error_log);
        return;
    }
    std::string val2_str, val3_str;

    if (use_val2) {
        val2_str = std::to_string(val2);
    } else {
        const size_t var2_address = process.get_var_address(var2);
        if (var2_address == INVALID_ADDRESS) {
            std::string error_log = std::format("SUBTRACT: Cannot access variable '{}' - symbol table full (max 32 variables)", var2);

            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &time_t);

            std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\"",
                tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
                core_id, error_log);

            std::lock_guard lock(process.log_mutex);
            process.print_logs.push_back(log_entry);
            process.output_buffer.push_back("[ERROR] " + error_log);
            return;
        }
        val2 = process.read_memory_word(var2_address);
        val2_str = std::format("{}({})", var2, val2);
    }

    if (use_val3) {
        val3_str = std::to_string(val3);
    } else {
        const size_t var3_address = process.get_var_address(var3);
        if (var3_address == INVALID_ADDRESS) {
            std::string error_log = std::format("SUBTRACT: Cannot access variable '{}' - symbol table full (max 32 variables)", var3);

            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &time_t);

            std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"{}\"",
                tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
                core_id, error_log);

            std::lock_guard lock(process.log_mutex);
            process.print_logs.push_back(log_entry);
            process.output_buffer.push_back("[ERROR] " + error_log);
            return;
        }
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

 void WriteInstruction::execute(Process &process)
{
    uint16_t core_id = process.assigned_core.load();
    uint16_t val = use_var ? process.read_memory_word(process.get_var_address(var_name)) : literal; //literal or variable
    process.write_memory_word(address, val);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}) Core: {} \"WRITE @0x{:04X} -> {}\"",
     tm.tm_mon+1, tm.tm_mday, tm.tm_year+1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
     core_id, address, val);

    std::lock_guard lock(process.log_mutex);

    // Add to print_logs for viewing
    process.print_logs.push_back(log_entry);
    process.output_buffer.push_back(log_entry);
}

std::string WriteInstruction::get_type_name()const
{
    return "WRITE";
}

uint16_t InstructionEncoder::encode_string(const std::string &str)
{
    auto it = str_table.find(str);
    if (it != str_table.end()) return it->second;

    uint16_t id = next_str_id++;
    str_table[str] = id;

    if (r_str_table.size() <= id) {
        r_str_table.resize(id + 1);
    }
    r_str_table[id] = str;

    return id;
}

std::string InstructionEncoder::decode_string(const uint16_t str_id) const
{
    if (str_id < r_str_table.size()) return r_str_table[str_id];

    return "<MISSING_STRING_" + std::to_string(str_id) + ">";
}

EncodedInstruction InstructionEncoder::encode_instruction(const std::shared_ptr<IInstruction> &instruction)
{
    EncodedInstruction encoded = {0};

        if (const auto print_inst = std::dynamic_pointer_cast<PrintInstruction>(instruction)) {
        encoded.opcode = static_cast<uint8_t>(InstructionOpcode::ePRINT);
        encoded.operand1 = encode_string(print_inst->get_message());
        if (print_inst->has_var()) {
            encoded.flags |= 0x01; // Has variable flag
            encoded.operand2 = encode_string(print_inst->get_variable_name());
        }
    }
    else if (const auto declare_inst = std::dynamic_pointer_cast<DeclareInstruction>(instruction)) {
        encoded.opcode = static_cast<uint8_t>(InstructionOpcode::eDECLARE);
        encoded.operand1 = encode_string(declare_inst->get_var_name());
        encoded.operand2 = declare_inst->get_value();
    }
    else if (const auto add_inst = std::dynamic_pointer_cast<AddInstruction>(instruction)) {
        encoded.opcode = static_cast<uint8_t>(InstructionOpcode::eADD);
        encoded.operand1 = encode_string(add_inst->get_var1());

        if (add_inst->uses_val2()) {
            encoded.flags |= 0x01; // Operand 2 is literal
            encoded.operand2 = add_inst->get_val2();
        } else {
            encoded.operand2 = encode_string(add_inst->get_var2());
        }

        if (add_inst->uses_val3()) {
            encoded.flags |= 0x02; // Operand 3 is literal
            encoded.operand3 = add_inst->get_val3();
        } else {
            encoded.operand3 = encode_string(add_inst->get_var3());
        }
    }
    else if (const auto subtract_inst = std::dynamic_pointer_cast<SubtractInstruction>(instruction)) {
        encoded.opcode = static_cast<uint8_t>(InstructionOpcode::eSUBTRACT);
        encoded.operand1 = encode_string(subtract_inst->get_var1());

        if (subtract_inst->uses_val2()) {
            encoded.flags |= 0x01;
            encoded.operand2 = subtract_inst->get_val2();
        } else {
            encoded.operand2 = encode_string(subtract_inst->get_var2());
        }

        if (subtract_inst->uses_val3()) {
            encoded.flags |= 0x02;
            encoded.operand3 = subtract_inst->get_val3();
        } else {
            encoded.operand3 = encode_string(subtract_inst->get_var3());
        }
    }
    else if (const auto sleep_inst = std::dynamic_pointer_cast<SleepInstruction>(instruction)) {
        encoded.opcode = static_cast<uint8_t>(InstructionOpcode::eSLEEP);
        encoded.operand1 = sleep_inst->get_sleep_ticks();
    }
    else if (const auto read_inst = std::dynamic_pointer_cast<ReadInstruction>(instruction)) {
        encoded.opcode = static_cast<uint8_t>(InstructionOpcode::eREAD);
        encoded.operand1 = encode_string(read_inst->get_var());
        encoded.operand2 = static_cast<uint16_t>(read_inst->get_address());
        encoded.operand3 = static_cast<uint16_t>(read_inst->get_address() >> 16);
    }
    else if (const auto write_inst = std::dynamic_pointer_cast<WriteInstruction>(instruction)) {
        encoded.opcode = static_cast<uint8_t>(InstructionOpcode::eWRITE);
        encoded.operand1 = static_cast<uint16_t>(write_inst->get_address());
        encoded.operand2 = static_cast<uint16_t>(write_inst->get_address() >> 16);

        if (write_inst->uses_var()) {
            encoded.flags |= 0x01; // Uses variable
            encoded.operand3 = encode_string(write_inst->get_var_name());
        } else {
            encoded.operand3 = write_inst->get_literal();
        }
    }

    return encoded;
}

std::shared_ptr<IInstruction> InstructionEncoder::decode_instruction(const EncodedInstruction& encoded) const {
    switch (static_cast<InstructionOpcode>(encoded.opcode)) {
        case InstructionOpcode::ePRINT: {
            std::string message = decode_string(encoded.operand1);
            if (encoded.flags & 0x01) {
                std::string var_name = decode_string(encoded.operand2);
                return std::make_shared<PrintInstruction>(message, var_name);
            }
            return std::make_shared<PrintInstruction>(message);
        }

        case InstructionOpcode::eDECLARE: {
            std::string var_name = decode_string(encoded.operand1);
            return std::make_shared<DeclareInstruction>(var_name, encoded.operand2);
        }

        case InstructionOpcode::eADD: {
            std::string var1 = decode_string(encoded.operand1);

            if ((encoded.flags & 0x01) && (encoded.flags & 0x02)) {
                // Both operands are literals
                return std::make_shared<AddInstruction>(var1, encoded.operand2, encoded.operand3);
            }
            if (encoded.flags & 0x01) {
                // Operand 2 is literal, operand 3 is variable
                std::string var3 = decode_string(encoded.operand3);
                return std::make_shared<AddInstruction>(var1, encoded.operand2, var3);
            }
            if (encoded.flags & 0x02) {
                // Operand 2 is variable, operand 3 is literal
                std::string var2 = decode_string(encoded.operand2);
                return std::make_shared<AddInstruction>(var1, var2, encoded.operand3);
            }
            // Both operands are variables
            std::string var2 = decode_string(encoded.operand2);
            std::string var3 = decode_string(encoded.operand3);
            return std::make_shared<AddInstruction>(var1, var2, var3);
        }

        case InstructionOpcode::eSUBTRACT: {
            std::string var1 = decode_string(encoded.operand1);

            if ((encoded.flags & 0x01) && (encoded.flags & 0x02)) {
                return std::make_shared<SubtractInstruction>(var1, encoded.operand2, encoded.operand3);
            }
            if (encoded.flags & 0x01) {
                std::string var3 = decode_string(encoded.operand3);
                return std::make_shared<SubtractInstruction>(var1, encoded.operand2, var3);
            }
            if (encoded.flags & 0x02) {
                std::string var2 = decode_string(encoded.operand2);
                return std::make_shared<SubtractInstruction>(var1, var2, encoded.operand3);
            }
            std::string var2 = decode_string(encoded.operand2);
            std::string var3 = decode_string(encoded.operand3);
            return std::make_shared<SubtractInstruction>(var1, var2, var3);
        }

        case InstructionOpcode::eSLEEP:
            return std::make_shared<SleepInstruction>(static_cast<uint8_t>(encoded.operand1));

        case InstructionOpcode::eREAD: {
            std::string var = decode_string(encoded.operand1);
            uint32_t address = encoded.operand2 | (static_cast<uint32_t>(encoded.operand3) << 16);
            return std::make_shared<ReadInstruction>(var, address);
        }

        case InstructionOpcode::eWRITE: {
            uint32_t address = encoded.operand1 | (static_cast<uint32_t>(encoded.operand2) << 16);
            if (encoded.flags & 0x01) {
                std::string var_name = decode_string(encoded.operand3);
                return std::make_shared<WriteInstruction>(address, var_name);
            }
            return std::make_shared<WriteInstruction>(address, encoded.operand3);
        }

        default:
            return nullptr;
    }
}

void InstructionEncoder::store_str_table(const Process & process, const uint32_t base_address) const
{
    // Store number of strings first
    const uint16_t num_strings = static_cast<uint16_t>(r_str_table.size());
    process.write_memory_word(base_address, num_strings);

    uint32_t current_addr = base_address + 2;

    // Store each string with its length prefix
    for (size_t i = 1; i < r_str_table.size(); ++i) {
        const std::string &str = r_str_table[i];
        const uint16_t len = static_cast<uint16_t>(str.length());

        // Store length
        process.write_memory_word(current_addr, len);
        current_addr += 2;

        // Store string data
        for (const char c : str) {
            process.write_memory_byte(current_addr++, static_cast<uint8_t>(c));
        }
    }
}


void InstructionEncoder::load_str_table(const Process & process, const uint32_t base_address) {
    const uint16_t num_strings = process.read_memory_word(base_address);
    uint32_t current_addr = base_address + 2;

    r_str_table.clear();
    r_str_table.resize(num_strings);
    str_table.clear();

    for (uint16_t i = 1; i < num_strings; ++i) {
        const uint16_t len = process.read_memory_word(current_addr);
        current_addr += 2;

        std::string str;
        str.reserve(len);

        for (uint16_t j = 0; j < len; ++j) {
            str += static_cast<char>(process.read_memory_byte(current_addr++));
        }

        r_str_table[i] = str;
        str_table[str] = i;
    }

    next_str_id = num_strings;
}
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
            declare(variable_name, value);
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

    std::string log_entry = std::format("({:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}AM) Core: {} \"{}\")",
        tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, process.get_assigned_core(), msg
        );

    if (process.log_file.is_open()) {
        process.log_file << log_entry << std::endl;
        process.log_file.flush();
    }
}

void Instruction::declare(std::string var, uint16_t value) {  }

void Instruction::sleep(uint8_t x) {  }

void Instruction::_for(std::vector<std::shared_ptr<Instruction>> instructions, uint64_t repeats) {  }



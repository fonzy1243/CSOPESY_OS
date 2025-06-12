//
// Created by Alfon on 6/12/2025.
//

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <thread>
#include <iostream>
#include <format>
#include "process.h"

class Process;

enum class InstructionType
{
    ePrint,
    eDeclare,
    eAdd,
    eSubtract,
    eSleep,
    eFor
};

class Instruction {
    void print(Process &process, std::string msg);
    void declare(std::string var, uint16_t value);
    void sleep(uint8_t x);
    void _for(std::vector<std::shared_ptr<Instruction>> instructions, uint64_t repeats);
public:
    InstructionType type;
    std::string message;
    std::string variable_name;
    uint64_t repeat_count;
    uint16_t value;
    uint8_t sleep_duration;
    std::vector<std::shared_ptr<Instruction>> sub_instructions;

    Instruction(const InstructionType type, const std::string &msg) : type(type), message(msg) {}

    Instruction(const InstructionType type, const std::string &var, uint16_t val) : type(type), variable_name(var), value(val) {}

    Instruction(const InstructionType type, uint8_t duration) : type(type), sleep_duration(duration) {}

    Instruction(const InstructionType type, std::vector<std::shared_ptr<Instruction>> instructions, uint64_t repeats) : type(type), sub_instructions(instructions), repeat_count(repeats) {}

    void execute(Process &process);
};



#endif //INSTRUCTION_H

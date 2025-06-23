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
    void declare(Process &process, std::string var, uint16_t value);
    void add(Process &process);
    void subtract(Process &process);
    void sleep(uint8_t x);
    void _for(std::vector<std::shared_ptr<Instruction>> instructions, uint64_t repeats);


public:
    InstructionType type;
    std::string message;
    std::string variable_name;
    uint64_t repeat_count;
    uint16_t value;
    uint8_t sleep_duration;

    // mostly for add and subtract functions
    std::string var1, var2, var3;
    uint16_t value2, value3;
    bool use_value2, use_value3;

    std::vector<std::shared_ptr<Instruction>> sub_instructions;

    Instruction(const InstructionType type, const std::string &msg) : type(type), message(msg) {}

    Instruction(const InstructionType type, const std::string &var, uint16_t val) : type(type), variable_name(var), value(val) {}

    Instruction(const InstructionType type, uint8_t duration) : type(type), sleep_duration(duration) {}

    Instruction(const InstructionType type, std::vector<std::shared_ptr<Instruction>> instructions, uint64_t repeats) : type(type), sub_instructions(instructions), repeat_count(repeats) {}

    // constructors for add and subtract, so they can have variables or values
    Instruction(const InstructionType type, const std::string &var1, const std::string &var2, const std::string &var3)
    : type(type), var1(var1), var2(var2), var3(var3), use_value2(false), use_value3(false) {}

    Instruction(const InstructionType type, const std::string &var1, const std::string &var2, uint16_t val3)
        : type(type), var1(var1), var2(var2), value3(val3), use_value2(false), use_value3(true) {}

    Instruction(const InstructionType type, const std::string &var1, uint16_t val2, const std::string &var3)
        : type(type), var1(var1), var3(var3), value2(val2), use_value2(true), use_value3(false) {}

    Instruction(const InstructionType type, const std::string &var1, uint16_t val2, uint16_t val3)
        : type(type), var1(var1), value2(val2), value3(val3), use_value2(true), use_value3(true) {}

    void execute(Process &process);
};



#endif //INSTRUCTION_H

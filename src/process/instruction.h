//
// Created by Alfon on 6/12/2025.
//

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <format>
#include "process.h"

class Process;

class IInstruction {
public:
    virtual ~IInstruction() = default;
    virtual void execute(Process& process) = 0;
    virtual std::string get_type_name() const = 0;
};

class PrintInstruction : public IInstruction
{
    std::string message;
public:
    PrintInstruction(const std::string &message) : message(message) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

class DeclareInstruction : public IInstruction
{
    std::string var_name;
    uint16_t value;
public:
    DeclareInstruction(const std::string& var, const uint16_t value) : var_name(var), value(value) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

class AddInstruction : public IInstruction
{
    uint16_t val1;
    uint16_t val2;
    uint16_t val3;
public:
    AddInstruction(const uint16_t val1, const uint16_t val2, const uint16_t val3) : val1(val1), val2(val2), val3(val3) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

class SubtractInstruction : public IInstruction
{
    uint16_t val1;
    uint16_t val2;
    uint16_t val3;
public:
    SubtractInstruction(const uint16_t val1, const uint16_t val2, const uint16_t val3) : val1(val1), val2(val2), val3(val3) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

class SleepInstruction : public IInstruction
{
    uint8_t x;
public:
    SleepInstruction(const uint8_t x) : x(x) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

class ForInstruction : public IInstruction
{
    std::vector<std::shared_ptr<IInstruction>> sub_instructions;
    uint16_t repeats;
public:
    ForInstruction(const std::vector<std::shared_ptr<IInstruction>> &instructions, const uint16_t repeats) : sub_instructions(instructions), repeats(repeats) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

#endif //INSTRUCTION_H

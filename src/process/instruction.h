//
// Created by Alfon on 6/12/2025.
//

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

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
    std::string variable_name;
    bool has_variable;
public:
    PrintInstruction(const std::string &message) : message(message), has_variable(false) {}
    PrintInstruction(const std::string &message, const std::string &var_name) : message(message), variable_name(var_name), has_variable(true) {}

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
    std::string var1;
    std::string var2;
    std::string var3;

    uint16_t val2 = 0;
    uint16_t val3 = 0;

    bool use_val2;
    bool use_val3;
public:
    AddInstruction(const std::string& var1, const std::string& var2, const std::string& var3) : var1(var1), var2(var2), var3(var3), use_val2(false), use_val3(false) {}
    AddInstruction(const std::string& var1, const std::string& var2, const uint16_t val3) : var1(var1), var2(var2), val3(val3), use_val2(false), use_val3(true) {}
    AddInstruction(const std::string& var1, const uint16_t val2, const std::string& var3) : var1(var1), var3(var3), val2(val2), use_val2(true), use_val3(false) {}
    AddInstruction(const std::string& var1, const uint16_t val2, const uint16_t val3) : var1(var1), val2(val2), val3(val3), use_val2(true), use_val3(true) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

class SubtractInstruction : public IInstruction
{
    std::string var1;
    std::string var2;
    std::string var3;

    uint16_t val2 = 0;
    uint16_t val3 = 0;

    bool use_val2;
    bool use_val3;
public:
    SubtractInstruction(const std::string& var1, const std::string& var2, const std::string& var3) : var1(var1), var2(var2), var3(var3), use_val2(false), use_val3(false) {}
    SubtractInstruction(const std::string& var1, const std::string& var2, const uint16_t val3) : var1(var1), var2(var2), val3(val3), use_val2(false), use_val3(true) {}
    SubtractInstruction(const std::string& var1, const uint16_t val2, const std::string& var3) : var1(var1), var3(var3), val2(val2), use_val2(true), use_val3(false) {}
    SubtractInstruction(const std::string& var1, const uint16_t val2, const uint16_t val3) : var1(var1), val2(val2), val3(val3), use_val2(true), use_val3(true) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
};

class SleepInstruction : public IInstruction
{
    uint8_t x; // Number of CPU ticks to sleep
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
    void execute(Process &process) override;
    std::vector<std::shared_ptr<IInstruction>> &get_sub_instructions() { return sub_instructions; }
    uint16_t get_repeats() const { return repeats; }
    std::string get_type_name() const override;
};

class ReadInstruction : public IInstruction
{
    std::string var;
    uint32_t address;
public:
    ReadInstruction(const std::string& var, uint32_t address) : var(var), address(address) {}
    void execute(Process &process) override;
    std::string get_type_name() const override;
};

#endif //INSTRUCTION_H

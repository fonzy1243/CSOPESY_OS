//
// Created by Alfon on 6/12/2025.
//

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "process.h"

class Process;

enum class InstructionOpcode : uint8_t
{
    ePRINT = 0x01,
    eDECLARE = 0x02,
    eADD = 0x03,
    eSUBTRACT = 0x04,
    eSLEEP = 0x05,
    eFOR = 0x06,
    eREAD = 0x07,
    eWRITE = 0x08,
};

struct EncodedInstruction
{
    uint8_t opcode;
    uint8_t flags;
    uint16_t operand1;
    uint16_t operand2;
    uint16_t operand3;
};

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
    const std::string& get_message() const { return message; }
    const std::string& get_variable_name() const { return variable_name; }
    bool has_var() const { return has_variable; }
};

class DeclareInstruction : public IInstruction
{
    std::string var_name;
    uint16_t value;
public:
    DeclareInstruction(const std::string& var, const uint16_t value) : var_name(var), value(value) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
    const std::string& get_var_name() const { return var_name; }
    uint16_t get_value() const { return value; }
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
    const std::string& get_var1() const { return var1; }
    const std::string& get_var2() const { return var2; }
    const std::string& get_var3() const { return var3; }
    uint16_t get_val2() const { return val2; }
    uint16_t get_val3() const { return val3; }
    bool uses_val2() const { return use_val2; }
    bool uses_val3() const { return use_val3; }
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
    const std::string& get_var1() const { return var1; }
    const std::string& get_var2() const { return var2; }
    const std::string& get_var3() const { return var3; }
    uint16_t get_val2() const { return val2; }
    uint16_t get_val3() const { return val3; }
    bool uses_val2() const { return use_val2; }
    bool uses_val3() const { return use_val3; }
};

class SleepInstruction : public IInstruction
{
    uint8_t x; // Number of CPU ticks to sleep
public:
    SleepInstruction(const uint8_t x) : x(x) {}
    void execute(Process& process) override;
    std::string get_type_name() const override;
    uint8_t get_sleep_ticks() const { return x; }
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
    const std::string& get_var() const { return var; }
    uint32_t get_address() const { return address; }
};

class WriteInstruction : public IInstruction
{
    uint32_t address;
    bool use_var;
    std::string var_name;
    uint16_t literal;

public:
    // literal constructor
    WriteInstruction(uint32_t addr, uint16_t lit) : address(addr), use_var(false), literal(lit) {}

    // variable constructor
    WriteInstruction(uint32_t addr, const std::string &var) : address(addr), use_var(true), var_name(var) {}

    void execute(Process &process) override;
    std::string get_type_name() const override;
    uint32_t get_address() const { return address; }
    bool uses_var() const { return use_var; }
    const std::string& get_var_name() const { return var_name; }
    uint16_t get_literal() const { return literal; }
};

class InstructionEncoder
{
    std::unordered_map<std::string, uint16_t> str_table;
    std::vector<std::string> r_str_table;
    uint16_t next_str_id = 1;

    uint16_t encode_string(const std::string &str);
    [[nodiscard]] std::string decode_string(uint16_t str_id) const;

public:
    EncodedInstruction encode_instruction(const std::shared_ptr<IInstruction>& instruction);
    [[nodiscard]] std::shared_ptr<IInstruction> decode_instruction(const EncodedInstruction& encoded) const;

    void store_str_table(const Process & process, uint32_t base_address) const;
    void load_str_table(const Process & process, uint32_t base_address);
};

#endif //INSTRUCTION_H
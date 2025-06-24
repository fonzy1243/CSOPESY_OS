//
// Created by Alfon on 6/24/2025.
//

#include "memory.h"

uint8_t Memory::read_byte(uint16_t address) const
{
    if (address < memory.size()) {
        return memory[address];
    }
    return 0;
}

void Memory::write_byte(uint16_t address, uint8_t value)
{
    if (address < memory.size()) {
        memory[address] = value;
    }
}

uint16_t Memory::read_word(uint16_t address) const
{
    uint8_t low = read_byte(address);
    uint8_t high = read_byte(address + 1);
    return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

void Memory::write_word(uint16_t address, uint16_t value)
{
    write_byte(address, value & 0xff);
    write_byte(address + 1, (value >> 8) & 0xff);
}

void Memory::clear()
{
    std::fill(memory.begin(), memory.end(), 0);
}

size_t Memory::get_var_address(std::unordered_map<std::string, size_t> &symbol_table, const std::string &var_name)
{
    if (const auto it = symbol_table.find(var_name); it != symbol_table.end()) {
        return it->second;
    }

    const size_t address = next_free_index += 2;

    return address;
}
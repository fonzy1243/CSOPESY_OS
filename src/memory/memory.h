//
// Created by Alfon on 6/24/2025.
//

#ifndef MEMORY_H
#define MEMORY_H

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>


class Memory {
    std::vector<uint8_t> memory;
    size_t next_free_index = 0;
public:
    explicit Memory(const size_t size = 65536) : memory(size) {}

    uint8_t read_byte(uint16_t address) const;
    void write_byte(uint16_t address, uint8_t value);

    uint16_t read_word(uint16_t address) const;
    void write_word(uint16_t address, uint16_t value);

    void clear();
    size_t size() const { return memory.size(); }

    const uint8_t* data() const { return memory.data(); }

    // Fetch variable from memory or store if it is not yet stored.
    size_t get_var_address(std::unordered_map<std::string, size_t>& symbol_table, const std::string& var_name);
};



#endif //MEMORY_H

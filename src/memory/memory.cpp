//
// Created by Alfon on 6/24/2025.
//

#include "memory.h"
#include <algorithm>

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

    // Lock to prevent race conditions
    std::lock_guard lock(allocation_mutex);

    const size_t address = next_free_index += 2;

    symbol_table[var_name] = address;

    return address;
}

std::optional<size_t> Memory::allocate_block(int process_id, size_t size) {
    std::lock_guard<std::mutex> lock(allocation_mutex);
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->free && it->size >= size) {
            size_t start_addr = it->start;
            if (it->size > size) {
                // Split block
                Block new_block{it->start + size, it->size - size, true, -1};
                it->size = size;
                blocks.insert(it + 1, new_block);
            }
            it->free = false;
            it->process_id = process_id;
            return start_addr;
        }
    }
    return std::nullopt;
}

void Memory::free_block(int process_id) {
    std::lock_guard<std::mutex> lock(allocation_mutex);
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (!it->free && it->process_id == process_id) {
            it->free = true;
            it->process_id = -1;
            // Merge with next block if free
            if (it + 1 != blocks.end() && (it + 1)->free) {
                it->size += (it + 1)->size;
                blocks.erase(it + 1);
            }
            // Merge with previous block if free
            if (it != blocks.begin() && (it - 1)->free) {
                (it - 1)->size += it->size;
                blocks.erase(it);
            }
            break;
        }
    }
}

std::vector<Memory::Block> Memory::get_allocated_blocks() {
    std::lock_guard<std::mutex> lock(allocation_mutex);
    std::vector<Block> result;
    for (const auto& b : blocks) {
        if (!b.free) result.push_back(b);
    }
    return result;
}

std::vector<Memory::Block> Memory::get_free_blocks() {
    std::lock_guard<std::mutex> lock(allocation_mutex);
    std::vector<Block> result;
    for (const auto& b : blocks) {
        if (b.free) result.push_back(b);
    }
    return result;
}

size_t Memory::calculate_external_fragmentation(size_t min_block_size) {
    std::lock_guard<std::mutex> lock(allocation_mutex);
    size_t total = 0;
    for (const auto& b : blocks) {
        if (b.free && b.size < min_block_size) {
            total += b.size;
        }
    }
    return total;
}
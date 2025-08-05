//
// Created by Alfon on 6/24/2025.
//

#include "memory.h"
#include <algorithm>

std::optional<uint32_t> BackingStore::allocate_slot()
{
    std::lock_guard lock(store_mutex);
    for (size_t i = 0; i < allocated_slots.size(); i++) {
        if (!allocated_slots[i]) {
            allocated_slots[i] = true;
            return static_cast<uint32_t>(i);
        }
    }
    return std::nullopt;
}

void BackingStore::free_slot(uint32_t slot)
{
    std::lock_guard lock(store_mutex);
    if (slot < allocated_slots.size()) {
        allocated_slots[slot] = false;
    }
}

bool BackingStore::write_page(uint32_t slot, const uint8_t *page_data)
{
    std::lock_guard lock(store_mutex);
    page_file.seekp(slot * page_size);
    page_file.write(reinterpret_cast<const char *>(page_data), page_size);
    return page_file.good();
}

bool BackingStore::read_page(uint32_t slot, uint8_t *page_data)
{
    std::lock_guard lock(store_mutex);
    page_file.seekg(slot * page_size);
    page_file.read(reinterpret_cast<char*>(page_data), page_size);
    return page_file.good();
}

uint32_t Memory::find_victim_frame()
{
    if (replacement_queue.empty()) {
        for (size_t i = 0; i < frames.size(); i++) {
            if (!frames[i].is_free) {
                return static_cast<uint32_t>(i);
            }
        }
        return 0;
    }

    uint32_t victim_frame = replacement_queue.front();
    replacement_queue.pop();
    return victim_frame;
}

std::optional<uint32_t> Memory::allocate_frame()
{
    if (!free_frames.empty()) {
        uint32_t frame = free_frames.front();
        free_frames.pop();
        frames[frame].is_free = false;
        frames[frame].allocation_order = allocation_counter++;

        replacement_queue.push(frame);

        return frame;
    }

    return evict_and_allocate();
}

std::optional<uint32_t> Memory::evict_and_allocate()
{
    uint32_t victim_frame = find_victim_frame();

    uint32_t victim_process = frames[victim_frame].pid;
    uint32_t victim_page = frames[victim_frame].page_number;

    auto& process_space = process_spaces[victim_process];
    auto& page_entry = process_space->page_table[victim_page];

    if (page_entry.is_dirty()) {
        if (process_space->page_to_backing_slot.find(victim_page) == process_space->page_to_backing_slot.end()) {
            auto slot = backing_store->allocate_slot();
            if (!slot) return std::nullopt;
            process_space->page_to_backing_slot[victim_page] = *slot;
        }

        uint32_t physical_addr = get_physical_address(victim_frame, 0);
        backing_store->write_page(
            process_space->page_to_backing_slot[victim_page],
            &memory[physical_addr]
            );
        ++page_swaps;

        // increment page-out counter
        pages_paged_out.fetch_add(1);

    }

    page_entry.set_present(false);
    page_entry.set_dirty(false);

    process_space->allocated_pages--;

    frames[victim_frame].is_free = false;
    frames[victim_frame].allocation_order = allocation_counter++;
    replacement_queue.push(victim_frame);

    return victim_frame;
}

bool Memory::handle_page_fault(uint32_t pid, uint32_t page_number)
{
    auto it = process_spaces.find(pid);
    if (it == process_spaces.end()) {
        return false;
    }

    auto& process_space = it->second;

    // Instead of hard limit, use a reasonable maximum (e.g., 1GB worth of pages)
    const size_t MAX_VIRTUAL_PAGES = (1024 * 1024 * 1024) / page_size; // 1GB

    if (page_number >= MAX_VIRTUAL_PAGES) {
        return false; // True segmentation fault - way too high
    }

    // Expand page table if needed
    if (page_number >= process_space->page_table.entries.size()) {
        process_space->page_table.entries.resize(page_number + 1);
        process_space->max_pages = page_number + 1;
    }

    auto& page_entry = process_space->page_table[page_number];

    if (page_entry.is_present()) {
        return true;
    }

    ++page_faults;

    auto frame = allocate_frame();
    if (!frame) return false;

    frames[*frame].pid = pid;
    frames[*frame].page_number = page_number;

    uint32_t physical_addr = get_physical_address(*frame, 0);

    if (process_space->page_to_backing_slot.find(page_number) != process_space->page_to_backing_slot.end()) {
        backing_store->read_page(process_space->page_to_backing_slot[page_number], &memory[physical_addr]);

        // increment page-in counter
        pages_paged_in.fetch_add(1);
    } else {
        std::fill_n(&memory[physical_addr], page_size, 0);
    }

    page_entry.frame_num = *frame;
    page_entry.set_present(true);
    page_entry.set_valid(true);
    page_entry.set_referenced(true);

    process_space->allocated_pages++;

    return true;
}

bool Memory::is_valid_process_access(uint32_t pid, uint32_t virtual_address) const
{
    std::lock_guard lock(memory_mutex);

    auto it = process_spaces.find(pid);
    if (it == process_spaces.end()) return false;

    uint32_t page_num = get_page_number(virtual_address);
    auto& process_space = it->second;

    if (page_num >= process_space->max_pages) return false;

    if (!process_space->page_table.entries[page_num].is_valid()) return false;

    return true;
}


bool Memory::can_allocate_process(size_t required_memory_bytes) const
{
    std::lock_guard lock(memory_mutex);

    size_t pages_needed = calculate_pages_needed(required_memory_bytes);

    if (backing_store) {
        const size_t MAX_VIRTUAL_PAGES = (1024 * 1024 * 1024) / page_size;
        return pages_needed <= MAX_VIRTUAL_PAGES;
    }

    return pages_needed <= frames.size();
}

size_t Memory::get_available_memory() const
{
    std::lock_guard lock(memory_mutex);
    return free_frames.size() * page_size;
}

size_t Memory::get_total_allocated_memory() const
{
    return (frames.size() - free_frames.size()) * page_size;
}

size_t Memory::calculate_pages_needed(size_t memory_bytes) const
{
    return (memory_bytes + page_size - 1) / page_size;
}

size_t Memory::get_process_memory_usage(uint16_t pid) const {
    std::lock_guard lock(memory_mutex);

    auto it = process_spaces.find(pid);
    if (it == process_spaces.end()) {
        return 0;
    }

    return it->second->allocated_pages * page_size;
}

bool Memory::create_process_space(uint32_t pid, size_t memory_bytes)
{
    std::lock_guard lock(memory_mutex);

    if (process_spaces.contains(pid)) {
        return false;
    }

    size_t initial_pages = std::max(calculate_pages_needed(memory_bytes), static_cast<size_t>(1));
    process_spaces[pid] = std::make_unique<ProcessMemorySpace>(pid, initial_pages);

    return true;
}

void Memory::destroy_process_space(uint32_t pid)
{
    std::lock_guard lock(memory_mutex);

    auto it = process_spaces.find(pid);
    if (it == process_spaces.end())
        return;

    auto &process_space = it->second;

    size_t memory_to_free = process_space->max_pages * page_size;

    std::queue<uint32_t> new_replacement_queue;

    for (size_t i = 0; i < frames.size(); i++) {
        if (!frames[i].is_free && frames[i].pid == pid) {
            frames[i].is_free = true;
            free_frames.push(static_cast<uint32_t>(i));
        }
    }

    // Rebuild FIFO queue
    while (!replacement_queue.empty()) {
        uint32_t frame = replacement_queue.front();
        replacement_queue.pop();
        if (frames[frame].pid != pid) {
            new_replacement_queue.push(frame);
        }
    }

    replacement_queue = std::move(new_replacement_queue);

    for (const auto &[page, slot]: process_space->page_to_backing_slot) {
        backing_store->free_slot(slot);
    }

    process_spaces.erase(it);
}


std::optional<uint8_t> Memory::read_byte(uint16_t address) const
{
    if (address < memory.size()) {
        return memory[address];
    }
    return 0;
}

std::optional<uint8_t> Memory::read_byte(uint32_t pid, uint32_t virtual_address)
{
    if (!is_valid_process_access(pid, virtual_address)) return std::nullopt;

    std::lock_guard lock(memory_mutex);

    const auto it = process_spaces.find(pid);
    if (it == process_spaces.end())
        return std::nullopt;

    const uint32_t page_num = get_page_number(virtual_address);
    const uint32_t offset = get_page_offset(virtual_address);

    if (!handle_page_fault(pid, page_num)) return std::nullopt;

    auto &page_entry = it->second->page_table[page_num];
    page_entry.set_referenced(true);

    const uint32_t physical_addr = get_physical_address(page_entry.frame_num, offset);
    return memory[physical_addr];
}

bool Memory::write_byte(uint16_t address, uint8_t value)
{
    if (address < memory.size()) {
        memory[address] = value;
        return true;
    }

    return false;
}

bool Memory::write_byte(uint32_t pid, uint32_t virtual_address, uint8_t value)
{
    if (!is_valid_process_access(pid, virtual_address)) return false;

    std::lock_guard lock(memory_mutex);

    const auto it = process_spaces.find(pid);
    if (it == process_spaces.end()) return false;

    uint32_t page_num = get_page_number(virtual_address);
    uint32_t offset = get_page_offset(virtual_address);

    if (!handle_page_fault(pid, page_num)) return false;

    auto &page_entry = it->second->page_table[page_num];
    page_entry.set_referenced(true);
    page_entry.set_dirty(true);

    const uint32_t physical_addr = get_physical_address(page_entry.frame_num, offset);
    memory[physical_addr] = value;

    return true;
}


std::optional<uint16_t> Memory::read_word(uint16_t address) const
{
    const auto low = read_byte(address);
    const auto high = read_byte(address + 1);

    if (low.has_value() && high.has_value()) {
        return static_cast<uint16_t>(low.value()) | (static_cast<uint16_t>(high.value()) << 8);
    }

    return std::nullopt;
}

std::optional<uint16_t> Memory::read_word(uint32_t pid, uint32_t virtual_address)
{
    const auto low = read_byte(pid, virtual_address);
    const auto high = read_byte(pid, virtual_address + 1);

    if (low.has_value() && high.has_value()) {
        return static_cast<uint16_t>(*low) | (static_cast<uint16_t>(*high) << 8);
    }

    return std::nullopt;
}


bool Memory::write_word(uint16_t address, uint16_t value)
{
    write_byte(address, value & 0xff);
    write_byte(address + 1, (value >> 8) & 0xff);
}

bool Memory::write_word(uint32_t pid, uint16_t virtual_address, uint16_t value)
{
    const bool low_ok = write_byte(pid, virtual_address, value & 0xff);
    const bool high_ok = write_byte(pid, virtual_address + 1, (value >> 8) & 0xff);

    return low_ok && high_ok;
}


void Memory::clear()
{
    std::ranges::fill(memory, 0);
}

uint32_t Memory::get_var_address(uint32_t pid, std::unordered_map<std::string, size_t> &symbol_table, const std::string &var_name)
{
    std::lock_guard lock(memory_mutex);

    const auto it = process_spaces.find(pid);
    if (it == process_spaces.end()) return 0;

    if (auto var_it = symbol_table.find(var_name); var_it != symbol_table.end()) return static_cast<uint32_t>(var_it->second);

    auto& process_space = it->second;

    thread_local std::unordered_map<uint32_t, uint32_t> process_next_addr;

    if (process_next_addr.find(pid) == process_next_addr.end()) process_next_addr[pid] = 0;

    uint32_t virtual_addr = process_next_addr[pid];
    process_next_addr[pid] += 2;

    symbol_table[var_name] = virtual_addr;

    return virtual_addr;
}
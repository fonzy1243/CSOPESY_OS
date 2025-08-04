//
// Created by Alfon on 6/24/2025.
//

#ifndef MEMORY_H
#define MEMORY_H

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <queue>
#include <optional>

enum class PageFlags : uint8_t
{
    ePRESENT = 1 << 0,
    eDIRTY = 1 << 1,
    eREFERENCED = 1 << 2,
    eVALID = 1 << 3,
};

struct PageTableEntry
{
    uint32_t frame_num;
    uint8_t flags = 0;

    bool is_present() const { return flags & static_cast<uint8_t>(PageFlags::ePRESENT); }
    bool is_dirty() const { return flags & static_cast<uint8_t>(PageFlags::eDIRTY); }
    bool is_referenced() const { return flags & static_cast<uint8_t>(PageFlags::eREFERENCED); }
    bool is_valid() const { return flags & static_cast<uint8_t>(PageFlags::eVALID); }

    void set_present(bool value)
    {
        if (value) flags |= static_cast<uint8_t>(PageFlags::ePRESENT);
        else flags &= ~static_cast<uint8_t>(PageFlags::ePRESENT);
    }

    void set_dirty(bool value)
    {
        if (value) flags |= static_cast<uint8_t>(PageFlags::eDIRTY);
        else flags &= ~static_cast<uint8_t>(PageFlags::eDIRTY);
    }

    void set_referenced(bool value)
    {
        if (value) flags |= static_cast<uint8_t>(PageFlags::eREFERENCED);
        else flags &= ~static_cast<uint8_t>(PageFlags::eREFERENCED);
    }

    void set_valid(bool value)
    {
        if (value) flags |= static_cast<uint8_t>(PageFlags::eVALID);
        else flags &= ~static_cast<uint8_t>(PageFlags::eVALID);
    }
};

struct PageTable
{
    std::vector<PageTableEntry> entries;

    explicit PageTable(const size_t num_pages) : entries(num_pages) {}

    PageTableEntry& operator[](const uint32_t page_number) {
        return entries.at(page_number);
    }
};

struct Frame
{
    uint32_t pid = 0;
    uint32_t page_number = 0;
    uint32_t allocation_order = 0;
    bool is_free = true;
};

class BackingStore
{
    uint32_t page_size;
    std::string page_file_path;
    std::fstream page_file;
    std::vector<bool> allocated_slots;
    std::mutex store_mutex;

public:
    explicit BackingStore(const std::string &file_path, size_t max_pages = 4294967296, uint32_t page_size = 4096) :
        page_size(page_size), page_file_path(file_path), allocated_slots(max_pages)
    {
        page_file.open(page_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!page_file.is_open()) {
            page_file.open(page_file_path, std::ios::out | std::ios::binary);
            page_file.close();
            page_file.open(page_file_path, std::ios::in | std::ios::out | std::ios::binary);
        }

        page_file.seekp(max_pages * page_size - 1);
        page_file.write("", 1);
        page_file.flush();
    }

    ~BackingStore()
    {
        if (page_file.is_open()) {
            page_file.close();
        }
    }

    std::optional<uint32_t> allocate_slot();
    void free_slot(uint32_t slot);
    bool write_page(uint32_t slot, const uint8_t* page_data);
    bool read_page(uint32_t slot, uint8_t* page_data);
};

struct ProcessMemorySpace
{
    uint32_t process_id;
    PageTable page_table;
    std::unordered_map<uint32_t, uint32_t> page_to_backing_slot;
    size_t allocated_pages = 0;
    size_t max_pages;

    ProcessMemorySpace(const uint32_t pid, const size_t max_pages) : process_id(pid), page_table(max_pages), max_pages(max_pages) {}
};

class Memory {
    // Physical memory representation
    size_t max_overall_memory;
    std::atomic<size_t> total_allocated_memory{0};
    std::vector<uint8_t> memory;
    std::vector<Frame> frames;
    std::queue<uint32_t> free_frames;

    // Virtual memory
    uint32_t page_size;
    std::unordered_map<uint32_t, std::unique_ptr<ProcessMemorySpace>> process_spaces;
    std::unique_ptr<BackingStore> backing_store;
    std::queue<uint32_t> replacement_queue;
    std::atomic<uint32_t> allocation_counter{0};

    mutable std::recursive_mutex memory_mutex;

    // Stats
    std::atomic<uint64_t> page_faults{0};
    std::atomic<uint64_t> page_swaps{0};
    std::atomic<uint64_t> pages_paged_in{0};
    std::atomic<uint64_t> pages_paged_out{0};

    uint32_t get_page_number(uint32_t virtual_address) const
    {
        return virtual_address / page_size;
    }

    uint32_t get_page_offset(uint32_t virtual_address) const
    {
        return virtual_address % page_size;
    }

    uint32_t get_physical_address(uint32_t frame_num, uint32_t offset) const
    {
        return frame_num * page_size + offset;
    }

    uint32_t find_victim_frame();
    std::optional<uint32_t> allocate_frame();
    std::optional<uint32_t> evict_and_allocate();
    bool handle_page_fault(uint32_t process_id, uint32_t page_number);



public:
    explicit Memory(const size_t total_memory = 65536, const size_t frame_size = 4096, const size_t = 256) : memory(total_memory, 0), frames(total_memory / frame_size), page_size(frame_size), max_overall_memory(total_memory), backing_store(std::make_unique<BackingStore>("test_store.txt", 1024, frame_size))
    {
        for (size_t i = 0; i < frames.size(); i++) {
            free_frames.push(i);
            frames[i].is_free = true;
        }
    }

    bool create_process_space(uint32_t pid, size_t memory_bytes);
    void destroy_process_space(uint32_t pid);

    bool can_allocate_process(size_t required_memory_bytes) const;
    size_t get_available_memory() const;
    size_t get_total_allocated_memory() const;
    size_t calculate_pages_needed(size_t memory_bytes) const;

    size_t get_process_memory_usage(uint16_t pid) const;
    uint32_t get_page_size() const { return page_size; }

    uint64_t get_pages_paged_in() const { return pages_paged_in.load(); }
    uint64_t get_pages_paged_out() const { return pages_paged_out.load(); }

    [[nodiscard]] uint8_t read_byte(uint16_t address) const;
    [[nodiscard]] uint8_t read_byte(uint32_t pid, uint32_t virtual_address);

    void write_byte(uint16_t address, uint8_t value);
    void write_byte(uint32_t pid, uint32_t virtual_address, uint8_t value);

    [[nodiscard]] uint16_t read_word(uint16_t address) const;
    [[nodiscard]] uint16_t read_word(uint32_t pid, uint32_t virtual_address);

    void write_word(uint16_t address, uint16_t value);
    void write_word(uint32_t pid, uint16_t virtual_address, uint16_t value);

    void clear();
    [[nodiscard]] size_t size() const { return memory.size(); }

    [[nodiscard]] const uint8_t* data() const { return memory.data(); }

    // Fetch variable from memory or store if it is not yet stored.
    uint32_t get_var_address(uint32_t pid, std::unordered_map<std::string, size_t>& symbol_table, const std::string& var_name);

};



#endif //MEMORY_H

#ifndef PROCESS_H
#define PROCESS_H

#include <atomic>
#include <chrono>
#include <deque>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../memory/memory.h"
#include "instruction.h"

class IInstruction;
class Session;
class InstructionEncoder;

enum class ProcessState
{
    eReady,
    eRunning,
    eWaiting,
    eFinished
};

class Process
{
public:
    uint16_t id;
    std::string name;
    std::vector<std::shared_ptr<IInstruction>> instructions;
    std::vector<std::string> print_logs;
    std::atomic<int> current_instruction{0};
    std::atomic<ProcessState> current_state{ProcessState::eReady};
    std::atomic<uint16_t> assigned_core{9999};
    std::deque<std::string> output_buffer;
    std::atomic<uint64_t> sleep_until_tick{0};

    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;

    std::shared_ptr<Memory> memory;
    std::shared_ptr<Session> session;
    std::unordered_map<std::string, size_t> symbol_table;

    std::ofstream log_file;
    mutable std::mutex log_mutex;

    Process(const uint16_t id, const std::string &name, const std::shared_ptr<Memory> &memory);
    ~Process();

    // For running arbitrary programs
    template <typename F>
    void run(F&& program)
    {
        std::forward<F>(program)();
    }

    void execute(uint16_t core_id, uint32_t quantum = 0, uint32_t delay = 0);
    void add_instruction(std::shared_ptr<IInstruction> instruction);
    // For Week 6 homework
    void generate_print_instructions();
    void unroll_instructions();
    void save_smi_to_file();

    ProcessState get_state() const { return current_state.load(); }
    void set_state(const ProcessState state) { current_state.store(state); }

    uint16_t get_assigned_core() const { return assigned_core.load(); }
    void set_assigned_core(const uint16_t core_id) { assigned_core.store(core_id); }

    std::string get_status_string() const;

    std::string get_smi_string() const;

    uint32_t get_var_address(const std::string &var_name);

    std::optional<uint8_t> read_memory_byte(uint32_t virtual_address) const;
    bool write_memory_byte(uint32_t virtual_address, uint8_t value) const;

    std::optional<uint16_t> read_memory_word(uint32_t virtual_address) const;
    bool write_memory_word(uint32_t virtual_address, uint16_t value) const;

    void load_instructions_to_memory();

    void execute_from_memory(uint16_t core_id, uint32_t quantum = 0, uint32_t delay = 0);

    std::shared_ptr<IInstruction> fetch_instruction();

    uint32_t get_code_segment_base() const { return code_segment_base; }

    uint32_t get_program_counter() const { return program_counter.load(); }
    void set_program_counter(uint32_t pc) { program_counter.store(pc); }
    void increment_program_counter();

    void free_process_memory();

private:
    std::weak_ptr<Process> parent;
    std::vector<std::shared_ptr<Process>> children;
    uint32_t code_segment_base = 0x000;
    uint32_t str_table_base = 0x100;
    std::unique_ptr<InstructionEncoder> encoder;
    std::atomic<uint32_t> program_counter{0};

    void unroll_recursive(const std::vector<std::shared_ptr<IInstruction>>& to_expand, std::vector<std::shared_ptr<IInstruction>>& target_list);
};

#endif //PROCESS_H

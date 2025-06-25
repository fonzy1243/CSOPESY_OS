#ifndef PROCESS_H
#define PROCESS_H

#include <fstream>
#include <string>
#include <format>
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <atomic>
#include <thread>
#include <map>
#include <deque>
#include <unordered_map>
#include "instruction.h"
#include "../memory/memory.h"

class IInstruction;
class Session;

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

    Process(const uint16_t id, const std::string &name, const std::shared_ptr<Memory> &memory);
    ~Process();

    // For running arbitrary programs
    template <typename F>
    void run(F&& program)
    {
        std::forward<F>(program)();
    }

    void execute(uint16_t core_id, int max_instructions = -1);
    void add_instruction(std::shared_ptr<IInstruction> instruction);
    // For Week 6 homework
    void generate_print_instructions();
    void unroll_instructions();

    ProcessState get_state() const { return current_state.load(); }
    void set_state(const ProcessState state) { current_state.store(state); }

    uint16_t get_assigned_core() const { return assigned_core.load(); }
    void set_assigned_core(const uint16_t core_id) { assigned_core.store(core_id); }

    std::string get_status_string() const;

private:
    std::weak_ptr<Process> parent;
    std::vector<std::shared_ptr<Process>> children;

    void unroll_recursive(const std::vector<std::shared_ptr<IInstruction>>& to_expand, std::vector<std::shared_ptr<IInstruction>>& target_list);
};

#endif //PROCESS_H

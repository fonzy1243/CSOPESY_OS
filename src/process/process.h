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
#include "instruction.h"

class Instruction;

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
    std::vector<std::shared_ptr<Instruction>> instructions;
    std::atomic<int> current_instruction{0};
    std::atomic<ProcessState> current_state{ProcessState::eReady};
    std::atomic<uint16_t> assigned_core{9999};

    // storing variables in memory
    std::map<std::string, size_t> variable_indices;  // store indices here that points to the memory array
    std::vector<uint16_t> memory;  // memory array
    size_t next_free_index = 0;  // we use this to track next possible index in memory

    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;

    std::ofstream log_file;

    Process(const uint16_t id, const std::string &name);
    ~Process();

    // For running arbitrary programs
    template <typename F>
    void run(F&& program)
    {
        std::forward<F>(program)();
    }

    void execute(uint16_t core_id);
    void add_instruction(std::shared_ptr<Instruction> instruction);

    // function to try out the declare, add, subtract
    void generate_instructions();
    // For Week 6 homework
    void generate_print_instructions();

    ProcessState get_state() const { return current_state.load(); }
    void set_state(const ProcessState state) { current_state.store(state); }

    uint16_t get_assigned_core() const { return assigned_core.load(); }
    void set_assigned_core(const uint16_t core_id) { assigned_core.store(core_id); }

    std::string get_status_string() const;

private:
    std::weak_ptr<Process> parent;
    std::vector<std::shared_ptr<Process>> children;
};

#endif //PROCESS_H

//
// Created by Alfon on 6/24/2025.
//

#ifndef APHELIOS_H
#define APHELIOS_H

#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include "shell/shell.h"
#include "scheduler/scheduler.h"
#include "memory/memory.h"
#include "session/session.h"
#include "config/config_reader.h"

class Shell;

class ApheliOS {
public:
    std::unique_ptr<Scheduler> scheduler;
    std::shared_ptr<Memory> memory;

    std::atomic<bool> running;
    std::thread cpu_clock_thread;
  
    std::vector<std::shared_ptr<Session>> sessions;
    std::shared_ptr<Session> current_session;

    std::unique_ptr<Shell> shell;

    bool quit{false};

    ApheliOS();
    ~ApheliOS();

    void run();

    void process_command(const std::string& input);
    bool is_initialized() const { return initialized; }
    bool initialize(const std::string& config_file = "config.txt");
private:
    std::optional<CPUConfig> config;
    bool initialized{false};
    uint16_t current_pid{0};

    void run_system_clock();
    
    // Process generation
    std::atomic<bool> scheduler_generating_processes{false};
    std::thread process_generation_thread;

    void create_screen(const std::string& name, const size_t memory_size);
    void create_screen_with_instructions(const std::string& name, size_t memory_size, const std::string& raw_code);
    void switch_screen(const std::string& name);
    void exit_screen();
    void create_session(const std::string& session_name, bool has_leader, std::shared_ptr<Process> process);
    void switch_session(const std::string& session_name);

    void handle_screen_cmd(const std::string& input);
    void display_smi();
    void display_process_smi();

    
    void start_process_generation();
    void stop_process_generation();
    void process_generation_worker();
    
    // Helper function for generating nested for instructions
    std::shared_ptr<class ForInstruction> generate_random_for_instruction(
        const std::string& process_name, 
        int instruction_index, 
        std::vector<std::string>& declared_vars,
        std::mt19937& gen,
        std::uniform_int_distribution<>& value_dis,
        std::uniform_int_distribution<>& sleep_dis,
        int depth,
        int& instruction_count,
        int max_instructions,
        int cumulative_multiplier
    );
};



#endif //APHELIOS_H

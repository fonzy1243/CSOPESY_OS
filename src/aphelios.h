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
    uint16_t current_sid{0};
    
    // Process generation
    std::atomic<bool> scheduler_generating_processes{false};
    std::thread process_generation_thread;
    uint16_t process_counter{1};

    void create_screen(const std::string& name);
    void switch_screen(const std::string& name);
    void exit_screen();
    void create_session(const std::string& session_name, bool has_leader, std::shared_ptr<Process> process);
    void switch_session(const std::string& session_name);

    void handle_screen_cmd(const std::string& input);
    void display_smi();
    
    void start_process_generation();
    void stop_process_generation();
    void process_generation_worker();
};



#endif //APHELIOS_H

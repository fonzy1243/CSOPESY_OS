#include "aphelios.h"

#include <cassert>
#include <chrono>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <random>
#include <ranges>

#include "cpu_tick.h"
#include "process/instruction.h"


 ApheliOS::ApheliOS()
 {
     memory = std::make_shared<Memory>();
     shell = std::make_unique<Shell>(*this);
     create_session("pts", true, shell->shell_process);
     ShellUtils::print_header(shell->output_buffer);
     current_session->output_buffer = shell->output_buffer;
 }

ApheliOS::~ApheliOS()
{
     running.store(false);

     stop_process_generation();
     scheduler->stop();

     if (cpu_clock_thread.joinable()) {
         cpu_clock_thread.join();
     }
}

void ApheliOS::run_system_clock()
 {
     while (running.load()) {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
         increment_cpu_tick();
     }
 }


void ApheliOS::run()
 {
     shell->run(true);
 }

void ApheliOS::process_command(const std::string &input_raw)
 {
     std::string command;

     if (const size_t start = input_raw.find_first_not_of(" \t\n\r\f\v"); start == std::string::npos) {
         command = "";
     } else {
         const size_t end = input_raw.find_last_not_of(" \t\n\r\f\v");
         command = input_raw.substr(start, end - start + 1);
     }

     std::string command_lower = command | std::ranges::views::transform([] (unsigned char c) {
         return static_cast<char>(std::tolower(c)); }) | std::ranges::to<std::string>();

     bool is_initial_shell = !current_session || current_session->name == "pts";

     auto in_main = [&](const std::string &cmd) {
         if (!is_initial_shell) {
             shell->output_buffer.emplace_back(std::format("Error: '{}' can only be run in the main session.", cmd));
             return false;
         }
         return true;
     };

     if (command_lower == "initialize") {
         if (initialized) {
             shell->output_buffer.emplace_back("ApheliOS already initialized.");
         } else {
             initialize();
         }
         return;
     }

     if (!is_initialized() && command_lower != "initialize" && command_lower != "exit") {
         shell->output_buffer.emplace_back("Error: ApheliOS is not initialized.");
         return;
     }

     if (command_lower == "exit") {
         if (is_initial_shell) {
             this->quit = true;
         } else {
             shell->output_buffer.emplace_back("[screen is terminating]");
             exit_screen();
             shell->exit_to_main_menu = true;
         }
     } else if (command_lower == "clear") {
         ShellUtils::clear_screen();
         shell->output_buffer.clear();
     } else if (command_lower.rfind("screen", 0) == 0) {
         if (!in_main("screen")) return;
         handle_screen_cmd(command);
     } else if (command_lower == "marquee") {
         ShellUtils::toggle_marquee_mode(*shell);
     } else if (command_lower == "scheduler-start") {
         if (!in_main("scheduler-start")) return;
         if (scheduler_generating_processes) {
             shell->output_buffer.emplace_back("Scheduler is already generating processes.");
         } else {
             start_process_generation();
             shell->output_buffer.emplace_back("Scheduler started generating processes.");
         }
     } else if (command_lower == "scheduler-stop") {
         if (!in_main("scheduler-stop")) return;
         if (!scheduler_generating_processes) {
             shell->output_buffer.emplace_back("Scheduler is not generating processes.");
         } else {
             stop_process_generation();
             shell->output_buffer.emplace_back("Scheduler stopped generating processes.");
         }
     } else if (command_lower == "report-util") {
         if (!in_main("report_util")) return;
         scheduler->write_utilization_report();
         shell->output_buffer.emplace_back("Utilization report saved to logs/csopesy-log.txt");
     } else if (command_lower == "smi") {
        display_smi();
     } else if (command_lower == "process-smi") {
         const std::string smi_output = current_session->process->get_smi_string();
         shell->add_multiline_output(current_session->process->get_smi_string());
         current_session->process->save_smi_to_file();
     } else if (!command.empty()) {
         shell->output_buffer.push_back(std::format("{}: command not found", command));
     }
 }

bool ApheliOS::initialize(const std::string& config_file)
 {
     ConfigReader reader;

     auto load_result = reader.load_file(config_file);
     if (!load_result) {
         shell->output_buffer.emplace_back(std::format("Failed to load config file '{}': {}", config_file, load_result.error()));
         return false;
     }

     auto config_result = reader.parse_config();
     if (!config_result) {
         shell->output_buffer.emplace_back(std::format("Error: Invalid configuration - {}", config_result.error()));
         return false;
     }

     config = *config_result;

     memory = std::make_shared<Memory>(config->max_overall_mem, config->mem_per_frame, config->max_overall_mem / config->mem_per_frame);
     shell->shell_process->memory = memory;

     if (current_session && current_session->process) {
         current_session->process->memory = memory;
     }

     scheduler = std::make_unique<Scheduler>(config->num_cpu);

     running.store(true);

     cpu_clock_thread = std::thread(&ApheliOS::run_system_clock, this);
    
    // Configure scheduler type
    if (config->scheduler == "fcfs") {
        scheduler->set_scheduler_type(SchedulerType::FCFS);
        scheduler->set_delay(0);
    } else if (config->scheduler == "rr") {
        scheduler->set_scheduler_type(SchedulerType::RR);
        scheduler->set_delay(config->delays_per_exec);
    }
    
    // Set quantum cycles for round robin
    scheduler->set_quantum_cycles(config->quantum_cycles);

    scheduler->start();

    initialized = true;

     shell->output_buffer.emplace_back("Initialize command done.");
     shell->output_buffer.emplace_back(std::format("Configuration loaded successfully:"));
     shell->output_buffer.emplace_back(std::format("  CPUs: {}", config->num_cpu));
     shell->output_buffer.emplace_back(std::format("  Scheduler: {}", config->scheduler));
     shell->output_buffer.emplace_back(std::format("  Quantum: {}", config->quantum_cycles));
     shell->output_buffer.emplace_back(std::format("  Batch Process Freq: {}", config->batch_process_freq));
     shell->output_buffer.emplace_back(std::format("  Min/Max Instructions: {}/{}", config->min_ins, config->max_ins));

     return true;
 }

void ApheliOS::handle_screen_cmd(const std::string &input)
 {
     auto words = input | std::views::split(' ')
     | std::views::filter([](auto&& str) { return !std::ranges::empty(str); })
     | std::views::drop(1)
     | std::views::transform([](auto&& str) { return std::string_view(str.begin(), str.end()); });

     auto args = std::vector(words.begin(), words.end());
     if (args.empty()) return;

     if ((args[0] == "-S" || args[0] == "-s") && args.size() > 2) {
         const std::string& process_name = std::string(args[1]);
         size_t memory_size = 0;
         try {
             memory_size = std::stoul(std::string(args[2]));
         } catch (...) {
             shell->output_buffer.emplace_back("Error: invalid memory allocation");
             return;
         }

         // Check that memory_size is within [2^64, 2^216] and is power of 2
        if (memory_size < 64 || memory_size > 65536 || (memory_size & (memory_size - 1)) != 0) {
             shell->output_buffer.emplace_back("Error: invalid memory allocation");
             return;
         }
         create_screen(process_name, memory_size);

     } else if (args[0] == "-r" && args.size() > 1) {
         switch_screen(std::string(args[1]));
     } else if (args[0] == "-ls") {
         std::string status = scheduler->get_status_string();
         shell->add_multiline_output(status);
     } else {
         shell->output_buffer.emplace_back("Error: Wrong usage of the screen command");
     }
 }


void ApheliOS::create_screen(const std::string &name, const size_t memory_size)
 {
     if (current_session) {
         current_session->output_buffer = shell->output_buffer;
     }

     size_t process_memory = memory_size;

     // Check if there is enough memory to allocate this process
     if (!memory->can_allocate_process(process_memory)) {
         shell->output_buffer.emplace_back(std::format(
             "Error: Not enough memory to create process '{}'. Available: {} bytes, Required: {} bytes",
             name, memory->get_available_memory(), process_memory
         ));
         return;
     }

     auto new_process = std::make_shared<Process>(current_pid++, name, memory);

     // Try to create the process's memory space
     if (!memory->create_process_space(new_process->id, process_memory)) {
         shell->output_buffer.emplace_back(std::format(
             "Error: Failed to allocate memory for process '{}'", name
         ));
         return;
     }

     create_session(name, false, new_process);
     scheduler->add_process(new_process);

     shell->output_buffer.clear();

     shell->output_buffer.push_back(std::format("Process name: {}", current_session->process->name));
     shell->output_buffer.push_back(std::format("Current time: {:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime));
     shell->output_buffer.push_back(std::format("Memory allocated: {} bytes ({} pages)",
       process_memory, memory->calculate_pages_needed(process_memory)));


     current_session->output_buffer = shell->output_buffer;
 }

void ApheliOS::switch_screen(const std::string &name)
 {
     if (current_session) {
         current_session->output_buffer = shell->output_buffer;
     }

     switch_session(name);
     shell->output_buffer = current_session->output_buffer;

     if (shell->output_buffer.empty()) {
         shell->output_buffer.push_back(std::format("Process name: {}", current_session->process->name));
         shell->output_buffer.push_back(std::format("Current time: {:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime));
         current_session->output_buffer = shell->output_buffer;
     }
 }

void ApheliOS::exit_screen()
 {
     if (current_session && current_session->name != "pts") {
         current_session->output_buffer = shell->output_buffer;
     }
     switch_session("pts");
     shell->output_buffer = current_session->output_buffer;
 }

void ApheliOS::create_session(const std::string &session_name, bool has_leader, std::shared_ptr<Process> process)
 {
     auto new_session = std::make_shared<Session>();
     new_session->id = process->id;
     new_session->name = session_name;

     auto now = std::chrono::system_clock::now();
     auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
     new_session->createTime = std::chrono::zoned_time{std::chrono::current_zone(), now_seconds};

     new_session->process = process;
     process->session = new_session;


     sessions.push_back(new_session);
     current_session = new_session;
 }

void ApheliOS::switch_session(const std::string &session_name)
 {
     for (auto& session : sessions) {
         if (session->name == session_name) {
             current_session = session;
             break;
         }
     }
 }

void ApheliOS::display_smi()
 {
     ShellUtils::display_smi(*shell);
 }

void ApheliOS::start_process_generation()
{
    if (!scheduler_generating_processes.load()) {
        scheduler_generating_processes.store(true);
        process_generation_thread = std::thread(&ApheliOS::process_generation_worker, this);
    }
}

void ApheliOS::stop_process_generation()
{
    if (scheduler_generating_processes.load()) {
        scheduler_generating_processes.store(false);
        if (process_generation_thread.joinable()) {
            process_generation_thread.join();
        }
    }
}

// Helper function to generate random for instruction with nested support
std::shared_ptr<ForInstruction> ApheliOS::generate_random_for_instruction(
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
) {
    const int max_depth = 3;
    std::vector<std::shared_ptr<IInstruction>> sub_instructions;
    
    // Fixed iteration count: always 5 iterations regardless of depth
    uint16_t repeats = 5;
    int new_cumulative_multiplier = cumulative_multiplier * repeats;


    // Add 1-3 random sub-instructions to the for loop body, but respect max instruction limit
    std::uniform_int_distribution<> sub_count_dis(1, 3);
    int sub_count = sub_count_dis(gen);
    
    // Instruction type distribution (include for loops if we haven't reached max depth)
    std::uniform_int_distribution<> instruction_type_dis(0, depth < max_depth ? 5 : 4);
    
    for (int j = 0; j < sub_count && instruction_count < max_instructions; ++j) {
        if (instruction_count + new_cumulative_multiplier > max_instructions) {
            break;
        }

        int sub_type = instruction_type_dis(gen);
        
        switch (sub_type) {
            case 0: { // PrintInstruction
                auto print_instruction = std::make_shared<PrintInstruction>(
                    std::format("Loop iteration from {} depth {} instruction {}!", process_name, depth, j)
                );
                sub_instructions.push_back(print_instruction);
                instruction_count += new_cumulative_multiplier; // Count this instruction * repeats
                break;
            }
            case 1: { // DeclareInstruction
                std::string var_name = std::format("loop_var{}_{}_{}_d{}", process_name, instruction_index, j, depth);
                uint16_t value = value_dis(gen);
                auto declare_instruction = std::make_shared<DeclareInstruction>(var_name, value);
                sub_instructions.push_back(declare_instruction);
                declared_vars.push_back(var_name);
                instruction_count += new_cumulative_multiplier; // Count this instruction * repeats
                break;
            }
            case 2: { // AddInstruction
                std::string result_var = std::format("loop_result{}_{}_{}_d{}", process_name, instruction_index, j, depth);
                uint16_t val1 = value_dis(gen);
                uint16_t val2 = value_dis(gen);
                auto add_instruction = std::make_shared<AddInstruction>(result_var, val1, val2);
                sub_instructions.push_back(add_instruction);
                declared_vars.push_back(result_var);
                instruction_count += new_cumulative_multiplier; // Count this instruction * repeats
                break;
            }
            case 3: { // SubtractInstruction
                std::string result_var = std::format("loop_diff{}_{}_{}_d{}", process_name, instruction_index, j, depth);
                uint16_t val1 = value_dis(gen);
                uint16_t val2 = value_dis(gen);
                auto subtract_instruction = std::make_shared<SubtractInstruction>(result_var, val1, val2);
                sub_instructions.push_back(subtract_instruction);
                declared_vars.push_back(result_var);
                instruction_count += new_cumulative_multiplier; // Count this instruction * repeats
                break;
            }
            case 4: { // SleepInstruction
                uint8_t sleep_time = sleep_dis(gen);
                auto sleep_instruction = std::make_shared<SleepInstruction>(sleep_time);
                sub_instructions.push_back(sleep_instruction);
                instruction_count += new_cumulative_multiplier; // Count this instruction * repeats
                break;
            }
            case 5: { // Nested ForInstruction (only if we haven't reached max depth)
                if (depth < max_depth && instruction_count < max_instructions) {
                    auto nested_for = generate_random_for_instruction(
                        process_name, instruction_index, declared_vars, gen, value_dis, sleep_dis, depth + 1, instruction_count, max_instructions, new_cumulative_multiplier
                    );
                    sub_instructions.push_back(nested_for);
                    // Note: ForInstruction itself doesn't count, but its contents do
                }
                break;
            }
        }
    }
    
    return std::make_shared<ForInstruction>(sub_instructions, repeats);
}

void ApheliOS::process_generation_worker()
{
    if (!config.has_value()) return;

    const int batch_frequency = config->batch_process_freq;
    const int min_instructions = config->min_ins;
    const int max_instructions = config->max_ins;
    const int delays_per_exec = config->delays_per_exec;
    const size_t min_mem_per_proc = config->min_mem_per_proc;
    const size_t max_mem_per_proc = config->max_mem_per_proc;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> memory_dis(min_mem_per_proc, max_mem_per_proc);

    uint64_t last_gen_tick = 0;

    while (scheduler_generating_processes.load()) {
        uint64_t current_tick = get_cpu_tick();

        if (current_tick > last_gen_tick && (current_tick % batch_frequency) == 0) {
            // Generate random memory requirement for this process
            size_t process_memory = memory_dis(gen);

            // Check if we have enough memory before creating the process
            if (!memory->can_allocate_process(process_memory)) {
                // Skip this generation cycle - not enough memory
                last_gen_tick = current_tick;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Generate a new dummy process
            std::string process_name = std::format("p{:02d}", current_pid);

            auto new_process = std::make_shared<Process>(current_pid++, process_name, memory);

            // Create process space with the randomly allocated memory
            if (!memory->create_process_space(new_process->id, process_memory)) {
                // Failed to allocate memory, skip this process
                last_gen_tick = current_tick;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Create a session for this process (but don't make it current)
            auto new_session = std::make_shared<Session>();
            new_session->id = new_process->id;
            new_session->name = process_name;

            auto now = std::chrono::system_clock::now();
            auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
            new_session->createTime = std::chrono::zoned_time{std::chrono::current_zone(), now_seconds};

            new_session->process = new_process;
            new_process->session = new_session;

            // Add session to sessions list (but don't change current_session)
            sessions.push_back(new_session);

            // Generate random number of instructions within min/max range
            std::uniform_int_distribution<> dis(min_instructions, max_instructions);
            int target_instructions = dis(gen);

            // Random instruction generation setup
            std::uniform_int_distribution<> instruction_type_dis(0, 5);
            std::uniform_int_distribution<> value_dis(1, 100);
            std::uniform_int_distribution<> sleep_dis(1, 10);

            // Keep track of declared variables for use in operations and instruction count
            std::vector<std::string> declared_vars;
            int instruction_count = 0;

            // Add instructions to the process using random selection, respecting max instruction limit
            for (int i = 0; instruction_count < target_instructions; ++i) {
                int instruction_type = instruction_type_dis(gen);

                switch (instruction_type) {
                    case 0: { // PrintInstruction
                        auto print_instruction = std::make_shared<PrintInstruction>(
                            std::format("Hello world from {} instruction {} (mem: {} bytes)!", process_name, i, process_memory)
                        );
                        new_process->add_instruction(print_instruction);
                        instruction_count++;
                        break;
                    }
                    case 1: { // DeclareInstruction
                        std::string var_name = std::format("var{}_{}", process_name, declared_vars.size());
                        uint16_t value = value_dis(gen);
                        auto declare_instruction = std::make_shared<DeclareInstruction>(var_name, value);
                        new_process->add_instruction(declare_instruction);
                        declared_vars.push_back(var_name);
                        instruction_count++;
                        break;
                    }
                    case 2: { // AddInstruction
                        if (declared_vars.size() >= 2) {
                            std::uniform_int_distribution<> var_dis(0, declared_vars.size() - 1);
                            std::string result_var = std::format("result{}_{}", process_name, i);
                            std::string var1 = declared_vars[var_dis(gen)];
                            std::string var2 = declared_vars[var_dis(gen)];
                            auto add_instruction = std::make_shared<AddInstruction>(result_var, var1, var2);
                            new_process->add_instruction(add_instruction);
                            declared_vars.push_back(result_var);
                        } else {
                            std::string result_var = std::format("result{}_{}", process_name, i);
                            uint16_t val1 = value_dis(gen);
                            uint16_t val2 = value_dis(gen);
                            auto add_instruction = std::make_shared<AddInstruction>(result_var, val1, val2);
                            new_process->add_instruction(add_instruction);
                            declared_vars.push_back(result_var);
                        }
                        instruction_count++;
                        break;
                    }
                    case 3: { // SubtractInstruction
                        if (declared_vars.size() >= 2) {
                            std::uniform_int_distribution<> var_dis(0, declared_vars.size() - 1);
                            std::string result_var = std::format("diff{}_{}", process_name, i);
                            std::string var1 = declared_vars[var_dis(gen)];
                            std::string var2 = declared_vars[var_dis(gen)];
                            auto subtract_instruction = std::make_shared<SubtractInstruction>(result_var, var1, var2);
                            new_process->add_instruction(subtract_instruction);
                            declared_vars.push_back(result_var);
                        } else {
                            std::string result_var = std::format("diff{}_{}", process_name, i);
                            uint16_t val1 = value_dis(gen);
                            uint16_t val2 = value_dis(gen);
                            auto subtract_instruction = std::make_shared<SubtractInstruction>(result_var, val1, val2);
                            new_process->add_instruction(subtract_instruction);
                            declared_vars.push_back(result_var);
                        }
                        instruction_count++;
                        break;
                    }
                    case 4: { // SleepInstruction
                        uint8_t sleep_time = sleep_dis(gen);
                        auto sleep_instruction = std::make_shared<SleepInstruction>(sleep_time);
                        new_process->add_instruction(sleep_instruction);
                        instruction_count++;
                        break;
                    }
                    case 5: { // ForInstruction
                        if (instruction_count < target_instructions) {
                            auto for_instruction = generate_random_for_instruction(
                                process_name, i, declared_vars, gen, value_dis, sleep_dis, 0, instruction_count, target_instructions, 1
                            );
                            new_process->add_instruction(for_instruction);
                        }
                        break;
                    }
                }
            }

            scheduler->add_process(new_process);
            last_gen_tick = current_tick;
        }

        // Sleep for a reasonable time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
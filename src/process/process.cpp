#include "process.h"

#include <filesystem>
#include <ranges>

Process::Process(const uint16_t id, const std::string &name) : id(id), name(name)
 {
    creation_time = std::chrono::system_clock::now();

    std::filesystem::create_directories("logs");

    std::string log_filename = std::format("logs/process_{}.txt", id);
    log_file.open(log_filename, std::ios::out | std::ios::trunc);

    if (log_file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        log_file << std::format("Process name: {}\n", this->name);
        log_file << std::format("Logs:\n");
    }
 }

 Process::~Process()
{
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Process::execute(uint16_t core_id)
{
    start_time = std::chrono::system_clock::now();

    for (const auto instruction : instructions) {
        instruction->execute(*this);

        // Delay after each instruction, just for demo of week 6, remove after
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    end_time = std::chrono::system_clock::now();
}

void Process::add_instruction(std::shared_ptr<Instruction> instruction)
{
    instructions.push_back(instruction);
}

void Process::generate_print_instructions()
{
    for (int i = 0; i < 50; ++i) {
        std::string message = std::format("Hello world from {}!", name);
        auto print_instruction = std::make_shared<Instruction>(InstructionType::ePrint, message);

        add_instruction(print_instruction);
    }
}

void Process::generate_instructions()
{
    // sample code for creating declare instructions
    auto declare1 = std::make_shared<Instruction>(InstructionType::eDeclare, "a", (uint16_t)100);
    auto declare2 = std::make_shared<Instruction>(InstructionType::eDeclare, "b", (uint16_t)50);
    auto declare3 = std::make_shared<Instruction>(InstructionType::eDeclare, "c", (uint16_t)400);

    add_instruction(declare1);
    add_instruction(declare2);
    add_instruction(declare3);

    // sample code for creating add instructions
    auto add_var_var = std::make_shared<Instruction>(InstructionType::eAdd, "c", "a", "b");
    auto add_val_val = std::make_shared<Instruction>(InstructionType::eAdd, "c", 2, 8);

    add_instruction(add_var_var);
    add_instruction(add_val_val);

    // sample code for creating subtract instructions

    // the var1 can be a variable that hasn't been declared yet
    auto sub_var_var = std::make_shared<Instruction>(InstructionType::eSubtract, "d", "a", "b");
    auto sub_val_val = std::make_shared<Instruction>(InstructionType::eSubtract, "e", 10, 8);

    add_instruction(sub_var_var);
    add_instruction(sub_val_val);
}


std::string Process::get_status_string() const
{
    std::string state_str;
    switch (get_state()) {
        case ProcessState::eReady: state_str = "Ready"; break;
        case ProcessState::eWaiting: state_str = "Waiting"; break;
        case ProcessState::eRunning: state_str = "Running"; break;
        case ProcessState::eFinished: state_str = "Finished"; break;
    }

    // formatting creation time
    auto time_t_val = std::chrono::system_clock::to_time_t(creation_time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d %H:%M:%S");
    std::string formatted_time = ss.str();

    if (state_str == "Finished") {
        return std::format("{:<12} ({})  {:<10} {:>3}/{:<3}", name, formatted_time, "Finished", current_instruction.load(), instructions.size());
    }
    if (state_str == "Running") {
        std::string core_info = std::format("Core: {}", assigned_core.load());
        return std::format("{:<12} ({})  {:<10} {:>3}/{:<3}", name, formatted_time, core_info, current_instruction.load(), instructions.size());
    }

    return "debug";
}

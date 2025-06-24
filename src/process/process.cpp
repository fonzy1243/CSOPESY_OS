#include "process.h"

#include <filesystem>
#include <ranges>

Process::Process(const uint16_t id, const std::string &name, const std::shared_ptr<Memory> &memory) : id(id), name(name), memory(memory)
 {
    creation_time = std::chrono::system_clock::now();

    std::filesystem::create_directories("logs");

    std::string log_filename = std::format("logs/process_{}.txt", id);
    log_file.open(log_filename, std::ios::out | std::ios::trunc);

    if (log_file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};

        localtime_s(&tm, &time_t);

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

void Process::execute(uint16_t core_id, int max_instructions)
{
    start_time = std::chrono::system_clock::now();
    int executed = 0;
    while (current_instruction < (int)instructions.size() && (max_instructions < 0 || executed < max_instructions)) {
        if (get_state() == ProcessState::eWaiting) break;
        instructions[current_instruction]->execute(*this);
        current_instruction++;
        executed++;
    }
    if (current_instruction >= (int)instructions.size()) {
        end_time = std::chrono::system_clock::now();
    }
}

void Process::add_instruction(std::shared_ptr<IInstruction> instruction)
{
    instructions.push_back(instruction);
}

void Process::generate_print_instructions()
{
    for (int i = 0; i < 100; ++i) {
        std::string message = std::format("Hello world from {}!", name);
    }
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

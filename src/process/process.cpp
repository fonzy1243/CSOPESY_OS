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
    }

    end_time = std::chrono::system_clock::now();
}

void Process::add_instruction(std::shared_ptr<Instruction> instruction)
{
    instructions.push_back(instruction);
}

void Process::generate_print_instructions()
{
    for (int i = 0; i < 100; ++i) {
        std::string message = std::format("Hello world from {}!", name);
        auto print_instruction = std::make_shared<Instruction>(InstructionType::ePrint, message);
        add_instruction(print_instruction);
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

    if (state_str == "Finished") {
        return std::format("{}\t({})\tFinished\t{}/{}", name, creation_time, current_instruction.load(), instructions.size());
    }
    if (state_str == "Running") {
        return std::format("{}\t({})\tCore: {}\t{}/{}", name, creation_time, assigned_core.load(), current_instruction.load(), instructions.size());
    }

    return "debug";
}

#include "process.h"

#include <filesystem>
#include <ranges>

#include "../cpu_tick.h"

Process::Process(const uint16_t id, const std::string &name, const std::shared_ptr<Memory> &memory) : id(id), name(name), memory(memory)
 {
    creation_time = std::chrono::system_clock::now();

    std::filesystem::create_directories("logs");
 }

 Process::~Process(){}

void Process::execute(uint16_t core_id, std::atomic<bool>& is_running, uint32_t quantum, uint32_t delay)
{
    start_time = std::chrono::system_clock::now();
    int ticks_executed = 0;

    const bool run_indefinitely = quantum == 0;

    while (is_running.load() && current_instruction < (int)instructions.size() && (run_indefinitely || ticks_executed < quantum)) {
        if (get_state() == ProcessState::eWaiting) break;

        uint64_t last_tick = get_cpu_tick();
        uint64_t current_tick;
        while ((current_tick = get_cpu_tick()) == last_tick) {
            std::this_thread::yield();
        }

        ticks_executed++;

        if (ticks_executed % (delay + 1) == 0) {
            instructions[current_instruction]->execute(*this);
            ++current_instruction;
        }

        if (get_state() == ProcessState::eWaiting) break;
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
        add_instruction(std::make_shared<PrintInstruction>(message));
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

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

    localtime_s(&tm, &time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::string formatted_time = ss.str();

    if (state_str == "Finished") {
        return std::format("{:<12} ({})  {:<10} {:>3}/{:<3}",
                        name, formatted_time, "Finished",
                        current_instruction.load(), instructions.size());
    }
    if (state_str == "Running") {
        std::string core_info = std::format("Core: {}", assigned_core.load());
            return std::format("{:<12} ({})  {:<10} {:>3}/{:<3}",
                        name, formatted_time, core_info,
                        current_instruction.load(), instructions.size());
    }

    return "debug";
}

std::string Process::get_smi_string() const
{
    std::ostringstream out;

    out << std::format("Process name: {}\n", name);
    out << std::format("ID: {}\n", id);

    if (current_state == ProcessState::eFinished)
        out << "Status: Finished!\n";

    out << "Logs:\n";
    for (const auto &log: print_logs)
        out << "  " << log << "\n";

    out << std::format("Current instruction line: {}\n", current_instruction.load());
    out << std::format("Lines of code: {}\n", instructions.size());

    return out.str();
}

void Process::unroll_recursive(const std::vector<std::shared_ptr<IInstruction>> &to_expand,
                               std::vector<std::shared_ptr<IInstruction>> &target_list)
{
    for (const auto& instruction : to_expand) {
        if (auto for_inst = std::dynamic_pointer_cast<ForInstruction>(instruction)) {
            for (uint16_t i = 0; i < for_inst->get_repeats(); i++) {
                unroll_recursive(for_inst->get_sub_instructions(), target_list);
            }
        } else {
            target_list.push_back(instruction);
        }
    }
}

void Process::unroll_instructions()
{
    if (instructions.empty()) {
        return;
    }

    std::vector<std::shared_ptr<IInstruction>> expanded_list;
    expanded_list.reserve(instructions.size() * 2);

    unroll_recursive(instructions, expanded_list);

    instructions = std::move(expanded_list);

}

void Process::save_smi_to_file()
{
    std::filesystem::create_directories("logs");
    const std::string filename = std::format("logs/process_smi_{}.txt", name);
    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (out.is_open()) {
        out << get_smi_string();
        out.close();
    }
}

#include "process.h"
#include "instruction.h"

#include <filesystem>
#include <ranges>

#include "../cpu_tick.h"

Process::Process(const uint16_t id, const std::string &name, const std::shared_ptr<Memory> &memory) : id(id), name(name), memory(memory), encoder(std::make_unique<InstructionEncoder>())
 {
    creation_time = std::chrono::system_clock::now();
    std::filesystem::create_directories("logs");

    // memory->create_process_space(id, max_memory_pages);
 }

 Process::~Process()
{
    this->free_process_memory();
}

void Process::free_process_memory()
{
    if (memory) {
        memory->destroy_process_space(id);
    }
}

void Process::execute(uint16_t core_id, uint32_t quantum, uint32_t delay)
{
    start_time = std::chrono::system_clock::now();
    int ticks_executed = 0;

    const bool run_indefinitely = quantum == 0;

    while (current_instruction < (int)instructions.size() && (run_indefinitely || ticks_executed < quantum)) {
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
    str_table_base = (instructions.size() * sizeof(EncodedInstruction)) + 0x100;
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

    uint32_t current_inst = (program_counter.load() - code_segment_base) / sizeof(EncodedInstruction);

    if (state_str == "Finished") {
        return std::format("{:<12} ({})  {:<10} {:>3}/{:<3}",
                        name, formatted_time, "Finished",
                        current_inst, instructions.size());
    }
    if (state_str == "Running") {
        std::string core_info = std::format("Core: {}", assigned_core.load());
            return std::format("{:<12} ({})  {:<10} {:>3}/{:<3}",
                        name, formatted_time, core_info,
                        current_inst, instructions.size());
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

    uint32_t current_inst = (program_counter.load() - code_segment_base) / sizeof(EncodedInstruction);

    out << std::format("Current instruction line: {}\n", current_inst);
    out << std::format("Lines of code: {}\n", instructions.size());

    return out.str();
}

uint32_t Process::get_var_address(const std::string &var_name)
{
    return memory->get_var_address(id, symbol_table, var_name);
}

uint8_t Process::read_memory_byte(uint32_t virtual_address) const {
    return memory->read_byte(id, virtual_address);
}

void Process::write_memory_byte(uint32_t virtual_address, uint8_t value) const
{
    memory->write_byte(id, virtual_address, value);
}

uint16_t Process::read_memory_word(uint32_t virtual_address) const {
    return memory->read_word(id, virtual_address);
}

void Process::write_memory_word(uint32_t virtual_address, uint16_t value) const
{
    memory->write_word(id, virtual_address, value);
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

void Process::load_instructions_to_memory()
{
    encoder->store_str_table(*this, str_table_base);

    uint32_t current_addr = code_segment_base;

    for (const auto& inst : instructions) {
        EncodedInstruction encoded = encoder->encode_instruction(inst);

        write_memory_byte(current_addr + 0, encoded.opcode);
        write_memory_byte(current_addr + 1, encoded.flags);
        write_memory_word(current_addr + 2, encoded.operand1);
        write_memory_word(current_addr + 4, encoded.operand2);
        write_memory_word(current_addr + 6, encoded.operand3);

        current_addr += sizeof(EncodedInstruction);
    }


    program_counter.store(code_segment_base);
}

// This function may return null. Check for this in your code if you use it
std::shared_ptr<IInstruction> Process::fetch_instruction()
{
    uint32_t pc = program_counter.load();

    uint32_t max_addr = code_segment_base + (instructions.size() * sizeof(EncodedInstruction));
    if (pc >= max_addr) {
        return nullptr;
    }


    EncodedInstruction encoded;
    encoded.opcode = read_memory_byte(pc);
    encoded.flags = read_memory_byte(pc + 1);
    encoded.operand1 = read_memory_word(pc + 2);
    encoded.operand2 = read_memory_word(pc + 4);
    encoded.operand3 = read_memory_word(pc + 6);

    return encoder->decode_instruction(encoded);
}

void Process::increment_program_counter() { program_counter.fetch_add(sizeof(EncodedInstruction)); }


void Process::execute_from_memory(uint16_t core_id, uint32_t quantum, uint32_t delay)
{
    start_time = std::chrono::system_clock::now();
    uint32_t ticks_executed = 0;

    const bool run_indefinitely = quantum == 0;

    while (ticks_executed < quantum || run_indefinitely) {
        if (get_state() == ProcessState::eWaiting) break;

        uint64_t last_tick = get_cpu_tick();
        uint64_t current_tick;
        while ((current_tick = get_cpu_tick()) == last_tick) {
            std::this_thread::yield();
        }

        ticks_executed++;

        if (ticks_executed % (delay + 1) == 0) {
            auto instruction = fetch_instruction();
            if (!instruction) break;
            instruction->execute(*this);
            increment_program_counter();
        }

        if (get_state() == ProcessState::eWaiting) break;
    }

    uint32_t max_addr = code_segment_base + (instructions.size() * sizeof(EncodedInstruction));
    if (program_counter.load() >= max_addr) {
        end_time = std::chrono::system_clock::now();
    }
}

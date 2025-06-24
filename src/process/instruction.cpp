//
// Created by Alfon on 6/12/2025.
//

#include "instruction.h"
#include "../cpu_tick.h"

void PrintInstruction::execute(Process &process)
{

}

std::string PrintInstruction::get_type_name() const
{
    return "PRINT";
}

void DeclareInstruction::execute(Process &process)
{

}

std::string DeclareInstruction::get_type_name() const
{
    return "DECLARE";
}

void AddInstruction::execute(Process &process)
{

}

std::string AddInstruction::get_type_name() const
{
    return "ADD";
}

void SubtractInstruction::execute(Process &process)
{

}

std::string SubtractInstruction::get_type_name() const
{
    return "SUBTRACT";
}

void SleepInstruction::execute(Process &process) {
    uint64_t start_tick = get_cpu_tick();
    while (get_cpu_tick() - start_tick < x) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::string SleepInstruction::get_type_name() const
{
    return "SLEEP";
}

void ForInstruction::execute(Process &process) {  }

std::string ForInstruction::get_type_name() const
{
    return "FOR";
}
//
// Created by Alfon on 6/12/2025.
//

#include "instruction.h"
#include <fstream>


void PrintInstruction::execute(Process &process)
{
    if (process.log_file.is_open()) {
        process.log_file << message << "\n";
        process.log_file.flush(); 
    }

    process.output_buffer.push_back("[PRINT] " + message);
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
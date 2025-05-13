//
// Created by Alfon on 5/13/2025.
//

#include "session.h"

#include <print>

void Session::add_line(const std::string &line)
{
    output_buffer.push_back(line);
}

void Session::print_output()
{
    // for (const auto &line : output_buffer) {
    //     std::println(line.c_str());
    // }
}

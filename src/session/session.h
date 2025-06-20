#ifndef SESSION_H
#define SESSION_H

#include <chrono>
#include <format>
#include <memory>
#include <queue>
#include <vector>
#include "../process/process.h"

class Session
{
public:
    std::shared_ptr<Process> process;  // One process per session
    uint16_t id;
    std::string name;
    std::chrono::zoned_time<std::chrono::duration<long long>> createTime;
    std::deque<std::string> output_buffer;

    void add_line(const std::string &line);
    void print_output();
};

#endif //SESSION_H


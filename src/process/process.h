#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <memory>
#include <functional>

class Process
{
public:
    uint16_t id;
    std::string name;
    template <typename F>
    void run(F&& program)
    {
        std::forward<F>(program)();
    }

    Process(uint16_t id, std::string name) : id(id), name(name) {}
private:
    std::weak_ptr<Process> parent;
    std::vector<std::shared_ptr<Process>> children;
};

#endif //PROCESS_H

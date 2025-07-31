#include "aphelios.h"
#include "memory/memory.h"
#include <memory>

std::shared_ptr<Memory> global_memory_ptr = nullptr;

int main()
{
    ApheliOS apheliOs;
    global_memory_ptr = apheliOs.memory;
    apheliOs.run();
}

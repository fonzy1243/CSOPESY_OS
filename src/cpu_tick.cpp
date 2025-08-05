#include "cpu_tick.h"

std::atomic<uint64_t> cpu_tick{0};
std::atomic<uint64_t> active_cpu_ticks{0};
std::atomic<bool> any_core_active_this_tick{false};
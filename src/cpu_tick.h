#ifndef CPU_TICK_H
#define CPU_TICK_H
#include <atomic>
#include <cstdint>

extern std::atomic<uint64_t> cpu_tick;

inline uint64_t get_cpu_tick() { return cpu_tick.load(); }
inline void increment_cpu_tick() { cpu_tick.fetch_add(1); }

#endif // CPU_TICK_H 
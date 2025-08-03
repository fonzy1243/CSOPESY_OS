#ifndef CPU_TICK_H
#define CPU_TICK_H
#include <atomic>
#include <cstdint>

extern std::atomic<uint64_t> cpu_tick;
extern std::atomic<uint64_t> active_cpu_ticks;

inline uint64_t get_cpu_tick() { return cpu_tick.load(); }
inline void increment_cpu_tick() { cpu_tick.fetch_add(1); }
inline void increment_active_ticks() { active_cpu_ticks.fetch_add(1); }
inline uint64_t get_idle_ticks() { return cpu_tick.load() - active_cpu_ticks.load(); }
inline uint64_t get_active_ticks() { return active_cpu_ticks.load(); }


#endif // CPU_TICK_H 
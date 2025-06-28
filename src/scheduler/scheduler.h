#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <fstream>
#include <iostream>
#include <chrono>
#include <string>
#include <format>
#include "../process/process.h"

enum class SchedulerType { FCFS, RR };

class Scheduler {
private:
    std::queue<std::shared_ptr<Process>> ready_queue;
    std::queue<std::shared_ptr<Process>> waiting_queue;
    std::vector<std::shared_ptr<Process>> running_processes;
    std::vector<std::shared_ptr<Process>> finished_processes;

    // Per-core queues for better performance and reduced contention
    std::vector<std::queue<std::shared_ptr<Process>>> per_core_queues;
    std::vector<std::unique_ptr<std::mutex>> per_core_mutexes;

    std::mutex ready_mutex;
    std::mutex running_mutex;
    std::mutex waiting_mutex;
    std::mutex finished_mutex;
    std::mutex log_mutex;

    std::condition_variable scheduler_cv;

    std::atomic<bool> running{false};

    uint16_t num_cores;
    std::vector<std::thread> cpu_threads;
    std::thread scheduler_thread;

    uint32_t quantum_cycles = 1;
    uint32_t delay = 1;
    SchedulerType scheduler_type = SchedulerType::FCFS;

    void scheduler_loop();
    void cpu_worker(uint16_t core_id);

public:
    explicit Scheduler(uint16_t num_cores = 4);
    ~Scheduler();

    void start();
    void stop();

    void add_process(std::shared_ptr<Process> process);
    void write_utilization_report();

    std::vector<std::shared_ptr<Process>> get_finished();
    std::vector<std::shared_ptr<Process>> get_running();
    bool is_running() const { return running.load(); }

    std::string get_status_string();

    void set_delay(uint32_t delay) { this->delay = delay; }
    void set_quantum_cycles(uint32_t q) { quantum_cycles = q; }
    void set_scheduler_type(SchedulerType t) { scheduler_type = t; }
    uint32_t get_delay() const { return delay; }
    uint32_t get_quantum_cycles() const { return quantum_cycles; }
    SchedulerType get_scheduler_type() const { return scheduler_type; }
};

#endif //SCHEDULER_H

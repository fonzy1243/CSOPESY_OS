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

class Scheduler {
private:
    std::queue<std::shared_ptr<Process>> ready_queue;
    std::vector<std::shared_ptr<Process>> running_processes;
    std::vector<std::shared_ptr<Process>> finished_processes;

    std::mutex ready_mutex;
    std::mutex running_mutex;
    std::mutex finished_mutex;
    std::mutex log_mutex;

    std::condition_variable scheduler_cv;

    std::atomic<bool> running{false};

    uint16_t num_cores;
    std::vector<std::thread> cpu_threads;
    std::thread scheduler_thread;

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
};

#endif //SCHEDULER_H

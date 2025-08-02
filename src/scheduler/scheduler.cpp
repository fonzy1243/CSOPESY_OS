//
// Created by Alfon on 6/12/2025.
//
#include <fstream>
#include <format>
#include <filesystem>
#include "scheduler.h"
#include "../cpu_tick.h"
#include "../memory/memory.h" // Added for global_memory_ptr
#include <iomanip>
#include <ctime>
#include <sstream>

extern std::shared_ptr<Memory> global_memory_ptr; // Add this at the top if needed

 Scheduler::Scheduler(uint16_t num_cores) : num_cores(num_cores)
 {
     cpu_threads.reserve(num_cores);
     // Initialize per-core ready queues for better performance
     per_core_queues.resize(num_cores);
     per_core_mutexes.resize(num_cores);
     for (uint16_t i = 0; i < num_cores; ++i) {
         per_core_mutexes[i] = std::make_unique<std::mutex>();
     }
 }


Scheduler::~Scheduler()
 {
     stop();
 }

void Scheduler::start()
 {
     if (running.load()) return;

     running.store(true);

     for (int i = 0; i < num_cores; i++) {
         cpu_threads.emplace_back(&Scheduler::cpu_worker, this, i);
     }

     scheduler_thread = std::thread(&Scheduler::scheduler_loop, this);
 }

void Scheduler::stop()
 {
     if (!running.load()) return;

     running.store(false);
     scheduler_cv.notify_all();

     if (scheduler_thread.joinable()) {
         scheduler_thread.join();
     }

     for (auto& thread : cpu_threads) {
         if (thread.joinable()) {
             thread.join();
         }
     }

     cpu_threads.clear();
 }

void Scheduler::scheduler_loop()
 {
     uint16_t next_core = 0; // Round-robin assignment to cores
     
     while (running.load()) {
         // Check waiting processes and move them if they are done sleeping
         {
             std::lock_guard waiting_lock(waiting_mutex);
             std::queue<std::shared_ptr<Process>> still_waiting;
             while (!waiting_queue.empty()) {
                 auto process = waiting_queue.front();
                 waiting_queue.pop();

                 if (get_cpu_tick() >= process->sleep_until_tick.load()) {
                     process->set_state(ProcessState::eReady);
                     
                     // Directly assign to a specific core's queue for better performance
                     {
                         std::lock_guard core_lock(*per_core_mutexes[next_core]);
                         per_core_queues[next_core].push(process);
                     }
                     next_core = (next_core + 1) % num_cores;
                 } else {
                     still_waiting.push(process);
                 }
             }
             waiting_queue = std::move(still_waiting);
         }

         // Move processes from global ready queue to per-core queues
         {
             std::lock_guard ready_lock(ready_mutex);
             while (!ready_queue.empty()) {
                 auto process = ready_queue.front();
                 ready_queue.pop();
                 
                 // Assign to core in round-robin fashion
                 {
                     std::lock_guard core_lock(*per_core_mutexes[next_core]);
                     per_core_queues[next_core].push(process);
                 }
                 next_core = (next_core + 1) % num_cores;
             }
         }

         // Very short sleep to prevent busy waiting but maintain responsiveness
         std::this_thread::sleep_for(std::chrono::microseconds(10));
     }
 }

void Scheduler::add_process(std::shared_ptr<Process> process)
 {
     process->unroll_instructions();
     process->set_state(ProcessState::eReady);

     // Find the core with the least work for load balancing
     uint16_t best_core = 0;
     size_t min_queue_size = SIZE_MAX;
     
     for (uint16_t i = 0; i < num_cores; ++i) {
         std::lock_guard core_lock(*per_core_mutexes[i]);
         if (per_core_queues[i].size() < min_queue_size) {
             min_queue_size = per_core_queues[i].size();
             best_core = i;
         }
     }
     
     // Add process to the least loaded core's queue
     {
         std::lock_guard core_lock(*per_core_mutexes[best_core]);
         per_core_queues[best_core].push(process);
     }
 }


void Scheduler::cpu_worker(uint16_t core_id)
 {
     static std::atomic<uint64_t> global_quantum_counter{0};
     while (running.load()) {
         std::shared_ptr<Process> process_to_run = nullptr;

         // First, check this core's dedicated queue
         {
             std::lock_guard core_lock(*per_core_mutexes[core_id]);
             if (!per_core_queues[core_id].empty()) {
                 process_to_run = per_core_queues[core_id].front();
                 per_core_queues[core_id].pop();
                 process_to_run->set_assigned_core(core_id);
                 process_to_run->set_state(ProcessState::eRunning);
             }
         }

         // If no process in dedicated queue, try work stealing from other cores
         if (!process_to_run) {
             for (uint16_t i = 1; i < num_cores; ++i) {
                 uint16_t steal_from = (core_id + i) % num_cores;
                 std::lock_guard core_lock(*per_core_mutexes[steal_from]);
                 if (!per_core_queues[steal_from].empty()) {
                     process_to_run = per_core_queues[steal_from].front();
                     per_core_queues[steal_from].pop();
                     process_to_run->set_assigned_core(core_id);
                     process_to_run->set_state(ProcessState::eRunning);
                     break;
                 }
             }
         }

         if (process_to_run) {
             // Add to running processes for monitoring
             {
                 std::lock_guard running_lock(running_mutex);
                 running_processes.push_back(process_to_run);
             }

             uint32_t ticks_to_run = (scheduler_type == SchedulerType::FCFS) ? 0 : quantum_cycles;

             process_to_run->execute(core_id, ticks_to_run, delay);

             // Remove from running processes
             {
                 std::lock_guard lock(running_mutex);
                 std::erase(running_processes, process_to_run);
             }

             const bool finished = (process_to_run->current_instruction.load() >= (int)process_to_run->instructions.size());

             if (finished) {
                 process_to_run->set_state(ProcessState::eFinished);
                 std::lock_guard finished_lock(finished_mutex);
                 process_to_run->set_assigned_core(9999);
                 finished_processes.push_back(process_to_run);
             } else if (process_to_run->get_state() == ProcessState::eWaiting) {
                 // Still waiting (e.g., sleeping)
                 std::lock_guard waiting_lock(waiting_mutex);
                 process_to_run->set_assigned_core(9999);
                 waiting_queue.push(process_to_run);
             } else {
                 // Preempted due to quantum expiration, move back to ready queue
                 process_to_run->set_state(ProcessState::eReady);
                 // Add back to this core's queue for better cache locality
                 {
                     std::lock_guard core_lock(*per_core_mutexes[core_id]);
                     process_to_run->set_assigned_core(9999);
                     per_core_queues[core_id].push(process_to_run);
                 }
             }
             // Quantum cycle tracking and memory snapshot
             if (scheduler_type == SchedulerType::RR) {
                 uint64_t prev = global_quantum_counter.fetch_add(1) + 1;
                 if (prev % quantum_cycles == 0) {
                     output_memory_snapshot(prev / quantum_cycles);
                 }
             }
         } else {
             // No process to run, but don't sleep - just yield CPU briefly
             std::this_thread::yield();
         }
     }
 }

std::vector<std::shared_ptr<Process>> Scheduler::get_finished()
 {
     std::lock_guard lock(finished_mutex);
     return finished_processes;
 }

std::vector<std::shared_ptr<Process>> Scheduler::get_running()
 {
     std::lock_guard lock(running_mutex);
     return running_processes;
 }

std::string Scheduler::get_status_string()
 {
     auto running = get_running();
     auto finished = get_finished();

     struct ProcessSnapshot
     {
         std::string status_string;
         ProcessState state;
         int assigned_core;
     };

     std::vector<ProcessSnapshot> running_snapshots;
     std::vector<ProcessSnapshot> finished_snapshots;

     // Capture current state of running processes
     for (const auto& process : running) {
         running_snapshots.push_back({
             process->get_status_string(),
             process->get_state(),
             process->get_assigned_core()
         });
     }

     // Capture current state of finished processes
     for (const auto& process : finished) {
         finished_snapshots.push_back({
             process->get_status_string(),
             process->get_state(),
             process->get_assigned_core()
         });
     }

     std::string result = "-----------------------------\n";
     result += "CPU Utilization Report:\n";


     int cores_used = std::count_if(running_snapshots.begin(), running_snapshots.end(), [](const auto &snapshot) {
         return snapshot.state == ProcessState::eRunning && snapshot.assigned_core != 9999;
         });

     int cores_available = static_cast<int>(num_cores) - cores_used;
     float utilization = (num_cores > 0) ? (static_cast<float>(cores_used) / num_cores) * 100.0f : 0.0f;

     result += std::format("Cores used: {}\n", cores_used);
     result += std::format("Cores available: {}\n", cores_available);
     result += std::format("CPU utilization: {:.2f}%\n", utilization);
     result += "-----------------------------\n";

     result += "Running processes:\n";
         for (const auto& snapshot : running_snapshots) {
             if (snapshot.state == ProcessState::eRunning && snapshot.assigned_core != 9999) {
                 result += std::format("{}\n", snapshot.status_string);
             }
         }

     result += "\nFinished processes:\n";
         for (const auto& snapshot : finished_snapshots) {
             result += std::format("{}\n", snapshot.status_string);
         }
     result += "-----------------------------\n";

     return result;
 }

void Scheduler::write_utilization_report()
 {
     std::filesystem::create_directories("logs");
     std::string report = get_status_string();

     std::string log_filename = "logs/csopesy-log.txt";

     std::ofstream out(log_filename, std::ios::out | std::ios::trunc);
     if (out.is_open()) {
         out << report;
         out.close();
     }
 }


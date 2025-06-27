//
// Created by Alfon on 6/12/2025.
//
#include <fstream>
#include <format>
#include <filesystem>
#include "scheduler.h"
#include "../cpu_tick.h"

 Scheduler::Scheduler(uint16_t num_cores) : num_cores(num_cores)
 {
     cpu_threads.reserve(num_cores);
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
     while (running.load()) {
         // Check waiting processes and move them if they are done sleeping
         {
             std::lock_guard waiting_lock(waiting_mutex);
             std::unique_lock ready_lock(ready_mutex);
             std::queue<std::shared_ptr<Process>> still_waiting;
             while (!waiting_queue.empty()) {
                 auto process = waiting_queue.front();
                 waiting_queue.pop();

                 if (get_cpu_tick() >= process->sleep_until_tick.load()) {
                     process->set_state(ProcessState::eReady);
                     ready_queue.push(process);
                     scheduler_cv.notify_one();
                 } else {
                     still_waiting.push(process);
                 }
             }

             waiting_queue = std::move(still_waiting);
         }

         scheduler_cv.notify_all();

         std::unique_lock ready_lock(ready_mutex);

         if (scheduler_cv.wait_for(ready_lock, std::chrono::milliseconds(1), [this] {
             return !ready_queue.empty() || !running.load();
         })) {
             if (!running.load()) break;

             std::lock_guard running_lock(running_mutex);

             while (!ready_queue.empty() && static_cast<int>(running_processes.size()) < num_cores) {
                 auto process = ready_queue.front();
                 ready_queue.pop();

                 running_processes.push_back(process);
             }
         }

         ready_lock.unlock();
     }
 }

void Scheduler::add_process(std::shared_ptr<Process> process)
 {
     std::lock_guard lock(ready_mutex);

     process->unroll_instructions();

     ready_queue.push(process);
     process->set_state(ProcessState::eReady);
     scheduler_cv.notify_one();
 }


void Scheduler::cpu_worker(uint16_t core_id)
 {
     while (running.load()) {
         std::shared_ptr<Process> process_to_run = nullptr;

         {
             std::lock_guard lock(running_mutex);
             for (auto it = running_processes.begin(); it != running_processes.end(); ++it) {
                 if ((*it)->get_state() == ProcessState::eRunning && (*it)->get_assigned_core() == 9999) {
                     process_to_run = *it;
                     process_to_run->set_assigned_core(core_id);
                     process_to_run->set_state(ProcessState::eRunning);
                     break;
                 }
             }
         }

         if (process_to_run) {
             uint32_t ticks_to_run = (scheduler_type == SchedulerType::FCFS) ? 0 : quantum_cycles;

             process_to_run->execute(core_id, ticks_to_run, delay);

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
                 // Still waiting (e.g., sleeping), keep in running_processes
                 std::lock_guard waiting_lock(waiting_mutex);
                 process_to_run->set_assigned_core(9999);
                 waiting_queue.push(process_to_run);
             } else {
                 // Preempted due to quantum expiration, move back to ready queue
                 process_to_run->set_state(ProcessState::eReady);
                 std::lock_guard lock(ready_mutex);
                 process_to_run->set_assigned_core(9999);
                 ready_queue.push(process_to_run);
                 scheduler_cv.notify_one();
             }
         } else {
             // No process to run, sleep briefly to avoid busy waiting
             std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
     std::string result = "-----------------------------\n";
     result += "CPU Utilization Report:\n";

     int cores_used;
     {
         std::lock_guard lock(running_mutex);
         cores_used = static_cast<int>(running_processes.size());
     }

     int cores_available = static_cast<int>(num_cores) - cores_used;
     float utilization = (num_cores > 0) ? (static_cast<float>(cores_used) / num_cores) * 100.0f : 0.0f;

     result += std::format("Cores used: {}\n", cores_used);
     result += std::format("Cores available: {}\n", cores_available);
     result += std::format("CPU utilization: {:.2f}%\n", utilization);
     result += "-----------------------------\n";

     result += "Running processes:\n";
     auto running_processes = get_running();
     for (const auto& process : running_processes) {
         if (process->get_state() == ProcessState::eRunning)
             result += std::format("{}\n", process->get_status_string());
     }

     result += "\nFinished processes:\n";
     auto finished = get_finished();
     for (const auto& process : finished) {
         result += std::format("{}\n", process->get_status_string());
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


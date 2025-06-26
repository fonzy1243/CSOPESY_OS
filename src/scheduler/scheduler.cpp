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
         std::unique_lock ready_lock(ready_mutex);

         scheduler_cv.wait(ready_lock, [this] {
             return !ready_queue.empty() || !running.load();
         });

         if (!running.load()) break;

         std::lock_guard running_lock(running_mutex);

         while (!ready_queue.empty() && static_cast<int>(running_processes.size()) < num_cores) {
             auto process = ready_queue.front();
             ready_queue.pop();

             process->set_state(ProcessState::eRunning);
             running_processes.push_back(process);
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
                     break;
                 }
             }
         }

         if (process_to_run) {
             bool finished = false;
             bool preempted = false;
             
             if (scheduler_type == SchedulerType::FCFS) {
                 process_to_run->execute(core_id);
                 finished = true;
             } else if (scheduler_type == SchedulerType::RR) {
                 // Round Robin: Run for quantum_cycles instructions
                 int instructions_before = process_to_run->current_instruction.load();
                 
                 // Execute for the specified quantum
                 process_to_run->execute(core_id, quantum_cycles);
                 
                 int instructions_after = process_to_run->current_instruction.load();
                 
                 // Check if process finished
                 if (instructions_after >= (int)process_to_run->instructions.size()) {
                     finished = true;
                 } 
                 // Check if process is waiting (e.g., sleeping)
                 else if (process_to_run->get_state() == ProcessState::eWaiting) {
                     // Process is waiting, don't preempt
                     preempted = false;
                 }
                 // Check if quantum was used up (preemption needed)
                 else if (instructions_after - instructions_before >= quantum_cycles) {
                     preempted = true;
                 } else {
                     // Process finished before quantum expired
                     finished = true;
                 }
             }

             {
                 std::lock_guard running_lock(running_mutex);
                 std::lock_guard finished_lock(finished_mutex);

                 auto it = std::find(running_processes.begin(), running_processes.end(), process_to_run);

                 if (it != running_processes.end()) {
                     running_processes.erase(it);
                 }

                 if (finished) {
                     process_to_run->set_state(ProcessState::eFinished);
                     finished_processes.push_back(process_to_run);
                 } else if (process_to_run->get_state() == ProcessState::eWaiting) {
                     // Still waiting (e.g., sleeping), keep in running_processes
                     process_to_run->set_assigned_core(9999);
                     running_processes.push_back(process_to_run);
                 } else if (preempted) {
                     // Preempted due to quantum expiration, move back to ready queue
                     process_to_run->set_assigned_core(9999);
                     process_to_run->set_state(ProcessState::eReady);
                     {
                         std::lock_guard lock(ready_mutex);
                         ready_queue.push(process_to_run);
                         scheduler_cv.notify_one();
                     }
                 }
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


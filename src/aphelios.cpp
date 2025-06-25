//
// Created by Alfon on 6/24/2025.
//

#include "aphelios.h"

#include <ranges>
#include <cassert>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>


 ApheliOS::ApheliOS() : scheduler(4)
 {
     shell = std::make_unique<Shell>(*this);
     memory = std::make_shared<Memory>();
     create_session("pts", true, shell->shell_process);

     ShellUtils::print_header(shell->output_buffer);
     current_session->output_buffer = shell->output_buffer;
 }

void ApheliOS::run()
 {
     scheduler.start();
     shell->run(true);
 }

void ApheliOS::process_command(const std::string &input_raw)
 {
     std::string command;

     if (const size_t start = input_raw.find_first_not_of(" \t\n\r\f\v"); start == std::string::npos) {
         command = "";
     } else {
         const size_t end = input_raw.find_last_not_of(" \t\n\r\f\v");
         command = input_raw.substr(start, end - start + 1);
     }

     std::string command_lower = command | std::ranges::views::transform([] (unsigned char c) {
         return static_cast<char>(std::tolower(c)); }) | std::ranges::to<std::string>();

     bool is_initial_shell = !current_session || current_session->name == "pts";

     if (command_lower == "exit") {
         if (is_initial_shell) {
             this->quit = true;
         } else {
             shell->output_buffer.emplace_back("[screen is terminating]");
             exit_screen();
             shell->exit_to_main_menu = true;
         }
     } else if (command_lower == "clear") {
         ShellUtils::clear_screen();
         shell->output_buffer.clear();
     } else if (command_lower.rfind("screen", 0) == 0) {
         handle_screen_cmd(command);
     } else if (command_lower == "marquee") {
         ShellUtils::toggle_marquee_mode(*shell);
     } else if (command_lower == "report-util") {
         scheduler.write_utilization_report();
         shell->output_buffer.emplace_back("Utilization report saved to logs/csopesy-log.txt");
     } else if (command_lower == "smi") {
         display_smi();
     } else if (!command.empty()) {
         shell->output_buffer.push_back(std::format("{}: command not found", command));
     }
 }

void ApheliOS::handle_screen_cmd(const std::string &input)
 {
     auto words = input | std::views::split(' ')
     | std::views::filter([](auto&& str) { return !std::ranges::empty(str); })
     | std::views::drop(1)
     | std::views::transform([](auto&& str) { return std::string_view(str.begin(), str.end()); });

     auto args = std::vector(words.begin(), words.end());
     if (args.empty()) return;

     if (args[0] == "-S" && args.size() > 1) {
         create_screen(std::string(args[1]));
     } else if (args[0] == "-r" && args.size() > 1) {
         switch_screen(std::string(args[1]));
     } else if (args[0] == "-ls") {
         std::string status = scheduler.get_status_string();
         shell->output_buffer.emplace_back(status);
     }
 }


void ApheliOS::create_screen(const std::string &name)
 {
     if (current_session) {
         current_session->output_buffer = shell->output_buffer;
     }

     auto new_process = std::make_shared<Process>(current_pid++, name, this->memory);
     create_session(name, false, new_process);
     scheduler.add_process(new_process);


     shell->output_buffer.clear();

     shell->output_buffer.push_back(std::format("Process name: {}", current_session->process->name));
     shell->output_buffer.push_back(std::format("Current time: {:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime));

     current_session->output_buffer = shell->output_buffer;
 }

void ApheliOS::switch_screen(const std::string &name)
 {
     if (current_session) {
         current_session->output_buffer = shell->output_buffer;
     }

     switch_session(name);
     shell->output_buffer = current_session->output_buffer;

     if (shell->output_buffer.empty()) {
         shell->output_buffer.push_back(std::format("Process name: {}", current_session->process->name));
         shell->output_buffer.push_back(std::format("Current time: {:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime));
         current_session->output_buffer = shell->output_buffer;
     }
 }

void ApheliOS::exit_screen()
 {
     if (current_session && current_session->name != "pts") {
         current_session->output_buffer = shell->output_buffer;
     }
     switch_session("pts");
     shell->output_buffer = current_session->output_buffer;
 }

void ApheliOS::create_session(const std::string &session_name, bool has_leader, std::shared_ptr<Process> process)
 {
     auto new_session = std::make_shared<Session>();
     new_session->id = current_sid++;
     new_session->name = session_name;

     auto now = std::chrono::system_clock::now();
     auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
     new_session->createTime = std::chrono::zoned_time{std::chrono::current_zone(), now_seconds};

     new_session->process = process;
     process->session = new_session;


     sessions.push_back(new_session);
     current_session = new_session;
 }

void ApheliOS::switch_session(const std::string &session_name)
 {
     for (auto& session : sessions) {
         if (session->name == session_name) {
             current_session = session;
             break;
         }
     }
 }

void ApheliOS::display_smi()
 {
     ShellUtils::display_smi(*shell);
 }

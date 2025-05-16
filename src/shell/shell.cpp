#include "shell.h"

#include <iostream>
#include <format>
#include <print>
#include <ranges>
#include <string>
#include <locale>
#include <assert.h>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
void ShellUtils::clear_screen()
{
    HANDLE h_std_out = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h_std_out, &csbi);
    FillConsoleOutputCharacter(h_std_out, ' ', csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
    SetConsoleCursorPosition(h_std_out, coord);
}
#else
void ShellUtils::clear_screen() { std::cout << "\033[2J\033[1;1H"; }
#endif

std::string ShellUtils::convert_attributes_to_ansi(WORD attributes, WORD &current_screen_attributes)
{
    if (attributes == current_screen_attributes && current_screen_attributes != 0xFFFF) {
        return "";
    }

    static const int win_to_ansi_fg[] = {30, 34, 32, 36, 31, 35, 33, 37};
    static const int win_to_ansi_bg[] = {40, 44, 42, 46, 41, 45, 43, 47};

    std::string ansi_sequence = "\033[0m";

    int fg_color_index = attributes & 0x0007;
    int fg_ansi = win_to_ansi_fg[fg_color_index];
    if (attributes & FOREGROUND_INTENSITY) {
        fg_ansi += 60;
    }
    ansi_sequence += std::format("\033[{}m", fg_ansi);

    int bg_color_index = (attributes & 0x0070) >> 4;
    int bg_ansi = win_to_ansi_bg[bg_color_index];
    if (attributes & BACKGROUND_INTENSITY) {
        bg_ansi += 60;
    }
    ansi_sequence += std::format("\033[{}m", bg_ansi);

    // Convert simple cyan to specific ANSI code
    if ((attributes & (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)) ==
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)) {
        return "\033[38;5;36m";
    }

    current_screen_attributes = attributes;
    return ansi_sequence;
}

std::string ShellUtils::get_console_output()
{
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_console == INVALID_HANDLE_VALUE) {
        return "";
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(h_console, &csbi)) {
        return "";
    }

    COORD buffer_size = csbi.dwSize;
    if (buffer_size.X == 0 || buffer_size.Y == 0) {
        return ""; // Buffer is non-existent or empty
    }
    // Ensure buffer_length does not overflow if X*Y is very large, though unlikely for CHAR_INFO
    DWORD buffer_length = static_cast<DWORD>(buffer_size.X) * buffer_size.Y;


    std::unique_ptr<CHAR_INFO[]> buffer_data(new CHAR_INFO[buffer_length]);
    COORD char_buffer_coord = {0, 0}; // Read from top-left of our CHAR_INFO buffer

    // The region to read from the console screen buffer
    SMALL_RECT read_region;
    read_region.Left = 0;
    read_region.Top = 0;
    read_region.Right = static_cast<SHORT>(buffer_size.X - 1);
    read_region.Bottom = static_cast<SHORT>(buffer_size.Y - 1);

    // Read the entire console screen buffer (characters and attributes)
    if (!ReadConsoleOutputA(h_console, buffer_data.get(), buffer_size, char_buffer_coord, &read_region)) {
        return "";
    }

    std::vector<std::string> lines_content;
    WORD current_attributes = csbi.wAttributes; // Initialize with the console's current default attributes
                                              // Alternatively, use an invalid value like 0xFFFF to force first color set.

    // Determine the last row that actually contains non-space characters
    int last_effective_row = -1;
    for (SHORT y_check = buffer_size.Y - 1; y_check >= 0; --y_check) {
        for (SHORT x_check = 0; x_check < buffer_size.X; ++x_check) {
            if (buffer_data[static_cast<DWORD>(y_check) * buffer_size.X + x_check].Char.AsciiChar != ' ') {
                last_effective_row = y_check;
                goto found_last_row_label; // Exit both loops
            }
        }
    }
    found_last_row_label:;

    if (last_effective_row == -1) { // Console is empty or contains only spaces
        return "";
    }

    for (SHORT y = 0; y <= last_effective_row; ++y) {
        std::string current_line_buffer;
        int last_char_col_in_row = -1;

        for (SHORT x_check = buffer_size.X - 1; x_check >= 0; --x_check) {
            if (buffer_data[static_cast<DWORD>(y) * buffer_size.X + x_check].Char.AsciiChar != ' ') {
                last_char_col_in_row = x_check;
                break;
            }
        }

        if (last_char_col_in_row == -1) {
            lines_content.push_back("");
        } else {
            for (SHORT x = 0; x <= last_char_col_in_row; ++x) {
                CHAR_INFO char_info = buffer_data[static_cast<DWORD>(y) * buffer_size.X + x];
                current_line_buffer += convert_attributes_to_ansi(char_info.Attributes, current_attributes);
                current_line_buffer += char_info.Char.AsciiChar;
            }
            lines_content.push_back(current_line_buffer);
        }
    }

    std::string final_result_string;
    for (size_t i = 0; i < lines_content.size(); ++i) {
        final_result_string += lines_content[i];
        if (i < lines_content.size() - 1) {
            final_result_string += '\n';
        }
    }

    if (final_result_string.find('\033') != std::string::npos) {
         final_result_string += "\033[0m";
    }

    return final_result_string;
}

void ShellUtils::print_header()
{
    std::println("{}", ascii_art_name);
    std::println("\033[38;5;36mWelcome to ApheliOS!\033[0m");
    // std::println("\033[36mWelcome to ApheliOS!\033[0m");
    std::println("\033[31mType 'exit' to quit, and 'clear' to clear the screen.\n\033[0m");
}

std::function<void(Shell&, bool)> ShellUtils::shell_loop = [](Shell& shell, bool print_head = false) {
    bool quit = false;
    std::string input;
    std::string command;

    bool is_initial_shell = !shell.current_session || shell.current_session->name == "pst";

    if (shell.exit_to_main_menu && !is_initial_shell) {
        return;
    }

#ifdef _WIN32
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(h_out, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(h_out, dwMode);
    SetConsoleOutputCP(CP_UTF8);
#else
    std::locale::global(std::locale("en_US.UTF-8"));
#endif
    if (print_head)
        print_header();

    while (!quit && !shell.exit_to_main_menu) {
        std::print("ApheliOS:~$ ");
        std::getline(std::cin, input);

        if (const size_t start = input.find_first_not_of(" \t\n\r\f\v"); start == std::string::npos) {
            command = "";
        } else {
            const size_t end = input.find_last_not_of(" \t\n\r\f\v");
            command = input.substr(start, end - start + 1);
        }

        // lowercase command if needed
        // not sure if we'll be needing case-sensitive commands
        std::string command_lower =
                command |
                std::ranges::views::transform([](unsigned char c) { return static_cast<char>(std::tolower(c)); }) |
                std::ranges::to<std::string>();

        if (command_lower == "exit") {
            if (is_initial_shell) {
                quit = true;
            } else {
                shell.exit_to_main_menu = true;
                quit = true;
                clear_screen();
                if (shell.last_console_output != "") {
                    std::println("{}", shell.last_console_output);
                    std::println("[screen is terminating]");
                }
                return;
            }
        }
        if (command_lower == "clear") {
            clear_screen();
        } else if (command_lower == "initialize") {
            std::println("Initialize command recognized.");
        } else if (command_lower.contains("screen")) {
            handle_screen_cmd(shell, command, is_initial_shell);
        } else if (command_lower == "scheduler-test") {
            std::println("Scheduler test command recognized.");
        } else if (command_lower == "scheduler-stop") {
            std::println("Scheduler stop command recognized.");
        } else if (command_lower == "report-util") {
            std::println("Report utilization command recognized.");
        } else {
            std::println("{}: command not found", command);
        }

        if (is_initial_shell) {
            shell.exit_to_main_menu = false;
        }
    }
};

void ShellUtils::handle_screen_cmd(Shell& shell, std::string input, bool is_initial_shell)
{
    auto words = input | std::views::split(' ')
    | std::views::filter([](auto&& str) { return !std::ranges::empty(str); })
    | std::views::drop(1)
    | std::views::transform([](auto&& str) { return std::string_view(str.begin(), str.end()); });

    auto args = std::vector(words.begin(), words.end());

    // FOR NOW
    assert(args.size() == 2);

    if (is_initial_shell) {
        shell.last_console_output = get_console_output();
    }

    if (args[0] == "-S") {
        shell.create_screen(args[1].data());
    } else {
        shell.switch_screen(args[1].data());
    }
}


 Shell::Shell()
 {
    create_session("pst", true);
 }


void Shell::run()
{
    shell_process->run(std::bind(ShellUtils::shell_loop, *this, true));
}

void Shell::create_screen(const std::string &name)
{
    ShellUtils::clear_screen();
    create_session(name);
    current_process_group->processes[0]->run([this] {
        std::println("Process name: {}", current_process_group->processes[0]->name);
        std::string timestamp = std::format("{:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime);
        std::println("Current time: {}", timestamp);
        ShellUtils::shell_loop(*this, false);
    });
}

void Shell::switch_screen(const std::string &name)
{
    switch_session(name);
    ShellUtils::clear_screen();
    current_session->process_groups[0].processes[0]->run([this]() {
        std::println("Process name: {}", current_process_group->processes[0]->name);
        std::string timestamp = std::format("{:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime);
        std::println("Current time: {}", timestamp);
       ShellUtils::shell_loop(*this, false);
    });
}

void Shell::exit_screen()
{
    current_session = sessions.back();
    current_session->process_groups[0].processes[0]->run([this]() {
        ShellUtils::shell_loop(*this, false);
    });
}

void Shell::create_session(const std::string &session_name, bool has_leader)
{
    auto new_session = std::make_shared<Session>();
    new_session->id = current_sid++;
    new_session->name = session_name;
    auto now = std::chrono::system_clock::now();
    // Truncate to seconds precision
    auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    new_session->createTime = std::chrono::zoned_time{std::chrono::current_zone(), now_seconds};

    auto new_group = std::make_shared<ProcessGroup>();
    new_group->sid = new_session->id;

     if (!has_leader) {
         auto new_process = std::make_shared<Process>(current_pid, session_name);
         new_group->id = current_pid++;
         new_group->processes.push_back(new_process);
     } else {
         new_group->processes.push_back(shell_process);
     }

    new_session->process_groups.push_back(*new_group);
    sessions.push_back(new_session);
    current_session = new_session;
    current_process_group = new_group;
}

void Shell::switch_session(const std::string &session_name)
{
    for (auto &session : sessions) {
        if (session->name == session_name) {
            current_session = session;
            current_process_group = std::make_shared<ProcessGroup>(current_session->process_groups[0]);
            break;
        }
    }
}
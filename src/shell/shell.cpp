#include "shell.h"

#include <iostream>
#include <print>
#include <ranges>
#include <string>

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
void ShellUtils::clear_screan() { std::cout << "\033[2J\033[1;1H"; }
#endif


void Shell::run()
{
    bool quit = false;
    std::string input;
    std::string command;

    constexpr std::string_view ascii_art_name = R"(
                        ,,                 ,,    ,,
      db              `7MM               `7MM    db   .g8""8q.    .M"""bgd
     ;MM:               MM                 MM       .dP'    `YM. ,MI    "Y
    ,V^MM.   `7MMpdMAo. MMpMMMb.  .gP"Ya   MM  `7MM dM'      `MM `MMb.
   ,M  `MM     MM   `Wb MM    MM ,M'   Yb  MM    MM MM        MM   `YMMNq.
   AbmmmqMA    MM    M8 MM    MM 8M""""""  MM    MM MM.      ,MP .     `MM
  A'     VML   MM   ,AP MM    MM YM.    ,  MM    MM `Mb.    ,dP' Mb     dM
.AMA.   .AMMA. MMbmmd'.JMML  JMML.`Mbmmd'.JMML..JMML. `"bmmd"'   P"Ybmmd"
               MM
             .JMML.

)";

    std::println("{}", ascii_art_name);
    std::println("Welcome to ApheliOS!");
    std::println("Type 'exit' to quit, and 'clear' to clear the screen.\n");

    while (!quit) {
        std::print("ApheliOS:~$ ");
        std::cin >> input;

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
            quit = true;
        } else if (command_lower == "clear") {
            ShellUtils::clear_screen();
        } else if (command_lower == "initialize") {
            std::println("Initialize command recognized.");
        } else if (command_lower == "scheduler-test") {
            std::println("Scheduler test command recognized.");
        } else if (command_lower == "scheduler-stop") {
            std::println("Scheduler stop command recognized.");
        } else if (command_lower == "report-util") {
            std::println("Report utilization command recognized.");
        } else {
            std::println("{}: command not found", command);
        }
    }
}

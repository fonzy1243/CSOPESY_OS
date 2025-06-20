#include "shell.h"

#include <iostream>
#include <format>
#include <print>
#include <ranges>
#include <string>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <random>
#include <memory>

// FTXUI
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>


#ifdef _WIN32
#include <windows.h>
#else
#include <locale>
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
        int last_char_col_in_row = -1;

        for (SHORT x_check = buffer_size.X - 1; x_check >= 0; --x_check) {
            if (buffer_data[static_cast<DWORD>(y) * buffer_size.X + x_check].Char.AsciiChar != ' ') {
                last_char_col_in_row = x_check;
                break;
            }
        }

        if (last_char_col_in_row == -1) {
            lines_content.emplace_back("");
        } else {
            std::string current_line_buffer;
            for (SHORT x = 0; x <= last_char_col_in_row; ++x) {
                auto [Char, Attributes] = buffer_data[static_cast<DWORD>(y) * buffer_size.X + x];
                current_line_buffer += convert_attributes_to_ansi(Attributes, current_attributes);
                current_line_buffer += Char.AsciiChar;
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

void ShellUtils::print_header(std::deque<std::string>& output_buffer)
{
    output_buffer.emplace_back(ascii_art_name);
    output_buffer.emplace_back("Welcome to ApheliOS!");
    output_buffer.emplace_back("Type 'exit' to quit, and 'clear' to clear the screen.");
}

void ShellUtils::process_command(Shell &shell, const std::string &input, bool is_initial_shell)
{
    std::string command;

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
            shell.quit = true;
        } else {
            //save current
            if (shell.current_session) {
                shell.current_session->output_buffer = shell.output_buffer;
            }

            shell.output_buffer.emplace_back("[screen is terminating]");//append

            if (shell.current_session) { //save
                shell.current_session->output_buffer = shell.output_buffer;
            }

            shell.exit_screen();
            shell.quit = false;
            shell.exit_to_main_menu = true;
            return;
        }
    }
    if (command_lower == "clear") {
        clear_screen();
        shell.output_buffer.clear();
    } else if (command_lower == "initialize") {
        shell.output_buffer.emplace_back("Initialize command recognized.");
    } else if (command_lower.contains("screen")) {
        handle_screen_cmd(shell, command, is_initial_shell);
    } else if (command_lower == "marquee") {
        toggle_marquee_mode(shell);
    } else if (command_lower == "scheduler-test") {
        shell.output_buffer.emplace_back("Scheduler test command recognized.");
    } else if (command_lower == "scheduler-stop") {
        shell.output_buffer.emplace_back("Scheduler stop command recognized.");
    } else if (command_lower == "report-util") {
        shell.output_buffer.emplace_back("Report utilization command recognized.");
    } else if (command_lower == "smi") {
        display_smi(shell);
    } else if (command_lower != "exit") {
        shell.output_buffer.push_back(std::format("{}: command not found", command));
    }

    if (is_initial_shell) {
        shell.exit_to_main_menu = false;
    }
}

void ShellUtils::display_smi(Shell &shell)
{
   using namespace ftxui;
    // GPU Top Bar Info (above the table)
    auto gpu_top_info = hbox({
        text("NVIDIA-SMI 551.86") | center,
        filler(),
        text("Driver Version: 551.86") | flex | center,
        filler(),
        text("CUDA Version: 12.9") | center,
    });

    auto cell = [](const char* t) {
        auto style = [](const Element &e) {
            return e | flex | size(WIDTH, GREATER_THAN, 6) | size(HEIGHT, EQUAL, 1);

        };
        return text(t) | style;
    };

    auto empty_cell = []() {
        auto style = [](const Element &e) {
            return e | flex | center | size(WIDTH, GREATER_THAN, 10) | size(HEIGHT, EQUAL, 1);
        };
        return text("") | style;
    };

    auto gpu_info_header = hbox({
        gridbox({
            { cell("GPU"), cell("Name") | center, empty_cell(), empty_cell(), cell("Driver-Model") | center },
            { cell("Fan"), cell("Temp") | center, cell("Perf") | center, empty_cell(), cell("Pwr:Usage/Cap") | center }
        }) | size(WIDTH, EQUAL, 46),
       separatorDashed() ,
        gridbox({
            {cell("Bus-Id"), cell("Disp.A") | align_right},
           {empty_cell(), cell("Memory-Usage") | align_right},
        }) | center | size(WIDTH, EQUAL, 27),
        separatorDashed(),
        vbox({
            text("Volatile Uncorr. ECC") | align_right,
            hbox({
                 cell("GPU-Util"), filler(), cell("Compute M.") | align_right,
            }),
            text("MIG M.") | align_right,
        }) | center | size(WIDTH, EQUAL, 21),
    });

    auto gpu_info = hbox({
        gridbox({
            { hbox({cell("0"), cell("NVIDIA GeForce GTX 1080 Ti") | center, filler(), cell("WDDM") }) | flex_grow, },
            { hbox({cell("22%"), cell("54C") | center, separatorEmpty(), separatorEmpty(), separatorEmpty(), cell("P0") | center, filler(), cell("68W"), cell("/"),cell("275W")   }) | flex_grow, },
        }) | size(WIDTH, EQUAL, 46),
        separatorDashed(),
        gridbox({
                {cell("00000000:01:00.0"), cell("   On") | align_right},
               {text("3134MiB /  "), text("11264MiB") | align_right},
            }) | center | size(WIDTH, EQUAL, 27),
        separatorDashed(),
        vbox({
            text("N/A") | align_right,
            hbox({
                 cell("3%"), filler(), cell("Default") | align_right,
            }),
            text("N/A") | align_right,
        }) | align_right | size(WIDTH, EQUAL, 21),
    });

    // Process Table
    Table process_table = Table({
        {"Processes:"},
      { "GPU", "GI","CI", "      ", "PID", "Type", "Process name", "GPU Memory" },
        {"", "ID", "ID", "      ", "", "", "", "Usage"},
      { "0", "N/A","N/A", "      ", "11368", "C+G", R"(C:\Windows\System32\dwm.exe)", "N/A" },
      { "0", "N/A","N/A", "      ","2116", "C+G", "C:\\w...bXboxGameBarWidgets.exe", "N/A" },
      { "0", "N/A","N/A", "      ","5224", "C+G", "C:\\x123.0.2202.56\\msedgewebview2.exe", "N/A" },
      { "0", "N/A","N/A", "      ","6640", "C+G", "C:\\Windows\\explorer.exe", "N/A" },
      { "0", "N/A","N/A", "      ","6676", "C+G", "C:\\...SearchHost.exe", "N/A" },
      { "0", "N/A","N/A", "      ","6700", "C+G", "C:\\ztyeny\\StartMenuExperienceHost.exe", "N/A" },
    });

    process_table.SelectAll().Border(EMPTY);
    process_table.SelectRow(2).SeparatorVertical(EMPTY);
    process_table.SelectRow(2).BorderBottom(BorderStyle::DOUBLE);

    auto gpu_info_table = vbox({
        gpu_top_info, separatorDashed(), gpu_info_header, separatorHeavy(), gpu_info
    }) | borderDashed | size(WIDTH, EQUAL, 95);

    auto gpu_process_table = vbox({
        process_table.Render() | center | flex_grow
    }) | borderDashed | size(WIDTH, EQUAL, 95) | size(HEIGHT, GREATER_THAN, 20);

    auto smi = vbox({gpu_info_table, gpu_process_table});

    auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(smi));
    Render(screen, smi);

    std::string smi_output = screen.ToString();
    shell.output_buffer.push_back(smi_output);
}



std::function<void(Shell&, bool)> ShellUtils::shell_loop = [](Shell& shell, bool print_head = false) {
    using namespace ftxui;

    std::string input;
    const std::string prompt_text = "ApheliOS:~$ ";

    auto input_option = InputOption();
    input_option.on_enter = [&] {
        shell.output_buffer.push_back(prompt_text + input);

        bool is_initial_shell = !shell.current_session || shell.current_session->name == "pst";
        process_command(shell, input, is_initial_shell);

        input.clear();

        if (shell.quit) {
            shell.screen.Exit();
        }

        shell.screen.RequestAnimationFrame();
    };
    input_option.transform = [](InputState state) {
        if (state.focused || state.hovered) {
            state.element |= bgcolor(Color::RGBA(0, 0, 0, 0));
        }
        return state.element;
    };

    Component input_field = Input(&input, input_option);

    if (print_head && shell.output_buffer.empty()) {
        print_header(shell.output_buffer);
    }

    auto render_output = [&] {
        std::vector<Element> elements;

        for (const auto& line : shell.output_buffer) {
            elements.push_back(paragraph(line) | color(Color::Default));
        }

        return vbox(elements);
    };

    auto prompt = text(prompt_text) | color(Color::Default);

    const auto layout = Container::Vertical({input_field});

    const auto renderer = Renderer(layout, [&] {
        auto marquee_display = create_marquee_display(shell);

        return vbox({
            vbox(render_output() | flex),
            marquee_display,
            hbox({
                text(prompt_text),
                input_field->Render() | selectionForegroundColor(Color::Aquamarine1) | flex,
            })
        }) | yflex | yframe;
    });

    auto loop = Loop(&shell.screen, renderer);

    while (!shell.quit) {
        loop.RunOnce();

        if (shell.marquee_mode) {
            shell.screen.RequestAnimationFrame();
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
    // i commented out cause of ls command
    // assert(args.size() == 2);

    if (shell.current_session) {
    shell.current_session->output_buffer = shell.output_buffer;
    }

    if (args[0] == "-S") {
        shell.create_screen(args[1].data());
    } else if (args[0] == "-r") {
        shell.switch_screen(args[1].data());
    } else if (args[0] == "-ls") {
        std::string status = shell.scheduler.get_status_string();
        shell.output_buffer.push_back(status);
    }
}

void ShellUtils::toggle_marquee_mode(Shell& shell)
{
    shell.marquee_mode = !shell.marquee_mode;
    if (shell.marquee_mode) {
        shell.output_buffer.emplace_back("Marquee mode enabled! Use 'marquee' to disable.");
        std::uniform_int_distribution x_dist(0, shell.marquee_width - static_cast<int>(shell.marquee_text.length()));
        std::uniform_int_distribution y_dist(0, shell.marquee_height - 1);
        shell.marquee_x = x_dist(shell.marquee_rng);
        shell.marquee_y = y_dist(shell.marquee_rng);
    } else {
        shell.output_buffer.emplace_back("Marquee mode disabled!");
    }
}

void ShellUtils::update_marquee_position(Shell& shell)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - shell.last_marquee_update);

    if (elapsed.count() >= 50) {
        shell.marquee_x += shell.marquee_dx;
        shell.marquee_y += shell.marquee_dy;

        int text_width = static_cast<int>(shell.marquee_text.length());
        if (shell.marquee_x <= 0 || shell.marquee_x + text_width > shell.marquee_width) {
            shell.marquee_dx = -shell.marquee_dx;
            shell.marquee_x = std::clamp(shell.marquee_x, 0, shell.marquee_width - text_width);
        }

        if (shell.marquee_y <= 0 || shell.marquee_y >= shell.marquee_height) {
            shell.marquee_dy = -shell.marquee_dy;
            shell.marquee_y = std::clamp(shell.marquee_y, 0, shell.marquee_height - 1);
        }

        shell.last_marquee_update = now;
    }
}

ftxui::Element ShellUtils::create_marquee_display(Shell &shell)
{
    using namespace ftxui;

    if (!shell.marquee_mode) {
        return text("") | size(HEIGHT, EQUAL, 0);
    }

    update_marquee_position(shell);

    std::vector<std::vector<Element>> marquee_grid;

    for (int y = 0; y < shell.marquee_height; y++) {
        std::vector<Element> row;
        for (int x = 0; x < shell.marquee_width;) {
            if (y == shell.marquee_y && x >= shell.marquee_x && x < shell.marquee_x + static_cast<int>(shell.marquee_text.length())) {
                for (size_t i = 0; i < shell.marquee_text.length() && x < shell.marquee_width; i++, x++) {
                    row.push_back(text(std::string(1, shell.marquee_text[i])) | color(Color::Cyan));
                }
            } else {
                row.push_back(text(" "));
                x++;
            }
        }
        marquee_grid.push_back(std::move(row));
    }

    std::vector<Element> marquee_rows;
    for (const auto& row : marquee_grid) {
        marquee_rows.push_back(hbox(row));
    }

    return vbox(marquee_rows) | border | size(WIDTH, EQUAL, shell.marquee_width + 2) | size(HEIGHT, EQUAL, shell.marquee_height + 2);
}


Shell::Shell()
{
    create_session("pst", true, nullptr);
    // Initialize the header for the main session
    ShellUtils::print_header(output_buffer);
    current_session->output_buffer = output_buffer;
}

void Shell::run()
{
    // create needed processes
    for (int i = 1; i <= 10; i++) {
        auto process = std::make_shared<Process>(i, std::format("process_{}", i));
        process->generate_print_instructions();

        // store processes in sessions
        auto session_name = std::format("session_{}", i);
        create_session(session_name, false, process);

    }

    // switch session back to original, overrides if removed
    switch_session("pst");

    scheduler.start();
    // Add all processes to scheduler
    for (const auto& session : sessions) {
        if (session->process && session->name != "pst") {
            scheduler.add_process(session->process);
        }
    }

    shell_process->run(std::bind(ShellUtils::shell_loop, std::ref(*this), true));

    scheduler.stop();
}


void Shell::create_screen(const std::string &name)
{
    if (current_session) {
        current_session->output_buffer = output_buffer; // save
    }

    // Create new process for the screen
    auto new_process = std::make_shared<Process>(current_pid++, name);
    create_session(name, false, new_process);

    output_buffer.clear();

    current_session->process->run([this] {
        output_buffer.push_back(std::format("Process name: {}", current_session->process->name));
        output_buffer.push_back(std::format("Current time: {:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime));
        output_buffer.push_back(std::format("Line info: {}/{}", output_buffer.size(), output_buffer.size()));

        current_session->output_buffer = output_buffer; // save
    });
}

void Shell::switch_screen(const std::string &name)
{
    if (current_session) {
        current_session->output_buffer = output_buffer; // save
    }

    switch_session(name);
    output_buffer = current_session->output_buffer; // restore

    if (output_buffer.empty()) {
        current_session->process->run([this]() {
            output_buffer.push_back(std::format("Process name: {}", current_session->process->name));
            output_buffer.push_back(std::format("Current time: {:%m/%d/%Y, %I:%M:%S %p}", current_session->createTime));
            output_buffer.push_back(std::format("Line info: {}/{}", output_buffer.size(), output_buffer.size()));

            current_session->output_buffer = output_buffer;
        });
    }
}

void Shell::exit_screen()
{
    // Save current session state if it's not pst
    if (current_session && current_session->name != "pst") {
        current_session->output_buffer = output_buffer;
    }

    // Find and switch to pst session
    for (auto& session : sessions) {
        if (session->name == "pst") {
            current_session = session;
            output_buffer = current_session->output_buffer;
            break;
        }
    }
}

void Shell::create_session(const std::string &session_name, bool has_leader, std::shared_ptr<Process> process)
{
    auto new_session = std::make_shared<Session>();
    new_session->id = current_sid++;
    new_session->name = session_name;

    auto now = std::chrono::system_clock::now();
    // Truncate to seconds precision
    auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    new_session->createTime = std::chrono::zoned_time{std::chrono::current_zone(), now_seconds};

    if (has_leader) {
        new_session->process = shell_process;
    } else {
        new_session->process = process ? process : std::make_shared<Process>(current_pid++, session_name);
    }

    sessions.push_back(new_session);
    current_session = new_session;
}

void Shell::switch_session(const std::string &session_name)
{
    for (auto &session : sessions) {
        if (session->name == session_name) {
            current_session = session;
            break;
        }
    }
}
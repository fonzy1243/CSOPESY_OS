#include "shell.h"
#include "../aphelios.h"
#include "../cpu_tick.h"

#include <iostream>
#include <format>
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

void ShellUtils::print_header(std::deque<std::string>& output_buffer)
{
    std::istringstream iss(ascii_art_name.data());
    std::string line;
    while (std::getline(iss, line)) {
        output_buffer.push_back(line);
    }
    output_buffer.emplace_back("Welcome to ApheliOS!");
    output_buffer.emplace_back("Type 'exit' to quit, and 'clear' to clear the screen.");
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

void Shell::add_multiline_output(const std::string &content)
{
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        output_buffer.push_back(line);
    }
}

void Shell::shell_loop(bool print_header)
{
    using namespace ftxui;

    int scroll_offset = 0;
    bool autoscroll = true;
    bool pendingscroll = false;
    std::string input;

    const std::string prompt_text = "ApheliOS:~$ ";

    auto input_option = InputOption();
    input_option.on_enter = [&] {
        autoscroll = true;
        pendingscroll = true;
        output_buffer.push_back(prompt_text + input);

        apheli_os.process_command(input);

        if (apheli_os.current_session && apheli_os.current_session->process) {
            output_buffer.insert(output_buffer.end(), apheli_os.current_session->process->output_buffer.begin(),
                                 apheli_os.current_session->process->output_buffer.end());
            apheli_os.current_session->process->output_buffer.clear();
        }

        output_buffer.push_back("");
        input.clear();

        if (apheli_os.quit) {
            screen.Exit();
        }

        screen.RequestAnimationFrame();
    };

    input_option.transform = [](InputState state) {
        if (state.focused || state.hovered) {
            state.element |= bgcolor(Color::RGBA(0, 0, 0, 0));
        }
        return state.element;
    };

    Component input_field = Input(&input, input_option);

    if (print_header && output_buffer.empty()) {
        ShellUtils::print_header(output_buffer);
    }

    auto render_output = [&]() -> Element {
        std::vector<Element> elements;
        int total_lines = static_cast<int>(output_buffer.size());
        int height = screen.dimy();
        int max_lines = std::max(1, height - 3); 

        int max_scroll = std::max(0, total_lines - max_lines);
        scroll_offset = std::clamp(scroll_offset, 0, max_scroll);

        int start = scroll_offset;
        int end = std::min(start + max_lines, total_lines);

        for (int i = start; i < end; ++i) {
            elements.push_back(paragraph(output_buffer[i]) | color(Color::Default));
        }

        return vbox(elements);
    };

    const auto layout = Container::Vertical({input_field});

    auto base_renderer = Renderer(layout, [&] {
    auto marquee_display = ShellUtils::create_marquee_display(*this);
    
    //so it has the border and 3 lines alloted space
    auto bottom_section = vbox({
        text(""), 
        hbox({
            text(prompt_text),
            input_field->Render() | selectionForegroundColor(Color::Aquamarine1) | flex
        }), 
        text("")   
    });  
    
    return vbox({
        render_output() | flex,
        marquee_display, 
        bottom_section  
    }) | yflex;
});

    auto renderer = CatchEvent(base_renderer, [&](Event event) {
        int height = screen.dimy();
        int max_lines = std::max(1, height - 3); 
        int total_lines = static_cast<int>(output_buffer.size());
        int max_scroll = std::max(0, total_lines - max_lines);

        if (event == Event::ArrowUp) {
            autoscroll = false;
            scroll_offset = std::max(0, scroll_offset - 1);
            return true;
        }
        if (event == Event::ArrowDown) {
            autoscroll = false;
            scroll_offset = std::min(scroll_offset + 1, max_scroll);
            return true;
        }
        if (event == Event::PageUp) {
            autoscroll = false;
            scroll_offset = std::max(0, scroll_offset - max_lines);
            return true;
        }
        if (event == Event::PageDown) {
            autoscroll = false;
            scroll_offset = std::min(scroll_offset + max_lines, max_scroll);
            return true;
        }
        if (event == Event::Home) {
            autoscroll = false;
            scroll_offset = 0;
            return true;
        }
        if (event == Event::End) {
            autoscroll = true;
            scroll_offset = max_scroll;
            return true;
        }
        return false;
    });

    auto loop = Loop(&screen, renderer);

    while (!apheli_os.quit) {
        loop.RunOnce();

        // autoscroll for newly added content
        if (autoscroll && pendingscroll) {
            int height = screen.dimy();
            int max_lines = std::max(1, height - 3);
            int total_lines = static_cast<int>(output_buffer.size());
            scroll_offset = std::max(0, total_lines - max_lines);
            pendingscroll = false;
            screen.PostEvent(Event::Custom);
        }

        // for marquee scrolling
        if (marquee_mode) {
            int height = screen.dimy();
            int max_lines = std::max(1, height - 8);
            int total_lines = static_cast<int>(output_buffer.size());
            if (autoscroll) {
                scroll_offset = std::max(0, total_lines - max_lines);
            }
            screen.PostEvent(Event::Custom);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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


Shell::Shell(ApheliOS& aphelios_ref) : apheli_os(aphelios_ref)
{
    shell_process = std::make_shared<Process>(0, "pts", apheli_os.memory);
}

void Shell::run(bool print_header)
{
    shell_loop(print_header);
}

#ifndef SHELL_H
#define SHELL_H
#include <functional>
#include <memory>
#include <queue>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <random>

#include "../aphelios.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../session/session.h"

class ApheliOS;
class Shell;

namespace ShellUtils {
    void clear_screen();
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
    void print_header(std::deque<std::string>& output_buffer);
    void process_command(Shell& shell, const std::string& input, bool is_init_shell);
    extern std::function<void(Shell&, bool)> shell_loop;
    void handle_screen_cmd(Shell& shell, std::string input, bool is_initial_shell);
    void display_smi(Shell& shell);
    void toggle_marquee_mode(Shell& shell);
    void update_marquee_position(Shell& shell);
    ftxui::Element create_marquee_display(Shell& shell);
};

class Shell {
public:
    ApheliOS& apheli_os;
    std::shared_ptr<Process> shell_process;

    // Marquee
    bool marquee_mode{false};
    std::string marquee_text{"So many weapons, Aphelios. The deadliest is your faith."};
    int marquee_x{0};
    int marquee_y{0};
    int marquee_dx{1};
    int marquee_dy{1};
    int marquee_width{80};
    int marquee_height{20};
    std::chrono::steady_clock::time_point last_marquee_update{std::chrono::steady_clock::now()};
    std::mt19937 marquee_rng{std::random_device{}()};

    std::deque<std::string> output_buffer;
    ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();
    bool exit_to_main_menu{false};

    explicit Shell(ApheliOS& aphelios_ref);

    void run(bool print_header);

private:
    void shell_loop(bool print_header);
};

#endif //SHELL_H

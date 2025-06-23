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
#include <random>

#include "../../cmake-build-release/_deps/ftxui-src/include/ftxui/dom/elements.hpp"
#include "../process/process.h"
#include "../session/session.h"
#include "../scheduler/scheduler.h"

class Shell;

namespace ShellUtils {
    void clear_screen();
    std::string convert_attributes_to_ansi(WORD attributes, WORD& current_screen_attributes);
    std::string get_console_output();
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
    Scheduler scheduler{4};
    std::vector<std::shared_ptr<Session>> sessions;
    std::shared_ptr<Session> current_session;
    std::shared_ptr<Process> shell_process = std::make_shared<Process>(0, "pts");

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

    std::deque<std::string> last_console_output;
    std::deque<std::string> output_buffer;
    ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();


    bool exit_to_main_menu{false};
    bool quit{false};

    Shell();

    void run();

    void create_screen(const std::string &name);
    void switch_screen(const std::string &name);
    void exit_screen();

private:
    uint16_t current_pid{0};
    uint16_t current_sid{0};


    // added an extra argument for processes
    void create_session(const std::string &session_name, bool has_leader = false, std::shared_ptr<Process> process = nullptr);
    void switch_session(const std::string &session_name);

};

#endif //SHELL_H

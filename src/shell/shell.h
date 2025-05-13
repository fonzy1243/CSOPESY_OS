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
    void print_header();
    extern std::function<void(Shell&, bool)> shell_loop;
    void handle_screen_cmd(Shell& shell, std::string input, bool is_initial_shell);
};

class Shell {
public:
    void run();

private:
};

#endif //SHELL_H

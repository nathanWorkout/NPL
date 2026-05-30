#ifndef RUNTIME_TUI_NCURSES_HPP
#define RUNTIME_TUI_NCURSES_HPP

#include <string>
#include <vector>

class Interpreter;
struct Value;

namespace runtime {

void register_tui_functions(Interpreter& interp);

}

#endif

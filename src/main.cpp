#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/ast.hpp"
#include "../include/interpreter.hpp"

#include "runtime/core.hpp"
#include "runtime/backend_net.hpp"
#include "runtime/tui_ncurses.hpp"

int main(int argc, char* argv[])
{
    if(argc < 2) {
        std::cerr << "usage: " << argv[0] << " <file.npl>" << std::endl;
        return 1;
    }

    // Active ncurses uniquement si --tui est passé
    bool use_tui = (argc > 2 && std::string(argv[2]) == "--tui");

    Interpreter interp;

    std::ifstream file(argv[1]);
    if(!file.is_open()) {
        if(interp.is_curses_mode()) endwin();

        std::cerr << "erreur: impossible d'ouvrir " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string src = buf.str();

    std::vector<Token> tokens = tokenize(src);

    Parser parser(tokens);
    auto ast = parser.parse();

    runtime::register_core_functions(interp);
    runtime::register_net_functions(interp);
    runtime::register_tui_functions(interp);

    try {
            interp.run(ast.get());
        }
        catch(const std::runtime_error& e) {
            if(interp.is_curses_mode()) endwin();

            std::cerr << "\033[1;31m[Erreur d'exécution NPL] " << e.what() << "\033[0m" << std::endl;
            return 1;
        }
        catch(const std::exception& e) {
            if(interp.is_curses_mode()) endwin();
            std::cerr << "[Erreur Système] " << e.what() << std::endl;
            return 1;
        }

    return 0;
}

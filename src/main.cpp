#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
// #include "codegen.hpp"
#include "interpreter.hpp"

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cerr << "usage: <file.npl>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if(!file.is_open())
    {
        std::cerr << "erreur: impossible d'ouvrir " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string src = buf.str();

    std::vector<Token> tokens = tokenize(src);

    Parser parser(tokens);
    auto ast = parser.parse();

    Interpreter interp;
    interp.run(ast.get());

    // Codegen codegen("output.asm");
    // codegen.generate(ast.get());
    // codegen.flush();

    return 0;
}
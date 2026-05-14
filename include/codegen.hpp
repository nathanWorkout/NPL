#ifndef CODEGEN_HPP
#define CODEGEN_HPP
#include "ast.hpp"
#include <fstream>
#include <string>
#include <set>

class Codegen {
    public:
        Codegen(const std::string& output_file);
        void generate(ASTNode* node);
        void flush();

    private:
        std::ofstream out_;
        std::string data_;
        std::string text_;
        std::string text_prefix_;
        std::set<std::string> string_vars_;
        std::set<std::string> bool_vars_;
        
int str_counter_ = 0;
void gen_block(Block* node);
void gen_assign(Assign* node);
void gen_output(Output* node);
void gen_if(IfStmt* node);
void gen_repeat(RepeatStmt* node);
void gen_while(WhileStmt* node);
};
#endif
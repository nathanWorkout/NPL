#ifndef PARSER_HPP
#define PARSER_HPP

#include "token.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>

class Parser {
    public:
        Parser(std::vector<Token>& tokens);
        std::unique_ptr<ASTNode> parse();

    private:
        std::vector<Token>& tokens_;
        size_t i = 0;

        Token& peek();
        Token& consume();

        std::unique_ptr<ASTNode> parse_statement();
        std::unique_ptr<ASTNode> parse_assign();
        std::unique_ptr<ASTNode> parse_output();
        std::unique_ptr<ASTNode> parse_expr();
        std::unique_ptr<ASTNode> parse_if();
        std::unique_ptr<ASTNode> parse_while();
        std::unique_ptr<ASTNode> parse_for();
        std::unique_ptr<ASTNode> parse_repeat();
        std::unique_ptr<ASTNode> parse_function();
        std::unique_ptr<ASTNode> parse_try();
};

#endif
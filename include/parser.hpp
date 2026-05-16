#pragma once
#include <vector>
#include <memory>
#include "token.hpp"
#include "ast.hpp"

class Parser
{
public:
    Parser(std::vector<Token>& tokens);
    std::unique_ptr<ASTNode> parse();

private:
    std::vector<Token>& tokens_;
    size_t i = 0;

    Token& peek();
    Token& consume();
    bool   at_end();

    void expect_arrow();
    std::unique_ptr<ASTNode> parse_body();

    std::unique_ptr<ASTNode> parse_statement();
    std::unique_ptr<ASTNode> parse_expr();
    std::unique_ptr<ASTNode> parse_primary();
    std::unique_ptr<ASTNode> parse_cond();
    std::unique_ptr<ASTNode> parse_assign();
    std::unique_ptr<ASTNode> parse_output();
    std::unique_ptr<ASTNode> parse_if();
    std::unique_ptr<ASTNode> parse_repeat();
    std::unique_ptr<ASTNode> parse_while();
    std::unique_ptr<ASTNode> parse_for();
    std::unique_ptr<ASTNode> parse_funcdef();
    std::unique_ptr<ASTNode> parse_funccall();
    std::unique_ptr<ASTNode> parse_return();
    std::unique_ptr<ASTNode> parse_use();
    std::unique_ptr<ASTNode> parse_comparison();
    std::unique_ptr<ASTNode> parse_try();
    std::unique_ptr<ASTNode> parse_throw();
};
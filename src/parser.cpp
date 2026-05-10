#include <iostream>
#include "parser.hpp"

Parser::Parser(std::vector<Token>& tokens) : tokens_(tokens) {}

Token& Parser::peek()
{
    return tokens_[i];
}

Token& Parser::consume()
{
    return tokens_[i++];
}

std::unique_ptr<ASTNode> Parser::parse()
{
    auto block = std::make_unique<Block>();
    
    while(i < tokens_.size())
    {
        block->statements.push_back(parse_statement());
    }
    
    return block;
}

std::unique_ptr<ASTNode> Parser::parse_statement()
{
    if(peek().type == TokenType::IDENTIFIER)
    {
        return parse_assign();
    }
    else if(peek().type == TokenType::OUTPUT && peek().value == ">>")
    {
        return parse_output();
    }

    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parse_expr()
{
    if(peek().type == TokenType::NUMBER)
    {
        auto node = std::make_unique<NumberLit>();
        node->value = std::stod(consume().value);
        return node;
    }
    else if(peek().type == TokenType::STRING)
    {
        auto node = std::make_unique<StringLit>();
        node->value = consume().value;
        return node;
    }
    else if(peek().type == TokenType::BOOL)
    {
        auto node = std::make_unique<BoolLit>();
        node->value = consume().value == "true";
        return node;
    }
    else if(peek().type == TokenType::IDENTIFIER)
    {
        auto node = std::make_unique<Identifier>();
        node->name = consume().value;
        return node;
    }
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parse_assign()
{
    auto node = std::make_unique<Assign>();
    node->name = consume().value;
    consume();
    node->value = parse_expr();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_output()
{
    auto node = std::make_unique<Output>();
    consume();
    node->value = parse_expr();
    return node;
}
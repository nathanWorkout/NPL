#include <iostream>
#include "parser.hpp"

Parser::Parser(std::vector<Token>& tokens) : tokens_(tokens) {}

Token& Parser::peek()
{
    if(i >= tokens_.size())
        throw std::runtime_error("Fin de fichier inattendue");
    return tokens_[i];
}
Token& Parser::consume()
{
    return tokens_[i++];
}

bool Parser::at_end()
{
    return i >= tokens_.size();
}

void Parser::expect_arrow()
{
    consume(); // ->
}

std::unique_ptr<ASTNode> Parser::parse_body()
{
    expect_arrow(); 

    if(!at_end() && peek().type == TokenType::PUNCTUATOR && peek().value == "{")
    {
        consume();
        auto blk = std::make_unique<Block>();
        while(!at_end() && !(peek().type == TokenType::PUNCTUATOR && peek().value == "}"))
        {
            blk->statements.push_back(parse_statement());
        }
        consume();
        return blk;
    }
    else
    {
        return parse_statement();
    }
}

std::unique_ptr<ASTNode> Parser::parse()
{
    auto block = std::make_unique<Block>();
    while(!at_end()) block->statements.push_back(parse_statement());
    return block;
}


std::unique_ptr<ASTNode> Parser::parse_statement()
{
    if(at_end()) return nullptr;

    if(peek().value == "fn") return parse_funcdef();
    if(peek().type == TokenType::IDENTIFIER)
    {
        if(i+1 < tokens_.size() && tokens_[i+1].value == "(")
            return parse_funccall();
        return parse_assign();
    }
    if(peek().type == TokenType::OUTPUT) return parse_output();
    if(peek().type == TokenType::RETURN_TOK) return parse_return();
    if(peek().value == "if") return parse_if();
    if(peek().value == "elif") { consume(); return nullptr; }
    if(peek().value == "else") { consume(); return nullptr; }
    if(peek().value == "repeat") return parse_repeat();
    if(peek().value == "while") return parse_while();
    if(peek().value == "for") return parse_for();
    if(peek().value == "use") return parse_use();
    if(peek().value == "try") return parse_try();
    if(peek().value == "throw") return parse_throw();

    consume();
    return nullptr;
}
std::unique_ptr<ASTNode> Parser::parse_primary()
{
    if(at_end()) return nullptr;

    if(peek().type == TokenType::OPERATOR && peek().value == "-")
    {
        consume();
        auto operand = parse_primary();
        auto binop   = std::make_unique<BinOp>();
        auto zero    = std::make_unique<NumberLit>();
        zero->value  = 0.0;
        binop->op    = "-";
        binop->lhs   = std::move(zero);
        binop->rhs   = std::move(operand);
        return binop;
    }

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
        node->value = (consume().value == "true");
        return node;
    }
    else if(peek().type == TokenType::IDENTIFIER)
    {
        std::string name = consume().value;
        if(!at_end() && peek().value == "(")
        {
            consume();
            auto node = std::make_unique<FuncCall>();
            node->name = name;
            while(!at_end() && peek().value != ")")
            {
                node->args.push_back(parse_expr());
                if(!at_end() && peek().value == ",") consume();
            }
            consume();
            return node;
        }
        if(!at_end() && peek().value == "[")
        {
            consume();
            auto node = std::make_unique<IndexExpr>();
            node->name  = name;
            node->index = parse_expr();
            consume();
            return node;
        }
        auto node = std::make_unique<Identifier>();
        node->name = name;
        return node;
    }
    else if(peek().type == TokenType::PUNCTUATOR && peek().value == "[")
    {
        consume();
        auto node = std::make_unique<ArrayLit>();
        while(!at_end() && !(peek().type == TokenType::PUNCTUATOR && peek().value == "]"))
        {
            node->elements.push_back(parse_expr());
            if(!at_end() && peek().value == ",") consume();
        }
        consume();
        return node;
    }
    else if(peek().type == TokenType::PUNCTUATOR && peek().value == "(")
    {
        consume();
        auto expr = parse_expr();
        consume();
        return expr;
    }

    else if(peek().type == TokenType::PUNCTUATOR && peek().value == "{")
    {
        consume();
        auto node = std::make_unique<MapLit>();
        while(!at_end() && !(peek().type == TokenType::PUNCTUATOR && peek().value == "}"))
        {
            std::string key = consume().value;
            consume();
            auto val = parse_expr();
            node->entries.push_back({key, std::move(val)});
            if(!at_end() && peek().value == ",") consume();
        }
        consume();
        return node;
    }

    else if(peek().type == TokenType::NULL_TOK)
    {
        consume();
        return std::make_unique<NullLit>();  
    }

    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parse_expr()
{
    auto left = parse_primary();

    while(!at_end() && peek().type == TokenType::OPERATOR && (peek().value == "+" || peek().value == "-" || peek().value == "*" || peek().value == "/" || peek().value == "%"))
    {
        auto binop = std::make_unique<BinOp>();
        binop->lhs = std::move(left);
        binop->op  = consume().value;
        binop->rhs = parse_primary();
        left = std::move(binop);
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parse_cond()
{
    auto left = parse_comparison();

    while(!at_end() && peek().type == TokenType::OPERATOR && (peek().value == "&&" || peek().value == "||"))
    {
        auto binop = std::make_unique<BinOp>();
        binop->lhs = std::move(left);
        binop->op  = consume().value;
        binop->rhs = parse_comparison();
        left = std::move(binop);
    }
    return left;
}


std::unique_ptr<ASTNode> Parser::parse_comparison()
{
    auto left = parse_expr();
    if(!at_end() && peek().type == TokenType::OPERATOR && peek().value != "->" && peek().value != ">>" && peek().value != "&&" && peek().value != "||")
    {
        auto binop = std::make_unique<BinOp>();
        binop->lhs = std::move(left);
        binop->op  = consume().value;
        if(at_end()) return binop;
        binop->rhs = parse_expr();
        return binop;
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parse_assign()
{
    std::string name = consume().value;

    if(!at_end() && peek().value == "[")
    {
        consume();
        auto idx = parse_expr();
        consume();
        consume();
        auto node = std::make_unique<IndexAssign>();
        node->name = name;
        node->index = std::move(idx);
        node->value = parse_expr();
        return node;
    }

    if(!at_end() && peek().type == TokenType::OPERATOR && (peek().value == "++" || peek().value == "--"))
    {
        std::string op = consume().value;
        auto node  = std::make_unique<Assign>();
        node->name = name;
        auto binop = std::make_unique<BinOp>();
        auto id = std::make_unique<Identifier>(); id->name = name;
        auto one = std::make_unique<NumberLit>();  one->value = 1.0;
        binop->op  = (op == "++") ? "+" : "-";
        binop->lhs = std::move(id);
        binop->rhs = std::move(one);
        node->value = std::move(binop);
        return node;
    }

    consume();
    if(!at_end() && peek().type == TokenType::INPUT)
    {
        consume();
        auto node = std::make_unique<InputStmt>();
        node->name = name;
        if(!at_end() && peek().type == TokenType::STRING)
            node->prompt = consume().value;
        return node;
    }

    auto node  = std::make_unique<Assign>();
    node->name = name;
    node->value = parse_expr();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_output()
{
    consume(); 
    auto node = std::make_unique<Output>();
    node->value = parse_expr();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_if()
{
    auto node = std::make_unique<IfStmt>();
    consume();
    auto cond = parse_cond();
    auto body = parse_body();
    node->branches.push_back({std::move(cond), std::move(body)});

    while(!at_end() && peek().value == "elif")
    {
        consume();
        auto econd = parse_cond();
        auto ebody = parse_body();
        node->branches.push_back({std::move(econd), std::move(ebody)});
    }
    if(!at_end() && peek().value == "else")
    {
        consume();
        node->else_body = parse_body();
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_repeat()
{
    auto node = std::make_unique<RepeatStmt>();
    consume();
    node->count = parse_expr();
    node->body  = parse_body();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_while()
{
    auto node = std::make_unique<WhileStmt>();
    consume();
    node->condition = parse_cond();
    node->body      = parse_body();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_for()
{
    auto node = std::make_unique<ForStmt>();
    consume();
    node->init = parse_assign();
    consume();
    node->condition = parse_cond();
    consume();
    node->increment = parse_assign();
    node->body = parse_body();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_funcdef()
{
    auto node = std::make_unique<FuncDef>();
    consume();
    node->name = consume().value;
    consume();
    while(!at_end() && peek().value != ")")
    {
        node->params.push_back(consume().value);
        if(!at_end() && peek().value == ",") consume();
    }
    consume();
    node->body = parse_body();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_funccall()
{
    auto node = std::make_unique<FuncCall>();
    node->name = consume().value;
    consume();
    while(!at_end() && peek().value != ")")
    {
        node->args.push_back(parse_expr());
        if(!at_end() && peek().value == ",") consume();
    }
    consume();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_return()
{
    consume();
    auto node = std::make_unique<ReturnStmt>();
    node->value = parse_expr();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_use()
{
    consume();
    auto node = std::make_unique<UseStmt>();
    node->lib = consume().value;
    while(!at_end() && peek().type == TokenType::OPERATOR && peek().value == "/") {
        consume();
        node->lib += "/" + consume().value;
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_try()
{
    consume();
    auto node = std::make_unique<TryStmt>();
    node->body = parse_body();
    
    if(at_end() || peek().value != "catch")
        throw std::runtime_error("try sans catch");
    consume(); 
    
    node->error_var = consume().value;
    
    node->catch_body = parse_body();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_throw()
{
    consume();
    auto node = std::make_unique<ThrowStmt>();
    node->value = parse_expr();
    return node;
}
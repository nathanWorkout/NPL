#include <iostream>
#include "../include/parser.hpp"

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
    if(at_end() || peek().value != "->") throw std::runtime_error("Expected '->'");
    consume();
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

    if(peek().value == "break") {
        consume();
        return std::make_unique<BreakStmt>();
    }
    if(peek().value == "continue") {
        consume();
        return std::make_unique<ContinueStmt>();
    }
    if(peek().value == "fn") return parse_funcdef();

    // force l'interception : Si on a un identifiant suivi d'une flèche (composant racine)
    if(peek().type == TokenType::IDENTIFIER && i + 1 < tokens_.size() &&
      (tokens_[i + 1].value == "->" || tokens_[i + 1].type == TokenType::ARROW))
    {
        auto node = std::make_unique<ExprStatement>();
        node->expr = parse_pipeline();
        return node;
    }

    if(peek().type == TokenType::IDENTIFIER)
    {
        // Regarder si après un éventuel appel de fonction on trouve un "|"
        size_t lookahead = i+1;
        int paren_count = 0;
        bool has_pipe = false;
        while (lookahead < tokens_.size()) {
            if (tokens_[lookahead].value == "(") paren_count++;
            else if (tokens_[lookahead].value == ")") {
                if (paren_count == 0) break;
                paren_count--;
            }
            else if (paren_count == 0 && tokens_[lookahead].value == "=") {
                break;
            }
            else if (paren_count == 0 && tokens_[lookahead].value == "|") {
                has_pipe = true;
                break;
            }
            lookahead++;
        }

        if (has_pipe) {
            auto node = std::make_unique<ExprStatement>();
            node->expr = parse_pipeline();
            return node;
        }
        // Sinon traitement normal
        if(i+1 < tokens_.size() && tokens_[i+1].value == "(") return parse_funccall();
        if(i+1 < tokens_.size() && tokens_[i+1].value == "|") {
            auto node = std::make_unique<ExprStatement>();
            node->expr = parse_pipeline();
            return node;
        }
        return parse_assign();
    }
    if(peek().type == TokenType::OUTPUT) return parse_output();
    if(peek().type == TokenType::RETURN_TOK) return parse_return();
    if(peek().value == "if") return parse_if();

    if(peek().value == "elif" || peek().value == "else") {
        consume();
        return nullptr;
    }

    if(peek().value == "repeat" && !(i > 0 && tokens_[i - 1].value == "|")) {
        return parse_repeat();
    }
    if(peek().value == "while") return parse_while();
    if(peek().value == "for") return parse_for();
    if(peek().value == "use") return parse_use();
    if(peek().value == "try") return parse_try();
    if(peek().value == "throw") return parse_throw();

    auto expr = parse_pipeline();
    if (expr) {
        auto node = std::make_unique<ExprStatement>();
        node->expr = std::move(expr);
        return node;
    }

    consume();
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parse_primary()
{
    if(at_end()) return nullptr;

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

    if(peek().type == TokenType::INPUT) {
        consume();
        auto node = std::make_unique<InputExpr>();
        if(!at_end() && peek().type == TokenType::STRING)
            node->prompt = consume().value;
        return node;
    }
    else if(peek().type == TokenType::BOOL)
    {
        auto node = std::make_unique<BoolLit>();
        node->value = (consume().value == "true");
        return node;
    }

    else if(peek().value == "fn") {
        consume();
        consume();
        auto lambda = std::make_unique<FuncDef>();
        lambda->name = "";
        while(!at_end() && peek().value != ")") {
            lambda->params.push_back(consume().value);
            if(!at_end() && peek().value == ",") consume();
        }
        consume();
        auto raw_body = parse_body();

        if(auto es = dynamic_cast<ExprStatement*>(raw_body.get())) {
            auto block = std::make_unique<Block>();
            auto ret   = std::make_unique<ReturnStmt>();
            ret->value = std::move(es->expr);
            block->statements.push_back(std::move(ret));
            lambda->body = std::move(block);
        } else if(auto fc = dynamic_cast<FuncCall*>(raw_body.get())) {
            auto block = std::make_unique<Block>();
            auto ret   = std::make_unique<ReturnStmt>();
            ret->value = std::move(raw_body);
            block->statements.push_back(std::move(ret));
            lambda->body = std::move(block);
        } else {
            lambda->body = std::move(raw_body);
        }
        return lambda;
    }

    if(peek().type == TokenType::IDENTIFIER && peek().value == "_") {
        consume();
        return std::make_unique<Placeholder>();
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
                node->args.push_back(parse_pipeline());

                if(!at_end() && peek().value == ",")
                    consume();
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
        auto expr = parse_pipeline();
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

std::unique_ptr<ASTNode> Parser::parse_term()
{
    auto left = parse_unary();

    while(!at_end() && peek().type == TokenType::OPERATOR &&
         (peek().value == "*" || peek().value == "/" || peek().value == "%"))
    {
        auto binop = std::make_unique<BinOp>();
        binop->lhs = std::move(left);
        binop->op  = consume().value;
        binop->rhs = parse_unary();
        left = std::move(binop);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parse_expr()
{
    auto left = parse_term();

    while(!at_end() && peek().type == TokenType::OPERATOR &&
         (peek().value == "+" || peek().value == "-"))
    {
        auto binop = std::make_unique<BinOp>();
        binop->lhs = std::move(left);
        binop->op  = consume().value;
        binop->rhs = parse_term();
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
    if(!at_end() && peek().type == TokenType::OPERATOR &&
       peek().value != "->" && peek().value != ">>" &&
       peek().value != "&&" && peek().value != "||" &&
       peek().value != "|")
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
        if(peek().value == "=") consume();
        auto node = std::make_unique<IndexAssign>();
        node->name = name;
        node->index = std::move(idx);
        node->value = parse_pipeline();
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

    if(!at_end() && peek().value == "=") {
        consume();
    }

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
    node->value = parse_pipeline();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_output()
{
    consume();
    auto node = std::make_unique<Output>();
    node->value = parse_pipeline();
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
    auto raw_body = parse_body();

    // Si le corps est une expression sans {} on le met dans un return
    if (auto es = dynamic_cast<ExprStatement*>(raw_body.get())) {
            auto block = std::make_unique<Block>();
            auto ret   = std::make_unique<ReturnStmt>();
            ret->value = std::move(es->expr);
            block->statements.push_back(std::move(ret));
            node->body = std::move(block);
        } else if (auto fc = dynamic_cast<FuncCall*>(raw_body.get())) {
            auto block = std::make_unique<Block>();
            auto ret   = std::make_unique<ReturnStmt>();
            ret->value = std::move(raw_body);
            block->statements.push_back(std::move(ret));
            node->body = std::move(block);
        } else {
            node->body = std::move(raw_body);
        }
        return node;
}

std::unique_ptr<ASTNode> Parser::parse_funccall()
{
    auto node = std::make_unique<FuncCall>();
    node->name = consume().value;
    consume();
    while(!at_end() && peek().value != ")")
    {
        node->args.push_back(parse_pipeline());
        if(!at_end() && peek().value == ",") consume();
    }
    consume();
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_return()
{
    consume();
    auto node = std::make_unique<ReturnStmt>();
    node->value = parse_cond();
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

std::unique_ptr<ASTNode> Parser::parse_unary()
{
    if (!at_end() && peek().type == TokenType::OPERATOR && peek().value == "-")
    {
        consume();
        auto node = std::make_unique<BinOp>();
        auto zero = std::make_unique<NumberLit>();
        zero->value = 0.0;
        node->op = "-";
        node->lhs = std::move(zero);
        node->rhs = parse_unary();
        return node;
    }
    return parse_primary();
}

std::unique_ptr<ASTNode> Parser::parse_component()
{
    // On sauvegarde la position au cas où ce n'est pas un composant
    size_t start_pos = i;

    // Si le token actuel est un identifiant (ex: div, h1, p) et que le suivant est "->"
    if (!at_end() && peek().type == TokenType::IDENTIFIER && (i + 1 < tokens_.size() && tokens_[i + 1].value == "->"))
    {
        // On extrait directement le nom sous forme d'Identifier
        auto id_node = std::make_unique<Identifier>();
        id_node->name = consume().value; // skip le nom (ex: "div")
        consume(); // Mange le "->"

        auto comp_node = std::make_unique<ComponentExpr>();
        comp_node->name = std::move(id_node);

        // Si derrière la flèche il y a un '{', on parse un bloc de sous-composants
        if (!at_end() && peek().type == TokenType::PUNCTUATOR && peek().value == "{") {
            consume();
            auto blk = std::make_unique<Block>();
            while (!at_end() && !(peek().type == TokenType::PUNCTUATOR && peek().value == "}")) {
                blk->statements.push_back(parse_statement());
            }
            consume();
            comp_node->body = std::move(blk);
        } else {
            // Sinon, c'est juste une expression classique
            comp_node->body = parse_cond();
        }
        return comp_node;
    }

    // Si ce n'est pas un composant avec flèche, on reprend le cours normal
    i = start_pos;
    auto left = parse_cond();

    if (!at_end() && peek().type == TokenType::ARROW)
    {
        consume(); // On mange le "->"

        auto comp_node = std::make_unique<ComponentExpr>();
        comp_node->name = std::move(left);

        if (!at_end() && peek().type == TokenType::PUNCTUATOR && peek().value == "{") {
            consume();
            auto blk = std::make_unique<Block>();
            while (!at_end() && !(peek().type == TokenType::PUNCTUATOR && peek().value == "}")) {
                blk->statements.push_back(parse_statement());
            }
            consume();
            comp_node->body = std::move(blk);
        } else {
            comp_node->body = parse_cond();
        }
        return comp_node;
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parse_pipeline()
{
    auto left = parse_component();

    while(!at_end() && peek().type == TokenType::OPERATOR && peek().value == "|")
    {
        consume();

        auto pipe = std::make_unique<PipeExpr>();
        pipe->lhs = std::move(left);

        if(!at_end() && peek().value == "{")
        {
            consume();
            auto lambda = std::make_unique<LambdaBlock>();
            auto blk = std::make_unique<Block>();
            while(!at_end() && peek().value != "}")
                blk->statements.push_back(parse_statement());
            consume();
            lambda->body = std::move(blk);
            pipe->rhs = std::move(lambda);
        }
        else if(
            peek().type == TokenType::IDENTIFIER ||
            peek().value == "repeat"
        )
        {
            // Si le prochian token est (
            if (i+1 < tokens_.size() && tokens_[i+1].value == "(") {
                // Appel de fonction normal avec parenthèses
                auto call = parse_funccall();
                pipe->rhs = std::move(call);
            } else {
                // Pas de parenthèses :
                std::string fname = consume().value;

                auto call = std::make_unique<FuncCall>();
                call->name = fname;

                // Vérifier si un lambda (ex filter)
                if(!at_end() && peek().value == "{") {

                    consume();

                    auto lambda = std::make_unique<LambdaBlock>();
                    auto blk = std::make_unique<Block>();

                    while(!at_end() && peek().value != "}")
                        blk->statements.push_back(parse_statement());

                    consume();

                    lambda->body = std::move(blk);

                    call->args.push_back(std::move(lambda));
                }

                // argument sans parenthèses
                while(
                    !at_end() &&
                    peek().value != "|" &&
                    peek().value != "}" &&
                    peek().value != ")"
                ) {
                    call->args.push_back(parse_cond());

                    if(!at_end() && peek().value == ",")
                        consume();
                    else
                        break;
                }

                pipe->rhs = std::move(call);
            }
        }
        else if(peek().type == TokenType::OUTPUT)
        {
            consume();
            auto out = std::make_unique<Output>();
            // si une expression suit le >>
            if(!at_end() && peek().type != TokenType::OPERATOR && peek().value != "|")
                out->value = parse_cond();
            pipe->rhs = std::move(out);
        }

        left = std::move(pipe);
    }
    return left;
}

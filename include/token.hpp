#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    STRING,
    OPERATOR,
    PUNCTUATOR,
    ARROW,
    OUTPUT,
    PIPE,
    BOOL,
    RETURN_TOK,

    UNKNOWN
};

inline std::string token_type_str(TokenType t) {
    switch(t) {
        case TokenType::KEYWORD:    return "KEYWORD";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER:     return "NUMBER";
        case TokenType::STRING:     return "STRING";
        case TokenType::OPERATOR:   return "OPERATOR";
        case TokenType::PUNCTUATOR: return "PUNCTUATOR";
        case TokenType::BOOL:       return "BOOL";
        case TokenType::OUTPUT: return "OUTPUT";
        case TokenType::ARROW:  return "ARROW";
        case TokenType::PIPE:   return "PIPE";
        default:                    return "UNKNOWN";
    }
}

struct Token {
    TokenType type;
    std::string value;
};

int lexer(const std::string& src, Token* tokens, int max_tokens);
void print_tokens(Token* tokens, int count);

#endif
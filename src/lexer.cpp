#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include "token.hpp"
#include "lexer.hpp"

std::vector<std::string> keywords =
{
    "if", "elif", "else", "while", "for", "repeat", "try", "catch", "use"
};
std::vector<std::string> operators =
{
    "+", "-", "*", "/", "=", "==", "!=", "<=", ">=", "<", ">", "->", ">>", "<<", "?>", "|", "&", "++", "--", "%"
};
std::vector<std::string> punctuators =
{
    ",", ";", "(", ")", "{", "}", "[", "]"
};

bool is_keyword(const std::string& word)
{
    for(const std::string& kw : keywords)
        if(word == kw) return true;
    return false;
}

bool is_operator(const std::string& op)
{
    for(const std::string& o : operators)
        if(op == o) return true;
    return false;
}
bool is_ponctuator(const std::string& punc)
{
    for(const std::string& pc : punctuators)
        if(punc == pc) return true;
    return false;
}

std::vector<Token> tokenize(const std::string& src)
{
    std::vector<Token> tokens;
    size_t i = 0;

    while(i < src.size())
    {
        if(src[i] == ' ' || src[i] == '\n' || src[i] == '\t' || src[i] == '\r')
        {
            i++;
            continue;
        }
        else if(src[i] == '#')
        {
            while(i < src.size() && src[i] != '\n') i++;
            continue;
        }
        else if(src[i] == '"')
        {
            i++;
            std::string value;
            while(i < src.size() && src[i] != '"')
            {
                if(src[i] == '\\' && i+1 < src.size())
                {
                    char esc = src[++i];
                    if(esc == 'n')  value += '\n';
                    else if(esc == 't') value += '\t';
                    else value += esc;
                }
                else value += src[i];
                i++;
            }
            i++; 
            tokens.push_back({TokenType::STRING, value});
            continue;
        }
        else if(isdigit(src[i]))
        {
            std::string value;
            while(i < src.size() && isdigit(src[i]))
                value += src[i++];
            if(i < src.size() && src[i] == '.' && i+1 < src.size() && isdigit(src[i+1]))
            {
                value += src[i++]; // '.'
                while(i < src.size() && isdigit(src[i]))
                    value += src[i++];
            }
            tokens.push_back({TokenType::NUMBER, value});
            continue;
        }
        else if(isalpha(src[i]) || src[i] == '_')
        {
            std::string value;
            while(i < src.size() && (isalnum(src[i]) || src[i] == '_'))
                value += src[i++];
            if(value == "true" || value == "false")
                tokens.push_back({TokenType::BOOL, value});
            else if(is_keyword(value))
                tokens.push_back({TokenType::KEYWORD, value});
            else
                tokens.push_back({TokenType::IDENTIFIER, value});
            continue;
        }
        else
        {
            std::string two = (i+1 < src.size()) ? src.substr(i, 2) : "";
            if(two == ">>") { tokens.push_back({TokenType::OUTPUT, two}); i += 2; continue; }
            if(two == "->") { tokens.push_back({TokenType::ARROW,  two}); i += 2; continue; }
            if(two == "<<") { tokens.push_back({TokenType::RETURN_TOK, two}); i += 2; continue; }
            if(two == "?>") { tokens.push_back({TokenType::INPUT, two}); i += 2; continue; }
            if(is_operator(two))   { tokens.push_back({TokenType::OPERATOR,   two}); i += 2; continue; }
            std::string one = src.substr(i, 1);
            if(is_operator(one))   { tokens.push_back({TokenType::OPERATOR,   one}); i++; continue; }
            if(is_ponctuator(one)) { tokens.push_back({TokenType::PUNCTUATOR, one}); i++; continue; }
            i++;
        }
    }

    return tokens;
}

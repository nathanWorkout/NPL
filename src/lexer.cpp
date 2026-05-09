#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include "token.hpp"
#include "lexer.hpp"

std::vector<std::string> keywords =
{
    "if", "elif", "else", "while", "for", "repeat", "try", "catch"
};

std::vector<std::string> operators =
{
    "+", "-", "*", "/", "=", "==", "!=", "<=", ">=", "<", ">", "->", ">>", "|", "&", "++", "--"
};

std::vector<std::string> punctuators =
{
    ",", ";", "(", ")", "{", "}", "[", "]"
};

bool is_keyword(const std::string& word)
{
    for(const std::string& kw : keywords) {
        if(word == kw)
        {
            return true;
        }
    }

    return false;
}

bool is_operator(const std::string& op)
{
    for(const std::string& o : operators)
    {
        if(op == o)
        {
            return true;
        }
    }

    return false;
}

bool is_ponctuator(const std::string& punc)
{
    for(const std::string& pc : punctuators)
    {
        if(punc == pc)
        {
            return true;
        }
    }

    return false;
}

std::vector<Token> tokenize(const std::string& src)
{
    std::vector<Token> tokens;
    size_t i = 0;
    
    while(i < src.size())
    {
        if(src[i] == ' ' || src[i] == '\n' || src[i] == '\t')
        {
            i++;
            continue;
        }

        else if(src[i] == '"')
        {
            i++;
            std::string value = "";
            while(src[i] != '"')
            {
                value += src[i++];
            }
            i++;
            tokens.push_back({TokenType::STRING, value});
            continue;
        }

        else if(isdigit(src[i]))
        {
            std::string value = "";
            while(i < src.size() && isdigit(src[i]))
            {
                value += src[i++];
            }
            tokens.push_back({TokenType::NUMBER, value});
            continue;
        }

        else if(isalpha(src[i]) || src[i] == '_')
        {
            std::string value = "";
            while(i < src.size() && (isalnum(src[i]) || src[i] == '_'))
            {
                value += src[i];
                i++;
            }
            if(value == "true" || value == "false")
            {
                tokens.push_back({TokenType::BOOL, value});
            }
            else if(is_keyword(value))
            {
                tokens.push_back({TokenType::KEYWORD, value});
            }
            else
            {
                tokens.push_back({TokenType::IDENTIFIER, value});
            }
            continue;
        }

        else
        {
            std::string two = src.substr(i, 2);
            if(is_operator(two))
            {
                tokens.push_back({TokenType::OPERATOR, two});
                i += 2;
                continue;
            }

            std::string one = src.substr(i, 1);
            if(is_operator(one))
            {
                tokens.push_back({TokenType::OPERATOR, one});
                i++;
                continue;
            }

            if(is_ponctuator(one))
            {
                tokens.push_back({TokenType::PUNCTUATOR, one});
                i++;
                continue;
            }

            i++;
        }
    }
    
    return tokens;
}
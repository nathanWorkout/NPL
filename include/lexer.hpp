#ifndef LEXER_HPP
#define LEXER_HPP

#include "token.hpp"
#include <string>
#include <vector>

std::vector<Token> tokenize(const std::string& src);

#endif
#pragma once
#include "token.h"
#include <vector>
#include <string>

class Lexer {
    std::string src;
    size_t pos = 0;
    int currentLine = 1;
    int currentColumn = 1;

    char peek() const;
    char peekNext() const;
    char advance();
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    Token lexIdentifier();
    Token lexNumber();
    Token lexString();

public:
    Lexer(const std::string &s);
    std::vector<Token> tokenize();
};

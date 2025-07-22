#pragma once
#include <string>

enum class TokenType {
    // Keywords
    IF, ELSE, WHILE, FOR, FUN, RETURN, VOID,
    
    // Types and literals
    INT, FLOAT, STRING, IDENT,
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL,

    // Operators
    ASSIGN, COLON_ASSIGN,  // = and :=
    EQ, NEQ, LT, LE, GT, GE,  // Comparison operators
    AND, OR,  // Logical operators
    INCREMENT, DECREMENT,  // ++ and --
    PLUS, MINUS, MUL, DIV,

    // Punctuation
    PRINT, SEMICOLON, LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET, COMMA, EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    Token(TokenType t, const std::string &l, int ln = 1, int col = 1) 
        : type(t), lexeme(l), line(ln), column(col) {}
};

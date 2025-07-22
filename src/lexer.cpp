#include "../include/lexer.h"
#include <cctype>
#include <iostream>

Lexer::Lexer(const std::string &s) : src(s) {}

// Extrae el caracter actual sin avanzar la posición
char Lexer::peek() const { 
    return pos < src.size() ? src[pos] : '\0'; 
}
// Extrae el siguiente caracter sin avanzar la posición
char Lexer::peekNext() const { 
    return pos+1 < src.size() ? src[pos+1] : '\0'; 
}
// Avanza a la siguiente posición y devuelve el caracter actual
char Lexer::advance() { 
    char c = src[pos++]; 
    if (c == '\n') {
        currentLine++;
        currentColumn = 1;
    } else {
        currentColumn++;
    }
    return c;
}

void Lexer::skipWhitespace() { 
    while (isspace(peek())) {
        advance();
    }
}

void Lexer::skipLineComment() {
    while (peek() && peek() != '\n') pos++;
}

void Lexer::skipBlockComment() {
    pos += 2; // skip /*
    while (peek() && !(peek() == '*' && peekNext() == '/')) pos++;
    if (peek()) pos += 2;
}

Token Lexer::lexIdentifier() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t start = pos;
    while (isalnum(peek()) || peek() == '_') advance();
    std::string text = src.substr(start, pos - start);
    
    if (text == "int")    return Token(TokenType::INT, text, startLine, startColumn);
    if (text == "float")  return Token(TokenType::FLOAT, text, startLine, startColumn);
    if (text == "string") return Token(TokenType::STRING, text, startLine, startColumn);
    if (text == "print")  return Token(TokenType::PRINT, text, startLine, startColumn);
    if (text == "if")     return Token(TokenType::IF, text, startLine, startColumn);
    if (text == "else")   return Token(TokenType::ELSE, text, startLine, startColumn);
    if (text == "while")  return Token(TokenType::WHILE, text, startLine, startColumn);
    if (text == "for")    return Token(TokenType::FOR, text, startLine, startColumn);
    if (text == "fun")    return Token(TokenType::FUN, text, startLine, startColumn);
    if (text == "return") return Token(TokenType::RETURN, text, startLine, startColumn);
    if (text == "void")   return Token(TokenType::VOID, text, startLine, startColumn);
    
    return Token(TokenType::IDENT, text, startLine, startColumn);
}

Token Lexer::lexNumber() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t start = pos;
    while (isdigit(peek())) advance();
    if (peek() == '.' && isdigit(peekNext())) {
        advance(); // skip '.'
        while (isdigit(peek())) advance();
        return Token(TokenType::FLOAT_LITERAL, src.substr(start, pos - start), startLine, startColumn);
    }
    return Token(TokenType::INT_LITERAL, src.substr(start, pos - start), startLine, startColumn);
}

Token Lexer::lexString() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    advance(); // skip "
    size_t start = pos;
    while (peek() && peek() != '"') advance();
    std::string text = src.substr(start, pos - start);
    if (peek() == '"') advance();
    return Token(TokenType::STRING_LITERAL, text, startLine, startColumn);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < src.size()) {
        skipWhitespace();
        char c = peek();
        if (c == '\0') break;

        // Handle comments
        if (c == '/' && peekNext() == '/') {
            skipLineComment();
            continue;
        }
        if (c == '/' && peekNext() == '*') {
            skipBlockComment();
            continue;
        }

        // Multi-character operators
        if (c == ':' && peekNext() == '=') {
            tokens.emplace_back(TokenType::COLON_ASSIGN, ":=", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '=' && peekNext() == '=') {
            tokens.emplace_back(TokenType::EQ, "==", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '!' && peekNext() == '=') {
            tokens.emplace_back(TokenType::NEQ, "!=", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '<' && peekNext() == '=') {
            tokens.emplace_back(TokenType::LE, "<=", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '>' && peekNext() == '=') {
            tokens.emplace_back(TokenType::GE, ">=", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '&' && peekNext() == '&') {
            tokens.emplace_back(TokenType::AND, "&&", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '|' && peekNext() == '|') {
            tokens.emplace_back(TokenType::OR, "||", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '+' && peekNext() == '+') {
            tokens.emplace_back(TokenType::INCREMENT, "++", currentLine, currentColumn);
            advance(); advance();
            continue;
        }
        if (c == '-' && peekNext() == '-') {
            tokens.emplace_back(TokenType::DECREMENT, "--", currentLine, currentColumn);
            advance(); advance();
            continue;
        }

        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            tokens.push_back(lexIdentifier());
            continue;
        }

        // Numbers
        if (isdigit(c)) {
            tokens.push_back(lexNumber());
            continue;
        }

        // Strings
        if (c == '"') {
            tokens.push_back(lexString());
            continue;
        }

        // Single-character operators - check division before other operators
        if (c == '/' && peekNext() != '/' && peekNext() != '*') {
            tokens.emplace_back(TokenType::DIV, "/", currentLine, currentColumn);
            advance();
            continue;
        }
        if (c == '=' && peekNext() != '=') { 
            tokens.emplace_back(TokenType::ASSIGN, "=", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '+' && peekNext() != '+') { 
            tokens.emplace_back(TokenType::PLUS, "+", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '-' && peekNext() != '-') { 
            tokens.emplace_back(TokenType::MINUS, "-", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '*') { 
            tokens.emplace_back(TokenType::MUL, "*", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '<' && peekNext() != '=') { 
            tokens.emplace_back(TokenType::LT, "<", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '>' && peekNext() != '=') { 
            tokens.emplace_back(TokenType::GT, ">", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == ';') { 
            tokens.emplace_back(TokenType::SEMICOLON, ";", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '(') { 
            tokens.emplace_back(TokenType::LPAREN, "(", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == ')') { 
            tokens.emplace_back(TokenType::RPAREN, ")", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '{') { 
            tokens.emplace_back(TokenType::LBRACE, "{", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '}') { 
            tokens.emplace_back(TokenType::RBRACE, "}", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == '[') { 
            tokens.emplace_back(TokenType::LBRACKET, "[", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == ']') { 
            tokens.emplace_back(TokenType::RBRACKET, "]", currentLine, currentColumn);
            advance();
            continue; 
        }
        if (c == ',') { 
            tokens.emplace_back(TokenType::COMMA, ",", currentLine, currentColumn);
            advance();
            continue; 
        }

        advance(); // skip unknown
    }
    tokens.emplace_back(TokenType::EOF_TOKEN, "");
    return tokens;
}

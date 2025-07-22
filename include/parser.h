#pragma once
#include "token.h"
#include "ast.h"
#include <vector>

class Parser {
    const std::vector<Token> &tokens;
    size_t pos = 0;
    
    const Token &peek() const;
    const Token &advance();
    bool match(TokenType t);
    bool check(TokenType t) const;
    void expect(TokenType t, const std::string &msg);
    
    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseRelational();
    ExprPtr parseAddSub();
    ExprPtr parseMulDiv();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();
    ExprPtr parseArray();
    
    std::vector<StmtPtr> parseBlock();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseForInitOrDecl();
    StmtPtr parseForPost();
    StmtPtr parseFunction(bool inference);
    StmtPtr parseStatement();

public:
    Parser(const std::vector<Token> &toks);
    std::vector<StmtPtr> parse();
};

#include "../include/parser.h"
#include <iostream>
#include <cstdlib>

Parser::Parser(const std::vector<Token> &toks) : tokens(toks) {}

const Token &Parser::peek() const { 
    return tokens[pos]; 
}

const Token &Parser::advance() { 
    return tokens[pos++]; 
}

bool Parser::match(TokenType t) {
    if (peek().type == t) { 
        advance(); 
        return true; 
    }
    return false;
}

bool Parser::check(TokenType t) const {
    return peek().type == t;
}

void Parser::expect(TokenType t, const std::string &msg) {
    if (!match(t)) {
        std::cerr << "Parse error: " << msg << " en '" << peek().lexeme << "'\n";
        std::exit(1);
    }
}

ExprPtr Parser::parseExpression() {
    return parseOr();
}

ExprPtr Parser::parseOr() {
    ExprPtr expr = parseAnd();
    while (match(TokenType::OR)) {
        ExprPtr right = parseAnd();
        expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::OR, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseAnd() {
    ExprPtr expr = parseEquality();
    while (match(TokenType::AND)) {
        ExprPtr right = parseEquality();
        expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::AND, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseRelational();
    while (true) {
        if (match(TokenType::EQ)) {
            ExprPtr right = parseRelational();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::EQ, std::move(expr), std::move(right));
        } else if (match(TokenType::NEQ)) {
            ExprPtr right = parseRelational();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::NEQ, std::move(expr), std::move(right));
        } else break;
    }
    return expr;
}

ExprPtr Parser::parseRelational() {
    ExprPtr expr = parseAddSub();
    while (true) {
        if (match(TokenType::LT)) {
            ExprPtr right = parseAddSub();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::LT, std::move(expr), std::move(right));
        } else if (match(TokenType::LE)) {
            ExprPtr right = parseAddSub();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::LE, std::move(expr), std::move(right));
        } else if (match(TokenType::GT)) {
            ExprPtr right = parseAddSub();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::GT, std::move(expr), std::move(right));
        } else if (match(TokenType::GE)) {
            ExprPtr right = parseAddSub();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::GE, std::move(expr), std::move(right));
        } else break;
    }
    return expr;
}

ExprPtr Parser::parseAddSub() {
    ExprPtr expr = parseMulDiv();
    while (true) {
        if (match(TokenType::PLUS)) {
            ExprPtr right = parseMulDiv();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::ADD, std::move(expr), std::move(right));
        } else if (match(TokenType::MINUS)) {
            ExprPtr right = parseMulDiv();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::SUB, std::move(expr), std::move(right));
        } else break;
    }
    return expr;
}

ExprPtr Parser::parseMulDiv() {
    ExprPtr expr = parseUnary();
    while (true) {
        if (match(TokenType::MUL)) {
            ExprPtr right = parseUnary();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::MUL, std::move(expr), std::move(right));
        } else if (match(TokenType::DIV)) {
            ExprPtr right = parseUnary();
            expr = std::make_unique<BinaryExpr>(BinaryExpr::Op::DIV, std::move(expr), std::move(right));
        } else break;
    }
    return expr;
}

ExprPtr Parser::parseUnary() {
    if (match(TokenType::INCREMENT)) {
        expect(TokenType::IDENT, "se esperaba identificador después de ++");
        std::string varName = tokens[pos-1].lexeme;
        return std::make_unique<UnaryExpr>(UnaryExpr::Op::PRE_INC, varName);
    }
    if (match(TokenType::DECREMENT)) {
        expect(TokenType::IDENT, "se esperaba identificador después de --");
        std::string varName = tokens[pos-1].lexeme;
        return std::make_unique<UnaryExpr>(UnaryExpr::Op::PRE_DEC, varName);
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    if (match(TokenType::INT_LITERAL))
        return std::make_unique<LiteralExpr>(std::stoi(tokens[pos-1].lexeme));
    if (match(TokenType::FLOAT_LITERAL))
        return std::make_unique<LiteralExpr>(std::stof(tokens[pos-1].lexeme));
    if (match(TokenType::STRING_LITERAL))
        return std::make_unique<LiteralExpr>(tokens[pos-1].lexeme);
    if (match(TokenType::IDENT)) {
        Token identToken = tokens[pos-1];
        std::string name = identToken.lexeme;
        // Check for function call
        if (match(TokenType::LPAREN)) {
            std::vector<ExprPtr> args;
            if (!match(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
                expect(TokenType::RPAREN, "se esperaba ')'");
            }
            return std::make_unique<CallExpr>(name, std::move(args));
        }
        // Check for array indexing
        if (match(TokenType::LBRACKET)) {
            ExprPtr index = parseExpression();
            expect(TokenType::RBRACKET, "se esperaba ']'");
            return std::make_unique<IndexExpr>(
                std::make_unique<VarExpr>(name, identToken.line, identToken.column), 
                std::move(index));
        }
        return std::make_unique<VarExpr>(name, identToken.line, identToken.column);
    }
    if (match(TokenType::LPAREN)) {
        ExprPtr e = parseExpression();
        expect(TokenType::RPAREN, "se esperaba ')'");
        return e;
    }
    if (match(TokenType::LBRACKET)) {
        return parseArray();
    }
    std::cerr << "Parse error: expresión inesperada\n";
    std::exit(1);
}

ExprPtr Parser::parseArray() {
    std::vector<ExprPtr> elements;
    
    if (!match(TokenType::RBRACKET)) {
        do {
            ExprPtr element = parseExpression();
            
            // Check if element is an array (this would make it a 2D matrix)
            if (dynamic_cast<const ArrayExpr*>(element.get())) {
                std::cerr << "Error: Matrices 2D no están soportadas. Solo se permiten arrays 1D.\n";
                std::exit(1);
            }
            
            elements.push_back(std::move(element));
        } while (match(TokenType::COMMA));
        expect(TokenType::RBRACKET, "se esperaba ']'");
    }
    
    return std::make_unique<ArrayExpr>(std::move(elements));
}

std::vector<StmtPtr> Parser::parseBlock() {
    expect(TokenType::LBRACE, "se esperaba '{'");
    std::vector<StmtPtr> stmts;
    while (!match(TokenType::RBRACE) && peek().type != TokenType::EOF_TOKEN) {
        if (auto stmt = parseStatement()) {
            stmts.push_back(std::move(stmt));
        }
    }
    return stmts;
}

StmtPtr Parser::parseIf() {
    expect(TokenType::LPAREN, "se esperaba '(' tras if");
    ExprPtr cond = parseExpression();
    expect(TokenType::RPAREN, "se esperaba ')'");
    auto thenBranch = parseBlock();
    std::vector<StmtPtr> elseBranch;
    if (match(TokenType::ELSE)) {
        elseBranch = parseBlock();
    }
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBranch), std::move(elseBranch));
}

StmtPtr Parser::parseWhile() {
    expect(TokenType::LPAREN, "se esperaba '(' tras while");
    ExprPtr cond = parseExpression();
    expect(TokenType::RPAREN, "se esperaba ')'");
    auto body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

StmtPtr Parser::parseFor() {
    expect(TokenType::LPAREN, "se esperaba '(' tras for");
    StmtPtr init = nullptr;
    if (!match(TokenType::SEMICOLON)) {
        init = parseForInitOrDecl();
        expect(TokenType::SEMICOLON, "se esperaba ';'");
    }
    ExprPtr cond = nullptr;
    if (!match(TokenType::SEMICOLON)) {
        cond = parseExpression();
        expect(TokenType::SEMICOLON, "se esperaba ';'");
    }
    StmtPtr post = nullptr;
    if (!match(TokenType::RPAREN)) {
        post = parseForPost();
        expect(TokenType::RPAREN, "se esperaba ')'");
    }
    auto body = parseBlock();
    return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
}

StmtPtr Parser::parseForInitOrDecl() {
    if (peek().type == TokenType::INT || peek().type == TokenType::STRING || peek().type == TokenType::FLOAT) {
        TokenType typeToken = advance().type;
        VarDeclStmt::Kind type;
        if (typeToken == TokenType::INT) type = VarDeclStmt::INT;
        else if (typeToken == TokenType::FLOAT) type = VarDeclStmt::FLOAT;
        else type = VarDeclStmt::STRING;
        
        expect(TokenType::IDENT, "se esperaba identificador");
        std::string name = tokens[pos-1].lexeme;
        expect(TokenType::ASSIGN, "se esperaba '='");
        ExprPtr init = parseExpression();
        
        // Check if the expression is an array and adjust type accordingly
        if (dynamic_cast<const ArrayExpr*>(init.get())) {
            if (type == VarDeclStmt::INT) type = VarDeclStmt::INT_ARRAY;
            else if (type == VarDeclStmt::FLOAT) type = VarDeclStmt::FLOAT_ARRAY;
            else if (type == VarDeclStmt::STRING) type = VarDeclStmt::STRING_ARRAY;
        }
        
        return std::make_unique<VarDeclStmt>(type, name, std::move(init));
    }
    if (peek().type == TokenType::IDENT && pos+1 < tokens.size()
        && tokens[pos+1].type == TokenType::COLON_ASSIGN) {
        std::string name = advance().lexeme;
        advance(); // :=
        ExprPtr init = parseExpression();
        return std::make_unique<InferDeclStmt>(name, std::move(init));
    }
    // expresión como stmt
    ExprPtr e = parseExpression();
    return std::make_unique<ExprStmt>(std::move(e));
}

StmtPtr Parser::parseForPost() {
    if (peek().type == TokenType::INCREMENT || peek().type == TokenType::DECREMENT) {
        ExprPtr e = parseUnary();
        return std::make_unique<ExprStmt>(std::move(e));
    }
    if (peek().type == TokenType::IDENT && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::ASSIGN) {
        Token identToken = advance();
        std::string name = identToken.lexeme;
        advance(); // =
        ExprPtr expr = parseExpression();
        return std::make_unique<AssignStmt>(name, std::move(expr), identToken.line, identToken.column);
    }
    ExprPtr e = parseExpression();
    return std::make_unique<ExprStmt>(std::move(e));
}

StmtPtr Parser::parseFunction(bool inference) {
    VarDeclStmt::Kind retType = VarDeclStmt::VOID; // valor por defecto
    if (!inference) {
        // función con tipo explícito: int/float/string/void function_name(...)
        if (tokens[pos-1].type == TokenType::INT) retType = VarDeclStmt::INT;
        else if (tokens[pos-1].type == TokenType::FLOAT) retType = VarDeclStmt::FLOAT;
        else if (tokens[pos-1].type == TokenType::STRING) retType = VarDeclStmt::STRING;
        else retType = VarDeclStmt::VOID;
        
        // Verificar si hay [] después del tipo (bloquear arrays como retorno)
        if (check(TokenType::LBRACKET)) {
            std::cerr << "Error: Las funciones no pueden retornar arrays. Use elementos individuales en su lugar.\n";
            std::exit(1);
        }
    }

    expect(TokenType::IDENT, "se esperaba nombre de función");
    std::string name = tokens[pos-1].lexeme;

    expect(TokenType::LPAREN, "se esperaba '('");
    std::vector<std::string> params;
    std::vector<bool> paramIsArray;
    if (!match(TokenType::RPAREN)) {
        do {
            expect(TokenType::IDENT, "se esperaba parámetro");
            std::string paramName = tokens[pos-1].lexeme;
            params.push_back(paramName);
            
            // Verificar si hay [] después del nombre del parámetro
            bool isArray = false;
            if (match(TokenType::LBRACKET)) {
                expect(TokenType::RBRACKET, "se esperaba ']' después de '['");
                isArray = true;
            }
            paramIsArray.push_back(isArray);
        } while (match(TokenType::COMMA));
        expect(TokenType::RPAREN, "se esperaba ')'");
    }

    auto body = parseBlock();
    
    // Asegurar que paramIsArray tenga el mismo tamaño que params
    while (paramIsArray.size() < params.size()) {
        paramIsArray.push_back(false);
    }
    
    return std::make_unique<FunctionStmt>(inference, retType, name, std::move(params), std::move(paramIsArray), std::move(body));
}

StmtPtr Parser::parseStatement() {
    // Skip empty statements (just semicolons)
    if (match(TokenType::SEMICOLON)) {
        return nullptr;
    }

    // Functions
    if (match(TokenType::FUN)) {
        return parseFunction(true); // inference function
    }
    
    // Explicit function declaration
    if ((peek().type == TokenType::INT || peek().type == TokenType::FLOAT || 
         peek().type == TokenType::STRING || peek().type == TokenType::VOID) &&
        pos + 2 < tokens.size() && tokens[pos+1].type == TokenType::IDENT &&
        tokens[pos+2].type == TokenType::LPAREN) {
        advance(); // consume type
        return parseFunction(false); // explicit function
    }

    // Return statements
    if (match(TokenType::RETURN)) {
        ExprPtr value = nullptr;
        if (!match(TokenType::SEMICOLON)) {
            value = parseExpression();
            expect(TokenType::SEMICOLON, "se esperaba ';'");
        }
        return std::make_unique<ReturnStmt>(std::move(value));
    }

    // Control flow statements
    if (match(TokenType::IF)) return parseIf();
    if (match(TokenType::WHILE)) return parseWhile();
    if (match(TokenType::FOR)) return parseFor();

    // Type inference with :=
    if (peek().type == TokenType::IDENT && pos + 1 < tokens.size()
        && tokens[pos + 1].type == TokenType::COLON_ASSIGN) {
        std::string name = advance().lexeme;
        advance(); // :=
        ExprPtr init = parseExpression();
        expect(TokenType::SEMICOLON, "se esperaba ';'");
        return std::make_unique<InferDeclStmt>(name, std::move(init));
    }

    // Explicit type declaration
    if (match(TokenType::INT) || match(TokenType::FLOAT) || match(TokenType::STRING)) {
        VarDeclStmt::Kind type;
        if (tokens[pos-1].type == TokenType::INT) type = VarDeclStmt::INT;
        else if (tokens[pos-1].type == TokenType::FLOAT) type = VarDeclStmt::FLOAT;
        else type = VarDeclStmt::STRING;

        expect(TokenType::IDENT, "se esperaba identificador");
        std::string name = tokens[pos-1].lexeme;
        expect(TokenType::ASSIGN, "se esperaba '='");
        ExprPtr init = parseExpression();
        
        // Check if the expression is an array and adjust type accordingly
        if (dynamic_cast<const ArrayExpr*>(init.get())) {
            if (type == VarDeclStmt::INT) type = VarDeclStmt::INT_ARRAY;
            else if (type == VarDeclStmt::FLOAT) type = VarDeclStmt::FLOAT_ARRAY;
            else if (type == VarDeclStmt::STRING) type = VarDeclStmt::STRING_ARRAY;
        }
        
        expect(TokenType::SEMICOLON, "se esperaba ';'");
        return std::make_unique<VarDeclStmt>(type, name, std::move(init));
    }

    // Assignment
    if (peek().type == TokenType::IDENT && pos + 1 < tokens.size()
        && tokens[pos + 1].type == TokenType::ASSIGN) {
        Token identToken = advance();
        std::string name = identToken.lexeme;
        advance(); // =
        ExprPtr expr = parseExpression();
        expect(TokenType::SEMICOLON, "se esperaba ';'");
        return std::make_unique<AssignStmt>(name, std::move(expr), identToken.line, identToken.column);
    }

    // Print statement
    if (match(TokenType::PRINT)) {
        Token printToken = tokens[pos-1];
        expect(TokenType::LPAREN, "se esperaba '('");
        ExprPtr e = parseExpression();
        expect(TokenType::RPAREN, "se esperaba ')'");
        expect(TokenType::SEMICOLON, "se esperaba ';'");
        return std::make_unique<PrintStmt>(std::move(e), printToken.line, printToken.column);
    }

    // Expression statements (like ++var, --var)
    if (peek().type == TokenType::INCREMENT || peek().type == TokenType::DECREMENT) {
        ExprPtr e = parseUnary();
        expect(TokenType::SEMICOLON, "se esperaba ';'");
        return std::make_unique<ExprStmt>(std::move(e));
    }

    // Function call as statement
    if (peek().type == TokenType::IDENT && pos + 1 < tokens.size()
        && tokens[pos + 1].type == TokenType::LPAREN) {
        ExprPtr e = parseExpression();
        expect(TokenType::SEMICOLON, "se esperaba ';'");
        return std::make_unique<ExprStmt>(std::move(e));
    }

    if (match(TokenType::EOF_TOKEN)) return nullptr;

    std::cerr << "Parse error: sentencia inesperada en token '" << peek().lexeme
              << "' de tipo " << static_cast<int>(peek().type) << "\n";
    std::exit(1);
}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;
    while (peek().type != TokenType::EOF_TOKEN) {
        if (auto stmt = parseStatement()) {
            stmts.push_back(std::move(stmt));
        }
    }
    return stmts;
}

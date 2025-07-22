#pragma once
#include "ast.h"
#include <map>
#include <vector>

class TypeAnalyzer {
    std::vector<std::map<std::string, VarDeclStmt::Kind>> scopes;
    std::map<std::string, VarDeclStmt::Kind> functions; // function name -> return type
    std::map<std::string, std::vector<VarDeclStmt::Kind>> functionParams; // function name -> parameter types

public:
    TypeAnalyzer();
    
    void pushScope();
    void popScope();
    
    VarDeclStmt::Kind inferType(const Expr *expr);
    
    void declareFunction(const std::string &name, VarDeclStmt::Kind returnType);
    void setFunctionParams(const std::string &name, const std::vector<VarDeclStmt::Kind> &paramTypes);
    std::vector<VarDeclStmt::Kind> getFunctionParams(const std::string &name);
    bool hasFunction(const std::string &name);
    
    void declareVariable(const std::string &name, VarDeclStmt::Kind type);
    VarDeclStmt::Kind getVariableType(const std::string &name);
    bool hasVariable(const std::string &name);
    
    // Analyze function calls to infer parameter types
    void analyzeCallExpr(const CallExpr *call);
    void analyzeExpression(const Expr *expr);
    void analyzeStatement(const Stmt *stmt);

    // Helper function to detect if a function has explicit return statements
    bool hasExplicitReturn(const std::vector<StmtPtr> &body);
    VarDeclStmt::Kind getFunctionReturnType(const std::string &name);
    
    // Validate function scopes to detect invalid variable access
    void validateFunctionScopes(const FunctionStmt *funcStmt);
    void validateExpressionInFunctionScope(const Expr *expr, const std::map<std::string, VarDeclStmt::Kind> &localVars, const std::string &functionName);
    void validateStatementInFunctionScope(const Stmt *stmt, const std::map<std::string, VarDeclStmt::Kind> &localVars, const std::string &functionName);
};

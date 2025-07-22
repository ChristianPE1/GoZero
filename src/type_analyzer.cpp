#include "../include/type_analyzer.h"
#include <iostream>
#include <variant>
#include <cstdlib>

TypeAnalyzer::TypeAnalyzer() {
    pushScope(); // Global scope
}

void TypeAnalyzer::pushScope() {
    scopes.emplace_back();
}

void TypeAnalyzer::popScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

VarDeclStmt::Kind TypeAnalyzer::inferType(const Expr *expr) {
    if (auto *lit = dynamic_cast<const LiteralExpr*>(expr)) {
        if (std::holds_alternative<int>(lit->value))
            return VarDeclStmt::INT;
        if (std::holds_alternative<float>(lit->value))
            return VarDeclStmt::FLOAT;
        if (std::holds_alternative<std::string>(lit->value))
            return VarDeclStmt::STRING;
    }
    if (auto *var = dynamic_cast<const VarExpr*>(expr)) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto varIt = it->find(var->name);
            if (varIt != it->end()) {
                return varIt->second;
            }
        }
        std::cerr << "Error: variable no declarada '" << var->name << "'\n";
        std::exit(1);
    }
    if (auto *bin = dynamic_cast<const BinaryExpr*>(expr)) {
        auto left_type = inferType(bin->left.get());
        auto right_type = inferType(bin->right.get());

        // String concatenation
        if (bin->op == BinaryExpr::Op::ADD &&
            (left_type == VarDeclStmt::STRING || right_type == VarDeclStmt::STRING)) {
            return VarDeclStmt::STRING;
        }

        // Array operations (array + array, array * array, array * scalar)
        if ((left_type == VarDeclStmt::INT_ARRAY || left_type == VarDeclStmt::FLOAT_ARRAY) &&
            (right_type == VarDeclStmt::INT_ARRAY || right_type == VarDeclStmt::FLOAT_ARRAY)) {
            // Array + Array or Array * Array
            if (bin->op == BinaryExpr::Op::ADD || bin->op == BinaryExpr::Op::MUL) {
                if (left_type == VarDeclStmt::FLOAT_ARRAY || right_type == VarDeclStmt::FLOAT_ARRAY) {
                    return VarDeclStmt::FLOAT_ARRAY;
                }
                return VarDeclStmt::INT_ARRAY;
            }
        }
        
        // Array * Scalar operations
        if ((left_type == VarDeclStmt::INT_ARRAY || left_type == VarDeclStmt::FLOAT_ARRAY) &&
            (right_type == VarDeclStmt::INT || right_type == VarDeclStmt::FLOAT)) {
            if (bin->op == BinaryExpr::Op::MUL) {
                if (left_type == VarDeclStmt::FLOAT_ARRAY || right_type == VarDeclStmt::FLOAT) {
                    return VarDeclStmt::FLOAT_ARRAY;
                }
                return VarDeclStmt::INT_ARRAY;
            }
        }

        // Logical operations return INT (0 or 1)
        if (bin->op == BinaryExpr::Op::AND || bin->op == BinaryExpr::Op::OR ||
            bin->op == BinaryExpr::Op::EQ || bin->op == BinaryExpr::Op::NEQ ||
            bin->op == BinaryExpr::Op::LT || bin->op == BinaryExpr::Op::LE ||
            bin->op == BinaryExpr::Op::GT || bin->op == BinaryExpr::Op::GE) {
            return VarDeclStmt::INT;
        }

        // Arithmetic operations
        if (left_type == VarDeclStmt::FLOAT || right_type == VarDeclStmt::FLOAT) {
            return VarDeclStmt::FLOAT;
        }
        return VarDeclStmt::INT;
    }
    if (auto *unary = dynamic_cast<const UnaryExpr*>(expr)) {
        return getVariableType(unary->varName);
    }
    if (auto *array = dynamic_cast<const ArrayExpr*>(expr)) {
        if (array->elements.empty()) {
            return VarDeclStmt::INT_ARRAY; // default
        }
        // Infer type from first element
        auto firstType = inferType(array->elements[0].get());
        switch (firstType) {
            case VarDeclStmt::INT:
                return VarDeclStmt::INT_ARRAY;
            case VarDeclStmt::FLOAT:
                return VarDeclStmt::FLOAT_ARRAY;
            case VarDeclStmt::STRING:
                return VarDeclStmt::STRING_ARRAY;
            default:
                std::cerr << "Error: tipo de array no soportado\n";
                std::exit(1);
        }
    }
    if (auto *index = dynamic_cast<const IndexExpr*>(expr)) {
        auto arrayType = inferType(index->array.get());
        switch (arrayType) {
            case VarDeclStmt::INT_ARRAY:
                return VarDeclStmt::INT;
            case VarDeclStmt::FLOAT_ARRAY:
                return VarDeclStmt::FLOAT;
            case VarDeclStmt::STRING_ARRAY:
                return VarDeclStmt::STRING;
            case VarDeclStmt::STRING:
                return VarDeclStmt::INT; // Character access
            default:
                std::cerr << "Error: indexación en tipo no-array\n";
                std::exit(1);
        }
    }
    if (auto *call = dynamic_cast<const CallExpr*>(expr)) {
        auto it = functions.find(call->callee);
        if (it != functions.end()) {
            return it->second;
        }
        std::cerr << "Error: función no declarada '" << call->callee << "'\n";
        std::exit(1);
    }
    return VarDeclStmt::INT; // default
}

void TypeAnalyzer::declareFunction(const std::string &name, VarDeclStmt::Kind returnType) {
    functions[name] = returnType;
}

void TypeAnalyzer::setFunctionParams(const std::string &name, const std::vector<VarDeclStmt::Kind> &paramTypes) {
    functionParams[name] = paramTypes;
}

std::vector<VarDeclStmt::Kind> TypeAnalyzer::getFunctionParams(const std::string &name) {
    auto it = functionParams.find(name);
    if (it != functionParams.end()) {
        return it->second;
    }
    return {}; // Return empty vector if function not found
}

bool TypeAnalyzer::hasFunction(const std::string &name) {
    return functions.find(name) != functions.end();
}

void TypeAnalyzer::declareVariable(const std::string &name, VarDeclStmt::Kind type) {
    if (!scopes.empty()) {
        scopes.back()[name] = type;
    }
}

VarDeclStmt::Kind TypeAnalyzer::getVariableType(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto varIt = it->find(name);
        if (varIt != it->end()) {
            return varIt->second;
        }
    }
    std::cerr << "Error: variable no declarada '" << name << "'\n";
    std::exit(1);
}

bool TypeAnalyzer::hasVariable(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->find(name) != it->end()) {
            return true;
        }
    }
    return false;
}

void TypeAnalyzer::analyzeCallExpr(const CallExpr *call) {
    if (!hasFunction(call->callee)) {
        return; // Function not declared yet
    }

    std::vector<VarDeclStmt::Kind> paramTypes;
    for (const auto &arg : call->args) {
        VarDeclStmt::Kind argType = inferType(arg.get());
        paramTypes.push_back(argType);
    }

    // Update function parameter types
    setFunctionParams(call->callee, paramTypes);
}

void TypeAnalyzer::analyzeExpression(const Expr *expr) {
    if (auto *call = dynamic_cast<const CallExpr*>(expr)) {
        analyzeCallExpr(call);
    } else if (auto *bin = dynamic_cast<const BinaryExpr*>(expr)) {
        analyzeExpression(bin->left.get());
        analyzeExpression(bin->right.get());
    } else if (auto *index = dynamic_cast<const IndexExpr*>(expr)) {
        analyzeExpression(index->array.get());
        analyzeExpression(index->index.get());
    } else if (auto *array = dynamic_cast<const ArrayExpr*>(expr)) {
        for (const auto &elem : array->elements) {
            analyzeExpression(elem.get());
        }
    }
}

void TypeAnalyzer::analyzeStatement(const Stmt *stmt) {
    if (auto *print = dynamic_cast<const PrintStmt*>(stmt)) {
        analyzeExpression(print->expr.get());
    } else if (auto *assign = dynamic_cast<const AssignStmt*>(stmt)) {
        analyzeExpression(assign->expr.get());
    } else if (auto *varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
        analyzeExpression(varDecl->init.get());
    } else if (auto *inferDecl = dynamic_cast<const InferDeclStmt*>(stmt)) {
        analyzeExpression(inferDecl->init.get());
    } else if (auto *exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        analyzeExpression(exprStmt->expr.get());
    } else if (auto *ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        analyzeExpression(ifStmt->cond.get());
        for (const auto &thenStmt : ifStmt->thenBranch) {
            analyzeStatement(thenStmt.get());
        }
        for (const auto &elseStmt : ifStmt->elseBranch) {
            analyzeStatement(elseStmt.get());
        }
    } else if (auto *whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        analyzeExpression(whileStmt->cond.get());
        for (const auto &bodyStmt : whileStmt->body) {
            analyzeStatement(bodyStmt.get());
        }
    } else if (auto *forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        if (forStmt->init) analyzeStatement(forStmt->init.get());
        if (forStmt->cond) analyzeExpression(forStmt->cond.get());
        if (forStmt->post) analyzeStatement(forStmt->post.get());
        for (const auto &bodyStmt : forStmt->body) {
            analyzeStatement(bodyStmt.get());
        }
    } else if (auto *funcStmt = dynamic_cast<const FunctionStmt*>(stmt)) {
        for (const auto &bodyStmt : funcStmt->body) {
            analyzeStatement(bodyStmt.get());
        }
    } else if (auto *retStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        if (retStmt->value) {
            analyzeExpression(retStmt->value.get());
        }
    }
}

bool TypeAnalyzer::hasExplicitReturn(const std::vector<StmtPtr> &body) {
    for (const auto &stmt : body) {
        if (dynamic_cast<const ReturnStmt*>(stmt.get())) {
            return true;
        }
        // Check nested statements (if, while, for)
        if (auto *ifStmt = dynamic_cast<const IfStmt*>(stmt.get())) {
            if (hasExplicitReturn(ifStmt->thenBranch) || hasExplicitReturn(ifStmt->elseBranch)) {
                return true;
            }
        }
        else if (auto *whileStmt = dynamic_cast<const WhileStmt*>(stmt.get())) {
            if (hasExplicitReturn(whileStmt->body)) {
                return true;
            }
        }
        else if (auto *forStmt = dynamic_cast<const ForStmt*>(stmt.get())) {
            if (hasExplicitReturn(forStmt->body)) {
                return true;
            }
        }
    }
    return false;
}

VarDeclStmt::Kind TypeAnalyzer::getFunctionReturnType(const std::string &name) {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return it->second;
    }
    return VarDeclStmt::VOID; // Default
}

void TypeAnalyzer::validateFunctionScopes(const FunctionStmt *funcStmt) {
    std::cout << "Validando función: " << funcStmt->name << std::endl;
    
    // Create a map of local variables (parameters only, no access to main variables)
    std::map<std::string, VarDeclStmt::Kind> localVars;
    
    // Add function parameters to local scope
    std::vector<VarDeclStmt::Kind> paramTypes = getFunctionParams(funcStmt->name);
    for (size_t i = 0; i < funcStmt->params.size(); ++i) {
        // Default to INT if no explicit type information available
        VarDeclStmt::Kind paramType = VarDeclStmt::INT;
        if (i < paramTypes.size()) {
            paramType = paramTypes[i];
        }
        
        // Si el parámetro está marcado como array, usar INT_ARRAY
        if (i < funcStmt->paramIsArray.size() && funcStmt->paramIsArray[i]) {
            paramType = VarDeclStmt::INT_ARRAY;
        }
        
        localVars[funcStmt->params[i]] = paramType;
    }
    
    // Validate all statements in function body sequentially, updating scope as we go
    for (const auto &stmt : funcStmt->body) {
        // First check if this statement declares a new variable
        if (auto *varDecl = dynamic_cast<const VarDeclStmt*>(stmt.get())) {
            // Validate the initialization expression with current scope
            validateExpressionInFunctionScope(varDecl->init.get(), localVars, funcStmt->name);
            // Add the new variable to scope for subsequent statements
            localVars[varDecl->name] = varDecl->type;
        }
        else if (auto *inferDecl = dynamic_cast<const InferDeclStmt*>(stmt.get())) {
            // Validate the initialization expression with current scope
            validateExpressionInFunctionScope(inferDecl->init.get(), localVars, funcStmt->name);
            // For scope validation, we can assume INT type - the actual type inference happens later
            localVars[inferDecl->name] = VarDeclStmt::INT;
        }
        else {
            // For non-declaration statements, validate with current scope
            validateStatementInFunctionScope(stmt.get(), localVars, funcStmt->name);
        }
    }
}

void TypeAnalyzer::validateExpressionInFunctionScope(const Expr *expr, const std::map<std::string, VarDeclStmt::Kind> &localVars, const std::string &functionName) {
    if (auto *var = dynamic_cast<const VarExpr*>(expr)) {
        // Check if variable is in local scope (parameters or locally declared)
        if (localVars.find(var->name) == localVars.end()) {
            std::cerr << "Error de scope en función '" << functionName << "': ";
            std::cerr << "variable '" << var->name << "' no está accesible desde esta función.\n";
            std::cerr << "Las funciones solo pueden acceder a sus parámetros y variables locales.\n";
            std::exit(1);
        }
    }
    else if (auto *bin = dynamic_cast<const BinaryExpr*>(expr)) {
        validateExpressionInFunctionScope(bin->left.get(), localVars, functionName);
        validateExpressionInFunctionScope(bin->right.get(), localVars, functionName);
    }
    else if (auto *call = dynamic_cast<const CallExpr*>(expr)) {
        // Function calls are OK, just validate arguments
        for (const auto &arg : call->args) {
            validateExpressionInFunctionScope(arg.get(), localVars, functionName);
        }
    }
    else if (auto *index = dynamic_cast<const IndexExpr*>(expr)) {
        validateExpressionInFunctionScope(index->array.get(), localVars, functionName);
        validateExpressionInFunctionScope(index->index.get(), localVars, functionName);
    }
    else if (auto *array = dynamic_cast<const ArrayExpr*>(expr)) {
        for (const auto &elem : array->elements) {
            validateExpressionInFunctionScope(elem.get(), localVars, functionName);
        }
    }
    else if (auto *unary = dynamic_cast<const UnaryExpr*>(expr)) {
        // Check if the variable being incremented/decremented is accessible
        if (localVars.find(unary->varName) == localVars.end()) {
            std::cerr << "Error de scope en función '" << functionName << "': ";
            std::cerr << "variable '" << unary->varName << "' no está accesible desde esta función.\n";
            std::exit(1);
        }
    }
    // Literals are always OK - no validation needed
}

void TypeAnalyzer::validateStatementInFunctionScope(const Stmt *stmt, const std::map<std::string, VarDeclStmt::Kind> &localVars, const std::string &functionName) {
    std::map<std::string, VarDeclStmt::Kind> updatedLocalVars = localVars;
    
    if (auto *varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
        // Validate the initialization expression first with current scope
        validateExpressionInFunctionScope(varDecl->init.get(), updatedLocalVars, functionName);
        // Then add new local variable to scope
        updatedLocalVars[varDecl->name] = varDecl->type;
    }
    else if (auto *inferDecl = dynamic_cast<const InferDeclStmt*>(stmt)) {
        // Validate the initialization expression first with current scope
        validateExpressionInFunctionScope(inferDecl->init.get(), updatedLocalVars, functionName);
        // For scope validation, we can assume INT type - the actual type inference happens later
        updatedLocalVars[inferDecl->name] = VarDeclStmt::INT;
    }
    else if (auto *assign = dynamic_cast<const AssignStmt*>(stmt)) {
        // Validate both variable and expression
        if (updatedLocalVars.find(assign->name) == updatedLocalVars.end()) {
            std::cerr << "Error de scope en función '" << functionName << "': ";
            std::cerr << "variable '" << assign->name << "' no está accesible desde esta función.\n";
            std::exit(1);
        }
        validateExpressionInFunctionScope(assign->expr.get(), updatedLocalVars, functionName);
    }
    else if (auto *print = dynamic_cast<const PrintStmt*>(stmt)) {
        validateExpressionInFunctionScope(print->expr.get(), updatedLocalVars, functionName);
    }
    else if (auto *exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        validateExpressionInFunctionScope(exprStmt->expr.get(), updatedLocalVars, functionName);
    }
    else if (auto *ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        validateExpressionInFunctionScope(ifStmt->cond.get(), updatedLocalVars, functionName);
        for (const auto &thenStmt : ifStmt->thenBranch) {
            validateStatementInFunctionScope(thenStmt.get(), updatedLocalVars, functionName);
        }
        for (const auto &elseStmt : ifStmt->elseBranch) {
            validateStatementInFunctionScope(elseStmt.get(), updatedLocalVars, functionName);
        }
    }
    else if (auto *whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        validateExpressionInFunctionScope(whileStmt->cond.get(), updatedLocalVars, functionName);
        for (const auto &bodyStmt : whileStmt->body) {
            validateStatementInFunctionScope(bodyStmt.get(), updatedLocalVars, functionName);
        }
    }
    else if (auto *forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        // For loops create their own scope
        std::map<std::string, VarDeclStmt::Kind> forLocalVars = updatedLocalVars;
        
        if (forStmt->init) {
            // Handle variable declarations in for loop init
            if (auto *varDecl = dynamic_cast<const VarDeclStmt*>(forStmt->init.get())) {
                validateExpressionInFunctionScope(varDecl->init.get(), forLocalVars, functionName);
                forLocalVars[varDecl->name] = varDecl->type;
            } else if (auto *inferDecl = dynamic_cast<const InferDeclStmt*>(forStmt->init.get())) {
                validateExpressionInFunctionScope(inferDecl->init.get(), forLocalVars, functionName);
                VarDeclStmt::Kind inferredType = inferType(inferDecl->init.get());
                forLocalVars[inferDecl->name] = inferredType;
            } else {
                validateStatementInFunctionScope(forStmt->init.get(), forLocalVars, functionName);
            }
        }
        if (forStmt->cond) validateExpressionInFunctionScope(forStmt->cond.get(), forLocalVars, functionName);
        if (forStmt->post) validateStatementInFunctionScope(forStmt->post.get(), forLocalVars, functionName);
        for (const auto &bodyStmt : forStmt->body) {
            validateStatementInFunctionScope(bodyStmt.get(), forLocalVars, functionName);
        }
    }
    else if (auto *retStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        if (retStmt->value) {
            validateExpressionInFunctionScope(retStmt->value.get(), updatedLocalVars, functionName);
        }
    }
}

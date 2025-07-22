#pragma once
#include "ast.h"
#include "type_analyzer.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <map>
#include <vector>
#include <memory>

class CodeGenerator {
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    std::vector<std::map<std::string, llvm::AllocaInst*>> namedValuesStack;
    std::map<std::string, VarDeclStmt::Kind> variableTypes;
    std::map<std::string, int> arraySizes; // Store array sizes
    std::map<std::string, llvm::Function*> functions; // function name -> llvm function
    std::map<std::string, VarDeclStmt::Kind> functionTypes; // function name -> return type
    llvm::Function *mainFunction;
    llvm::Function *currentFunction;
    TypeAnalyzer &typeAnalyzer;

    // Helper methods
    llvm::Value* createStringConstant(const std::string &str);
    llvm::Value* generateStringConcat(const Expr *left, const Expr *right);
    llvm::Value* generateArrayOperation(const Expr *left, const Expr *right, BinaryExpr::Op op);
    llvm::Value* generateArrayScalarOperation(const Expr *array, const Expr *scalar, BinaryExpr::Op op);

public:
    CodeGenerator(llvm::LLVMContext &ctx, TypeAnalyzer &ta);
    
    void pushScope();
    void popScope();
    
    llvm::AllocaInst* findVariable(const std::string &name);
    void createBoundsCheck(llvm::Value *index, llvm::Value *size, const std::string &varName);
    void declareVariable(const std::string &name, llvm::AllocaInst *alloca);
    llvm::AllocaInst* createEntryBlockAlloca(const std::string &varName, llvm::Type *type);
    llvm::Type* getLLVMType(VarDeclStmt::Kind kind);
    
    void generateVarDecl(const VarDeclStmt *stmt);
    void generateInferDecl(const InferDeclStmt *stmt);
    void generateAssign(const AssignStmt *stmt);
    llvm::Value* generate(const Expr *expr);
    
    void generatePrintStmt(const PrintStmt *stmt);
    void printArrayElements(const std::string &arrayName, VarDeclStmt::Kind arrayType);
    void generateStatement(const Stmt *stmt);
    void generateIf(const IfStmt *stmt);
    void generateWhile(const WhileStmt *stmt);
    void generateFor(const ForStmt *stmt);
    void generateFunction(const FunctionStmt *stmt);
    void generateReturn(const ReturnStmt *stmt);
    
    void finalize();
    void printIR();
    void generateToObjectFile(const std::string &filename);
    void linkToExecutable(const std::string &objectFile, const std::string &executableName);
};

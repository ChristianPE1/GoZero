#include "include/lexer.h"
#include "include/parser.h"
#include "include/type_analyzer.h"
#include "include/code_generator.h"
#include <llvm/IR/LLVMContext.h>
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    std::string filename = "mini_input.txt"; // default
    bool showIR = false;
    // Analizar argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--ir" || arg == "-i") {
            showIR = true;
        } else {
            filename = arg;
        }
    }
    
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "No se pudo abrir " << filename << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string source = buffer.str();

    // Lexical analysis
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    // Parsing
    Parser parser(tokens);
    auto stmts = parser.parse();

    // Type analysis
    TypeAnalyzer typeAnalyzer;
    
    // First pass: Declare all functions
    for (const auto &stmt : stmts) {
        if (auto *funcStmt = dynamic_cast<const FunctionStmt*>(stmt.get())) {
            VarDeclStmt::Kind retType;
            if (funcStmt->inference) {
                // For inference functions, determine return type based on whether there's an explicit return
                if (typeAnalyzer.hasExplicitReturn(funcStmt->body)) {
                    retType = VarDeclStmt::INT; // Assume int if there's a return
                } else {
                    retType = VarDeclStmt::VOID; // Void if no return
                }
            } else {
                retType = funcStmt->retType;
            }
            typeAnalyzer.declareFunction(funcStmt->name, retType);
        }
    }

    // Second pass: Validate all functions for scope errors
    std::cout << "=== Validando scopes de funciones ===\n";
    for (const auto &stmt : stmts) {
        if (auto *funcStmt = dynamic_cast<const FunctionStmt*>(stmt.get())) {
            typeAnalyzer.validateFunctionScopes(funcStmt);
        }
    }

    // Code generation
    llvm::LLVMContext context;
    CodeGenerator generator(context, typeAnalyzer);

    std::cout << "=== Generando codigo intermedio ===\n";
    for (const auto &stmt : stmts) {
        generator.generateStatement(stmt.get());
    }

    generator.finalize();

    if (showIR) {
        generator.printIR();
    }

    std::cout << "\n=== Generando código máquina ===\n";

    // Generate object file (.o)
    generator.generateToObjectFile("output.o");

    // Link and create executable
    generator.linkToExecutable("output.o", "my_program");

    std::cout << "\n¡Compilación completa! Ejecuta con: ./my_program\n";

    return 0;
}

/*
Compilar con:
clang++ -g -O3 -std=c++17 main.cpp src/*.cpp $(llvm-config --cxxflags --ldflags --system-libs --libs all) -o gozero
./gozero
./my_program
*/

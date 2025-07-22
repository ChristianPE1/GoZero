#include "../include/code_generator.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <iostream>
#include <variant>
#include <cstdlib>

CodeGenerator::CodeGenerator(llvm::LLVMContext &ctx, TypeAnalyzer &ta)
: context(ctx), module(std::make_unique<llvm::Module>("my_module", context)),
builder(context), typeAnalyzer(ta), currentFunction(nullptr) {

    // Create main function
    llvm::FunctionType *mainType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
    mainFunction = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module.get());

    // Create basic block
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", mainFunction);
    builder.SetInsertPoint(entry);
    
    // Push global scope
    pushScope();
}

void CodeGenerator::pushScope() {
    namedValuesStack.emplace_back();
}

void CodeGenerator::popScope() {
    if (!namedValuesStack.empty()) {
        namedValuesStack.pop_back();
    }
}

llvm::AllocaInst* CodeGenerator::findVariable(const std::string &name) {
    for (auto it = namedValuesStack.rbegin(); it != namedValuesStack.rend(); ++it) {
        auto varIt = it->find(name);
        if (varIt != it->end()) {
            return varIt->second;
        }
    }
    return nullptr;
}

void CodeGenerator::createBoundsCheck(llvm::Value *index, llvm::Value *size, const std::string &varName) {
    // Convert index to i32 if needed
    llvm::Value *indexInt = index;
    if (index->getType() != llvm::Type::getInt32Ty(context)) {
        indexInt = builder.CreateSExtOrTrunc(index, llvm::Type::getInt32Ty(context), "index_i32");
    }
    
    // Create bounds check: index >= 0 && index < size
    llvm::Value *negativeCheck = builder.CreateICmpSGE(indexInt, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "negative_check");
    
    llvm::Value *upperBoundCheck = builder.CreateICmpSLT(indexInt, size, "upper_bound_check");
    llvm::Value *boundsOk = builder.CreateAnd(negativeCheck, upperBoundCheck, "bounds_ok");
    
    // Create basic blocks
    llvm::Function *func = currentFunction ? currentFunction : mainFunction;
    llvm::BasicBlock *boundsOkBB = llvm::BasicBlock::Create(context, "bounds_ok", func);
    llvm::BasicBlock *boundsFailBB = llvm::BasicBlock::Create(context, "bounds_fail", func);
    
    // Branch based on bounds check
    builder.CreateCondBr(boundsOk, boundsOkBB, boundsFailBB);
    
    // Bounds fail block - print error and exit
    builder.SetInsertPoint(boundsFailBB);
    
    // Create error message
    std::string errorMsg = "Runtime Error: Index out of bounds for variable '" + varName + "'\n";
    llvm::Value *errorStr = builder.CreateGlobalString(errorMsg, "error_msg");
    
    // Call printf to print error
    llvm::Function *printfFunc = module->getFunction("printf");
    if (!printfFunc) {
        std::vector<llvm::Type*> printfArgs;
        printfArgs.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context)));
        llvm::FunctionType *printfType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(context), printfArgs, true);
        printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, 
            "printf", module.get());
    }
    
    builder.CreateCall(printfFunc, {errorStr});
    
    // Exit with error code
    llvm::Function *exitFunc = module->getFunction("exit");
    if (!exitFunc) {
        llvm::FunctionType *exitType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(context), 
            {llvm::Type::getInt32Ty(context)}, false);
        exitFunc = llvm::Function::Create(exitType, llvm::Function::ExternalLinkage, 
            "exit", module.get());
    }
    
    builder.CreateCall(exitFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)});
    builder.CreateUnreachable();
    
    // Continue with bounds ok block
    builder.SetInsertPoint(boundsOkBB);
}

void CodeGenerator::declareVariable(const std::string &name, llvm::AllocaInst *alloca) {
    if (!namedValuesStack.empty()) {
        namedValuesStack.back()[name] = alloca;
    }
}

llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(const std::string &varName, llvm::Type *type) {
    llvm::Function *func = currentFunction ? currentFunction : mainFunction;
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
}

llvm::Type* CodeGenerator::getLLVMType(VarDeclStmt::Kind kind) {
    switch (kind) {
        case VarDeclStmt::INT: return llvm::Type::getInt32Ty(context);
        case VarDeclStmt::FLOAT: return llvm::Type::getFloatTy(context);
        case VarDeclStmt::STRING: return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
        case VarDeclStmt::INT_ARRAY: return llvm::PointerType::getUnqual(llvm::Type::getInt32Ty(context));
        case VarDeclStmt::FLOAT_ARRAY: return llvm::PointerType::getUnqual(llvm::Type::getFloatTy(context));
        case VarDeclStmt::STRING_ARRAY: return llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context)));
        case VarDeclStmt::VOID: return llvm::Type::getVoidTy(context);
    }
    return llvm::Type::getInt32Ty(context);
}

llvm::Value* CodeGenerator::createStringConstant(const std::string &str) {
    llvm::Constant *strConst = llvm::ConstantDataArray::getString(context, str);
    llvm::GlobalVariable *strVar = new llvm::GlobalVariable(
        *module, strConst->getType(), true, llvm::GlobalValue::PrivateLinkage, strConst, ".str");

    llvm::Value *zero = llvm::Constant::getNullValue(llvm::Type::getInt32Ty(context));
    std::vector<llvm::Value*> indices({zero, zero});
    return builder.CreateInBoundsGEP(strVar->getValueType(), strVar, indices, "string_ptr");
}

llvm::Value* CodeGenerator::generateStringConcat(const Expr *left, const Expr *right) {
    // Get string values
    llvm::Value *leftStr = generate(left);
    llvm::Value *rightStr = generate(right);

    if (!leftStr || !rightStr) return nullptr;

    // Declare strcat function
    llvm::Type *charPtrType = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    std::vector<llvm::Type*> strcatArgs({charPtrType, charPtrType});
    llvm::FunctionType *strcatType = llvm::FunctionType::get(charPtrType, strcatArgs, false);
    llvm::FunctionCallee strcatFunc = module->getOrInsertFunction("strcat", strcatType);

    // Declare strlen function
    std::vector<llvm::Type*> strlenArgs({charPtrType});
    llvm::FunctionType *strlenType = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), strlenArgs, false);
    llvm::FunctionCallee strlenFunc = module->getOrInsertFunction("strlen", strlenType);

    // Declare malloc function
    std::vector<llvm::Type*> mallocArgs({llvm::Type::getInt64Ty(context)});
    llvm::FunctionType *mallocType = llvm::FunctionType::get(charPtrType, mallocArgs, false);
    llvm::FunctionCallee mallocFunc = module->getOrInsertFunction("malloc", mallocType);

    // Declare strcpy function
    std::vector<llvm::Type*> strcpyArgs({charPtrType, charPtrType});
    llvm::FunctionType *strcpyType = llvm::FunctionType::get(charPtrType, strcpyArgs, false);
    llvm::FunctionCallee strcpyFunc = module->getOrInsertFunction("strcpy", strcpyType);

    // Get lengths of both strings
    llvm::Value *leftLen = builder.CreateCall(strlenFunc, {leftStr}, "left_len");
    llvm::Value *rightLen = builder.CreateCall(strlenFunc, {rightStr}, "right_len");

    // Calculate total length + 1 for null terminator
    llvm::Value *totalLen = builder.CreateAdd(leftLen, rightLen, "total_len");
    llvm::Value *mallocSize = builder.CreateAdd(totalLen,
                                                llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 1), "malloc_size");

    // Allocate memory for result
    llvm::Value *result = builder.CreateCall(mallocFunc, {mallocSize}, "result_str");

    // Copy left string to result
    builder.CreateCall(strcpyFunc, {result, leftStr});

    // Concatenate right string
    builder.CreateCall(strcatFunc, {result, rightStr});

    return result;
}

void CodeGenerator::finalize() {
    builder.CreateRetVoid();
}

void CodeGenerator::printIR() {
    module->print(llvm::outs(), nullptr);
}

void CodeGenerator::generateToObjectFile(const std::string &filename) {
    // Initialize targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        std::cerr << "Error: " << error << "\n";
        return;
    }

    auto CPU = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(targetTriple, CPU, features, opt, std::nullopt);

    module->setDataLayout(targetMachine->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        std::cerr << "No se pudo abrir archivo: " << EC.message() << "\n";
        return;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "TargetMachine no puede emitir archivo del tipo requerido\n";
        return;
    }

    pass.run(*module);
    dest.flush();

    std::cout << "Archivo objeto generado: " << filename << "\n";
}

void CodeGenerator::linkToExecutable(const std::string &objectFile, const std::string &executableName) {
    std::vector<std::string> commands = {
        "clang -no-pie " + objectFile + " -o " + executableName,
        "gcc -no-pie " + objectFile + " -o " + executableName,
        "clang -static " + objectFile + " -o " + executableName
    };

    for (const auto& command : commands) {
        std::cout << "Intentando: " << command << "\n";
        int result = std::system(command.c_str());
        if (result == 0) {
            std::cout << "Ejecutable generado: " << executableName << "\n";
            return;
        }
    }
    std::cerr << "Error al enlazar el ejecutable con todas las opciones\n";
}

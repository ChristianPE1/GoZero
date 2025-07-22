#include "../include/code_generator.h"
#include <iostream>
#include <variant>
#include <cstdlib>

void CodeGenerator::generatePrintStmt(const PrintStmt *stmt) {
    // Check if we're printing an array variable
    if (auto *var = dynamic_cast<const VarExpr*>(stmt->expr.get())) {
        llvm::AllocaInst *alloca = findVariable(var->name);
        if (alloca && variableTypes.find(var->name) != variableTypes.end()) {
            VarDeclStmt::Kind varType = variableTypes[var->name];
            if (varType == VarDeclStmt::INT_ARRAY || 
                varType == VarDeclStmt::FLOAT_ARRAY || 
                varType == VarDeclStmt::STRING_ARRAY) {
                printArrayElements(var->name, varType);
                return;
            }
        }
    }
    
    // Check if we're printing a character (string index access)
    bool isCharacter = false;
    if (auto *index = dynamic_cast<const IndexExpr*>(stmt->expr.get())) {
        if (auto *varExpr = dynamic_cast<const VarExpr*>(index->array.get())) {
            VarDeclStmt::Kind varType = variableTypes[varExpr->name];
            if (varType == VarDeclStmt::STRING) {
                isCharacter = true;
            }
        }
    }
    
    // Regular print logic
    llvm::Value *val = generate(stmt->expr.get());
    if (!val) return;

    // Declare printf
    std::vector<llvm::Type*> printfArgs({llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context))});
    llvm::FunctionType *printfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), printfArgs, true);
    llvm::FunctionCallee printfFunc = module->getOrInsertFunction("printf", printfType);

    // Choose format string and handle type conversions
    std::string fmtStr;
    llvm::Value *printVal = val;

    if (isCharacter) {
        fmtStr = "%c\n";
    } else if (val->getType()->isIntegerTy()) {
        fmtStr = "%d\n";
    } else if (val->getType()->isFloatTy()) {
        fmtStr = "%f\n";
        // Convert float to double for printf
        printVal = builder.CreateFPExt(val, llvm::Type::getDoubleTy(context), "double_val");
    } else {
        fmtStr = "%s\n";
    }

    llvm::Constant *strConst = llvm::ConstantDataArray::getString(context, fmtStr);
    llvm::GlobalVariable *fmtVar = new llvm::GlobalVariable(
        *module, strConst->getType(), true, llvm::GlobalValue::PrivateLinkage, strConst, ".str");

    llvm::Value *zero = llvm::Constant::getNullValue(llvm::Type::getInt32Ty(context));
    std::vector<llvm::Value*> indices({zero, zero});

    llvm::Value *strPtr = builder.CreateInBoundsGEP(
        fmtVar->getValueType(), fmtVar, indices, "format"
    );

    // Call printf
    builder.CreateCall(printfFunc, {strPtr, printVal});
}

void CodeGenerator::printArrayElements(const std::string &arrayName, VarDeclStmt::Kind arrayType) {
    // Load the array pointer
    llvm::AllocaInst *arrayAlloca = findVariable(arrayName);
    if (!arrayAlloca) return;
    
    llvm::Value *arrayPtr = builder.CreateLoad(arrayAlloca->getAllocatedType(), arrayAlloca, "array_load");
    
    // Get the real array size
    int arraySize = 5; // Default fallback
    auto sizeIt = arraySizes.find(arrayName);
    if (sizeIt != arraySizes.end()) {
        arraySize = sizeIt->second;
    }
    
    // Determine element type and format string
    llvm::Type *elemType;
    std::string fmtStr;
    
    if (arrayType == VarDeclStmt::INT_ARRAY) {
        elemType = llvm::Type::getInt32Ty(context);
        fmtStr = "%d ";
    } else if (arrayType == VarDeclStmt::FLOAT_ARRAY) {
        elemType = llvm::Type::getFloatTy(context);
        fmtStr = "%f ";
    } else {
        elemType = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
        fmtStr = "%s ";
    }
    
    // Print opening bracket
    std::string openBracket = "[";
    llvm::Constant *openConst = llvm::ConstantDataArray::getString(context, openBracket);
    llvm::GlobalVariable *openVar = new llvm::GlobalVariable(
        *module, openConst->getType(), true, llvm::GlobalValue::PrivateLinkage, openConst, ".str");
    
    llvm::Value *zero = llvm::Constant::getNullValue(llvm::Type::getInt32Ty(context));
    std::vector<llvm::Value*> indices({zero, zero});
    llvm::Value *openPtr = builder.CreateInBoundsGEP(openVar->getValueType(), openVar, indices, "open_bracket");
    
    // Declare printf
    std::vector<llvm::Type*> printfArgs({llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context))});
    llvm::FunctionType *printfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), printfArgs, true);
    llvm::FunctionCallee printfFunc = module->getOrInsertFunction("printf", printfType);
    
    builder.CreateCall(printfFunc, {openPtr});
    
    // Print elements
    for (int i = 0; i < arraySize; i++) {
        // Get element pointer
        llvm::Value *elemPtr = builder.CreateInBoundsGEP(
            elemType, arrayPtr, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), i)}, "elem_ptr");
        
        // Load element
        llvm::Value *elem = builder.CreateLoad(elemType, elemPtr, "elem");
        
        // Create format string for this element
        llvm::Constant *fmtConst = llvm::ConstantDataArray::getString(context, fmtStr);
        llvm::GlobalVariable *fmtVar = new llvm::GlobalVariable(
            *module, fmtConst->getType(), true, llvm::GlobalValue::PrivateLinkage, fmtConst, ".str");
        
        llvm::Value *fmtPtr = builder.CreateInBoundsGEP(fmtVar->getValueType(), fmtVar, indices, "format");
        
        // Convert float to double for printf if needed
        if (arrayType == VarDeclStmt::FLOAT_ARRAY) {
            elem = builder.CreateFPExt(elem, llvm::Type::getDoubleTy(context), "double_val");
        }
        
        builder.CreateCall(printfFunc, {fmtPtr, elem});
    }
    
    // Print closing bracket and newline
    std::string closeBracket = "]\n";
    llvm::Constant *closeConst = llvm::ConstantDataArray::getString(context, closeBracket);
    llvm::GlobalVariable *closeVar = new llvm::GlobalVariable(
        *module, closeConst->getType(), true, llvm::GlobalValue::PrivateLinkage, closeConst, ".str");
    
    llvm::Value *closePtr = builder.CreateInBoundsGEP(closeVar->getValueType(), closeVar, indices, "close_bracket");
    builder.CreateCall(printfFunc, {closePtr});
}

void CodeGenerator::generateStatement(const Stmt *stmt) {
    if (auto *varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
        generateVarDecl(varDecl);
    }
    else if (auto *inferDecl = dynamic_cast<const InferDeclStmt*>(stmt)) {
        generateInferDecl(inferDecl);
    }
    else if (auto *assign = dynamic_cast<const AssignStmt*>(stmt)) {
        generateAssign(assign);
    }
    else if (auto *print = dynamic_cast<const PrintStmt*>(stmt)) {
        generatePrintStmt(print);
    }
    else if (auto *ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        generateIf(ifStmt);
    }
    else if (auto *whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        generateWhile(whileStmt);
    }
    else if (auto *forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        generateFor(forStmt);
    }
    else if (auto *exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        generate(exprStmt->expr.get());
    }
    else if (auto *funcStmt = dynamic_cast<const FunctionStmt*>(stmt)) {
        generateFunction(funcStmt);
    }
    else if (auto *retStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        generateReturn(retStmt);
    }
}

void CodeGenerator::generateIf(const IfStmt *stmt) {
    llvm::Value *condValue = generate(stmt->cond.get());
    if (!condValue) return;

    // Convert condition to boolean
    llvm::Value *condBool = builder.CreateICmpNE(condValue, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "if_cond");

    llvm::Function *function = currentFunction ? currentFunction : mainFunction;
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", function);
    llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else", function);
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont", function);

    builder.CreateCondBr(condBool, thenBB, elseBB);

    // Generate then branch
    builder.SetInsertPoint(thenBB);
    typeAnalyzer.pushScope();
    pushScope();
    for (const auto &thenStmt : stmt->thenBranch) {
        generateStatement(thenStmt.get());
    }
    popScope();
    typeAnalyzer.popScope();
    builder.CreateBr(mergeBB);

    // Generate else branch
    builder.SetInsertPoint(elseBB);
    typeAnalyzer.pushScope();
    pushScope();
    for (const auto &elseStmt : stmt->elseBranch) {
        generateStatement(elseStmt.get());
    }
    popScope();
    typeAnalyzer.popScope();
    builder.CreateBr(mergeBB);

    // Continue with merge block
    builder.SetInsertPoint(mergeBB);
}

void CodeGenerator::generateWhile(const WhileStmt *stmt) {
    llvm::Function *function = currentFunction ? currentFunction : mainFunction;
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "while_cond", function);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "while_body", function);
    llvm::BasicBlock *endBB = llvm::BasicBlock::Create(context, "while_end", function);

    // Jump to condition check
    builder.CreateBr(condBB);

    // Condition check
    builder.SetInsertPoint(condBB);
    llvm::Value *condValue = generate(stmt->cond.get());
    if (!condValue) return;
    
    llvm::Value *condBool = builder.CreateICmpNE(condValue, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "while_cond");
    builder.CreateCondBr(condBool, bodyBB, endBB);

    // Generate body
    builder.SetInsertPoint(bodyBB);
    typeAnalyzer.pushScope();
    pushScope();
    for (const auto &bodyStmt : stmt->body) {
        generateStatement(bodyStmt.get());
    }
    popScope();
    typeAnalyzer.popScope();
    builder.CreateBr(condBB); // Loop back to condition

    // Continue after loop
    builder.SetInsertPoint(endBB);
}

void CodeGenerator::generateFor(const ForStmt *stmt) {
    // Create blocks
    llvm::Function *function = currentFunction ? currentFunction : mainFunction;
    llvm::BasicBlock *initBB = llvm::BasicBlock::Create(context, "for_init", function);
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "for_cond", function);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "for_body", function);
    llvm::BasicBlock *postBB = llvm::BasicBlock::Create(context, "for_post", function);
    llvm::BasicBlock *endBB = llvm::BasicBlock::Create(context, "for_end", function);

    // Jump to init block
    builder.CreateBr(initBB);

    // Generate init
    builder.SetInsertPoint(initBB);
    typeAnalyzer.pushScope(); // For loop scope
    pushScope();
    if (stmt->init) {
        generateStatement(stmt->init.get());
    }
    builder.CreateBr(condBB);

    // Generate condition
    builder.SetInsertPoint(condBB);
    if (stmt->cond) {
        llvm::Value *condValue = generate(stmt->cond.get());
        if (!condValue) return;
        llvm::Value *condBool = builder.CreateICmpNE(condValue, 
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "for_cond");
        builder.CreateCondBr(condBool, bodyBB, endBB);
    } else {
        builder.CreateBr(bodyBB); // Infinite loop if no condition
    }

    // Generate body
    builder.SetInsertPoint(bodyBB);
    for (const auto &bodyStmt : stmt->body) {
        generateStatement(bodyStmt.get());
    }
    builder.CreateBr(postBB);

    // Generate post
    builder.SetInsertPoint(postBB);
    if (stmt->post) {
        generateStatement(stmt->post.get());
    }
    builder.CreateBr(condBB); // Loop back to condition

    // Continue after loop
    builder.SetInsertPoint(endBB);
    popScope();
    typeAnalyzer.popScope(); // End for loop scope
}

void CodeGenerator::generateFunction(const FunctionStmt *stmt) {
    // Get inferred parameter types from type analyzer
    std::vector<VarDeclStmt::Kind> paramKinds = typeAnalyzer.getFunctionParams(stmt->name);
    
    // Build LLVM parameter types
    std::vector<llvm::Type*> paramTypes;
    for (size_t i = 0; i < stmt->params.size(); ++i) {
        llvm::Type* paramType;
        
        // Si el parámetro es array, usar puntero
        if (i < stmt->paramIsArray.size() && stmt->paramIsArray[i]) {
            paramType = llvm::PointerType::getUnqual(llvm::Type::getInt32Ty(context)); // int* para arrays
        } else {
            if (i < paramKinds.size()) {
                paramType = getLLVMType(paramKinds[i]);
            } else {
                paramType = llvm::Type::getInt32Ty(context); // Default to int
            }
        }
        
        paramTypes.push_back(paramType);
    }
    
    llvm::Type *returnType;
    VarDeclStmt::Kind actualReturnType;
    
    // Get the return type from type analyzer (already determined in main)
    actualReturnType = typeAnalyzer.getFunctionReturnType(stmt->name);
    
    if (actualReturnType == VarDeclStmt::VOID) {
        returnType = llvm::Type::getVoidTy(context);
    } else {
        returnType = getLLVMType(actualReturnType);
    }
    
    llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, false);
    llvm::Function *function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, stmt->name, module.get());
    
    // Register function
    functions[stmt->name] = function;
    functionTypes[stmt->name] = actualReturnType;
    
    // Set parameter names
    auto argIt = function->arg_begin();
    for (size_t i = 0; i < stmt->params.size(); ++i, ++argIt) {
        argIt->setName(stmt->params[i]);
    }
    
    // Create function body
    llvm::BasicBlock *funcBB = llvm::BasicBlock::Create(context, "entry", function);
    
    // Save current builder state
    llvm::BasicBlock *oldInsertBlock = builder.GetInsertBlock();
    llvm::Function *oldFunction = currentFunction;
    
    builder.SetInsertPoint(funcBB);
    currentFunction = function;
    
    // Create new scope for function
    typeAnalyzer.pushScope();
    pushScope();
    
    // Add parameters to scope
    argIt = function->arg_begin();
    for (size_t i = 0; i < stmt->params.size(); ++i, ++argIt) {
        llvm::Type *paramType = paramTypes[i];
        llvm::AllocaInst *paramAlloca = createEntryBlockAlloca(stmt->params[i], paramType);
        builder.CreateStore(&*argIt, paramAlloca);
        declareVariable(stmt->params[i], paramAlloca);
        
        // Also declare in type analyzer
        VarDeclStmt::Kind paramKind = (i < paramKinds.size()) ? paramKinds[i] : VarDeclStmt::INT;
        
        // Si el parámetro está marcado como array, usar INT_ARRAY
        if (i < stmt->paramIsArray.size() && stmt->paramIsArray[i]) {
            paramKind = VarDeclStmt::INT_ARRAY;
        }
        
        typeAnalyzer.declareVariable(stmt->params[i], paramKind);
        variableTypes[stmt->params[i]] = paramKind;
    }
    
    // Generate function body
    for (const auto &bodyStmt : stmt->body) {
        generateStatement(bodyStmt.get());
    }
    
    // Add default return if needed
    if (actualReturnType == VarDeclStmt::VOID) {
        builder.CreateRetVoid();
    } else {
        // Default return value if no explicit return
        llvm::Value *defaultVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
        builder.CreateRet(defaultVal);
    }
    
    // Restore previous state
    popScope();
    typeAnalyzer.popScope();
    currentFunction = oldFunction;
    if (oldInsertBlock) {
        builder.SetInsertPoint(oldInsertBlock);
    }
}

void CodeGenerator::generateReturn(const ReturnStmt *stmt) {
    if (stmt->value) {
        llvm::Value *retVal = generate(stmt->value.get());
        if (retVal) {
            builder.CreateRet(retVal);
        }
    } else {
        builder.CreateRetVoid();
    }
}


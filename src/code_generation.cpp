#include "../include/code_generator.h"
#include <iostream>
#include <variant>
#include <cstdlib>

void CodeGenerator::generateVarDecl(const VarDeclStmt *stmt) {
    // First declare the variable in type analyzer
    typeAnalyzer.declareVariable(stmt->name, stmt->type);

    // Check if the init expression is an array to store its size
    if (auto *arrayExpr = dynamic_cast<const ArrayExpr*>(stmt->init.get())) {
        arraySizes[stmt->name] = arrayExpr->elements.size();
    }

    llvm::Value *initVal = generate(stmt->init.get());
    if (!initVal) return;

    llvm::Type *varType = getLLVMType(stmt->type);
    
    // For arrays, we need to handle them differently
    if (stmt->type == VarDeclStmt::INT_ARRAY || 
        stmt->type == VarDeclStmt::FLOAT_ARRAY || 
        stmt->type == VarDeclStmt::STRING_ARRAY) {
        
        // For array types, store the pointer to the array
        llvm::AllocaInst *alloca = createEntryBlockAlloca(stmt->name, varType);
        builder.CreateStore(initVal, alloca);
        declareVariable(stmt->name, alloca);
        variableTypes[stmt->name] = stmt->type;
    } else {
        // Regular variables
        llvm::AllocaInst *alloca = createEntryBlockAlloca(stmt->name, varType);

        // Type conversion if needed
        if (stmt->type == VarDeclStmt::FLOAT && initVal->getType()->isIntegerTy()) {
            initVal = builder.CreateSIToFP(initVal, llvm::Type::getFloatTy(context), "int_to_float");
        }

        builder.CreateStore(initVal, alloca);
        declareVariable(stmt->name, alloca);
        variableTypes[stmt->name] = stmt->type;
    }
}

void CodeGenerator::generateInferDecl(const InferDeclStmt *stmt) {
    // First analyze the expression to get its type
    VarDeclStmt::Kind inferredType = typeAnalyzer.inferType(stmt->init.get());
    typeAnalyzer.declareVariable(stmt->name, inferredType);

    // Check if the init expression is an array to store its size
    if (auto *arrayExpr = dynamic_cast<const ArrayExpr*>(stmt->init.get())) {
        arraySizes[stmt->name] = arrayExpr->elements.size();
    }
    // Handle array operations result size
    else if (auto *binExpr = dynamic_cast<const BinaryExpr*>(stmt->init.get())) {
        if (inferredType == VarDeclStmt::INT_ARRAY || inferredType == VarDeclStmt::FLOAT_ARRAY) {
            // For array operations, get size from one of the operands
            if (auto *leftVar = dynamic_cast<const VarExpr*>(binExpr->left.get())) {
                if (arraySizes.find(leftVar->name) != arraySizes.end()) {
                    arraySizes[stmt->name] = arraySizes[leftVar->name];
                }
            } else if (auto *rightVar = dynamic_cast<const VarExpr*>(binExpr->right.get())) {
                if (arraySizes.find(rightVar->name) != arraySizes.end()) {
                    arraySizes[stmt->name] = arraySizes[rightVar->name];
                }
            }
        }
    }

    llvm::Value *initVal = generate(stmt->init.get());
    if (!initVal) return;

    // For inferred array types, we need special handling
    if (inferredType == VarDeclStmt::INT_ARRAY || 
        inferredType == VarDeclStmt::FLOAT_ARRAY || 
        inferredType == VarDeclStmt::STRING_ARRAY) {
        
        llvm::Type *varType = getLLVMType(inferredType);
        llvm::AllocaInst *alloca = createEntryBlockAlloca(stmt->name, varType);
        builder.CreateStore(initVal, alloca);
        declareVariable(stmt->name, alloca);
        variableTypes[stmt->name] = inferredType;
    } else {
        // Regular inferred variables
        llvm::Type *varType = getLLVMType(inferredType);
        llvm::AllocaInst *alloca = createEntryBlockAlloca(stmt->name, varType);

        // Store the value directly since types should match
        builder.CreateStore(initVal, alloca);
        declareVariable(stmt->name, alloca);
        variableTypes[stmt->name] = inferredType;
    }
}

void CodeGenerator::generateAssign(const AssignStmt *stmt) {
    // Make sure the variable exists in type analyzer
    if (!typeAnalyzer.hasVariable(stmt->name)) {
        std::cerr << "Error (line " << stmt->line << ":" << stmt->column << "): intento de asignar a variable no declarada '" << stmt->name << "'\n";
        std::exit(1);
    }

    llvm::Value *val = generate(stmt->expr.get());
    if (!val) return;

    llvm::AllocaInst *alloca = findVariable(stmt->name);
    if (alloca) {
        // Type conversion if needed
        VarDeclStmt::Kind varType = variableTypes[stmt->name];
        if (varType == VarDeclStmt::FLOAT && val->getType()->isIntegerTy()) {
            val = builder.CreateSIToFP(val, llvm::Type::getFloatTy(context), "int_to_float");
        }
        builder.CreateStore(val, alloca);
    } else {
        std::cerr << "Runtime Error (line " << stmt->line << ":" << stmt->column << "): Variable '" << stmt->name << "' is not accessible in current scope\n";
        std::exit(1);
    }
}

llvm::Value* CodeGenerator::generate(const Expr *expr) {
    if (auto *lit = dynamic_cast<const LiteralExpr*>(expr)) {
        if (std::holds_alternative<int>(lit->value))
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), std::get<int>(lit->value));
        if (std::holds_alternative<float>(lit->value))
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(context), std::get<float>(lit->value));
        if (std::holds_alternative<std::string>(lit->value)) {
            return createStringConstant(std::get<std::string>(lit->value));
        }
    }
    else if (auto *var = dynamic_cast<const VarExpr*>(expr)) {
        llvm::AllocaInst *alloca = findVariable(var->name);
        if (alloca) {
            return builder.CreateLoad(alloca->getAllocatedType(), alloca, var->name + "_load");
        } else {
            std::cerr << "Runtime Error (line " << var->line << ":" << var->column << "): Variable '" << var->name << "' is not accessible in current scope\n";
            std::exit(1);
        }
    }
    else if (auto *bin = dynamic_cast<const BinaryExpr*>(expr)) {
        // Check if we're dealing with array operations
        VarDeclStmt::Kind leftType = typeAnalyzer.inferType(bin->left.get());
        VarDeclStmt::Kind rightType = typeAnalyzer.inferType(bin->right.get());
        
        // Array + Array operations (element-wise)
        if ((leftType == VarDeclStmt::INT_ARRAY || leftType == VarDeclStmt::FLOAT_ARRAY) &&
            (rightType == VarDeclStmt::INT_ARRAY || rightType == VarDeclStmt::FLOAT_ARRAY) &&
            (bin->op == BinaryExpr::Op::ADD || bin->op == BinaryExpr::Op::MUL)) {
            
            return generateArrayOperation(bin->left.get(), bin->right.get(), bin->op);
        }
        
        // Array * Scalar operations
        if ((leftType == VarDeclStmt::INT_ARRAY || leftType == VarDeclStmt::FLOAT_ARRAY) &&
            (rightType == VarDeclStmt::INT || rightType == VarDeclStmt::FLOAT) &&
            bin->op == BinaryExpr::Op::MUL) {
            
            return generateArrayScalarOperation(bin->left.get(), bin->right.get(), bin->op);
        }
        
        // String concatenation case
        if (bin->op == BinaryExpr::Op::ADD) {
            if (leftType == VarDeclStmt::STRING || rightType == VarDeclStmt::STRING) {
                return generateStringConcat(bin->left.get(), bin->right.get());
            }
        }

        llvm::Value *left = generate(bin->left.get());
        llvm::Value *right = generate(bin->right.get());
        if (!left || !right) return nullptr;

        // Type promotion for arithmetic operations
        if (left->getType()->isIntegerTy() && right->getType()->isFloatTy()) {
            left = builder.CreateSIToFP(left, llvm::Type::getFloatTy(context), "int_to_float");
        } else if (left->getType()->isFloatTy() && right->getType()->isIntegerTy()) {
            right = builder.CreateSIToFP(right, llvm::Type::getFloatTy(context), "int_to_float");
        }

        bool isFloat = left->getType()->isFloatTy();
        
        switch (bin->op) {
            case BinaryExpr::Op::ADD:
                return isFloat ? builder.CreateFAdd(left, right, "addtmp") 
                               : builder.CreateAdd(left, right, "addtmp");
            case BinaryExpr::Op::SUB:
                return isFloat ? builder.CreateFSub(left, right, "subtmp") 
                               : builder.CreateSub(left, right, "subtmp");
            case BinaryExpr::Op::MUL:
                return isFloat ? builder.CreateFMul(left, right, "multmp") 
                               : builder.CreateMul(left, right, "multmp");
            case BinaryExpr::Op::DIV:
                return isFloat ? builder.CreateFDiv(left, right, "divtmp") 
                               : builder.CreateSDiv(left, right, "divtmp");
            case BinaryExpr::Op::LT:
                if (isFloat) {
                    llvm::Value *cmp = builder.CreateFCmpOLT(left, right, "lttmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                } else {
                    llvm::Value *cmp = builder.CreateICmpSLT(left, right, "lttmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                }
            case BinaryExpr::Op::LE:
                if (isFloat) {
                    llvm::Value *cmp = builder.CreateFCmpOLE(left, right, "letmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                } else {
                    llvm::Value *cmp = builder.CreateICmpSLE(left, right, "letmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                }
            case BinaryExpr::Op::GT:
                if (isFloat) {
                    llvm::Value *cmp = builder.CreateFCmpOGT(left, right, "gttmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                } else {
                    llvm::Value *cmp = builder.CreateICmpSGT(left, right, "gttmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                }
            case BinaryExpr::Op::GE:
                if (isFloat) {
                    llvm::Value *cmp = builder.CreateFCmpOGE(left, right, "getmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                } else {
                    llvm::Value *cmp = builder.CreateICmpSGE(left, right, "getmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                }
            case BinaryExpr::Op::EQ:
                if (isFloat) {
                    llvm::Value *cmp = builder.CreateFCmpOEQ(left, right, "eqtmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                } else {
                    llvm::Value *cmp = builder.CreateICmpEQ(left, right, "eqtmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                }
            case BinaryExpr::Op::NEQ:
                if (isFloat) {
                    llvm::Value *cmp = builder.CreateFCmpONE(left, right, "neqtmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                } else {
                    llvm::Value *cmp = builder.CreateICmpNE(left, right, "neqtmp");
                    return builder.CreateZExt(cmp, llvm::Type::getInt32Ty(context), "booltmp");
                }
            case BinaryExpr::Op::AND:
                return builder.CreateAnd(left, right, "andtmp");
            case BinaryExpr::Op::OR:
                return builder.CreateOr(left, right, "ortmp");
        }
    }
    else if (auto *unary = dynamic_cast<const UnaryExpr*>(expr)) {
        llvm::AllocaInst *alloca = findVariable(unary->varName);
        if (!alloca) return nullptr;

        llvm::Value *current = builder.CreateLoad(alloca->getAllocatedType(), alloca, "current_val");
        llvm::Value *one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
        
        llvm::Value *newVal;
        if (unary->op == UnaryExpr::Op::PRE_INC) {
            newVal = builder.CreateAdd(current, one, "inc");
        } else {
            newVal = builder.CreateSub(current, one, "dec");
        }
        
        builder.CreateStore(newVal, alloca);
        return newVal;
    }
    else if (auto *array = dynamic_cast<const ArrayExpr*>(expr)) {
        if (array->elements.empty()) return nullptr;

        // Determine array type from first element
        VarDeclStmt::Kind elemType = typeAnalyzer.inferType(array->elements[0].get());
        llvm::Type *llvmElemType = getLLVMType(elemType);
        
        // Create array type
        llvm::ArrayType *arrayType = llvm::ArrayType::get(llvmElemType, array->elements.size());
        
        // Allocate temporary array on stack
        llvm::AllocaInst *arrayAlloca = createEntryBlockAlloca("array_tmp", arrayType);
        
        // Initialize elements
        for (size_t i = 0; i < array->elements.size(); ++i) {
            llvm::Value *elemVal = generate(array->elements[i].get());
            if (!elemVal) continue;
            
            llvm::Value *elemPtr = builder.CreateInBoundsGEP(
                arrayType, arrayAlloca, 
                {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
                 llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), i)}, 
                "elem_ptr");
            builder.CreateStore(elemVal, elemPtr);
        }
        
        // Return pointer to first element
        llvm::Value *arrayPtr = builder.CreateInBoundsGEP(
            arrayType, arrayAlloca,
            {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
             llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)}, 
            "array_ptr");
        
        return arrayPtr;
    }
    else if (auto *index = dynamic_cast<const IndexExpr*>(expr)) {
        llvm::Value *indexVal = generate(index->index.get());
        if (!indexVal) return nullptr;

        // Check if we're indexing a variable
        if (auto *varExpr = dynamic_cast<const VarExpr*>(index->array.get())) {
            std::string varName = varExpr->name;
            
            // Check if it's a string (character access)
            VarDeclStmt::Kind varType = variableTypes[varName];
            if (varType == VarDeclStmt::STRING) {
                llvm::AllocaInst *stringAlloca = findVariable(varName);
                if (!stringAlloca) return nullptr;
                
                llvm::Value *stringPtr = builder.CreateLoad(stringAlloca->getAllocatedType(), stringAlloca, "string_load");
                
                // Get string length for bounds checking
                std::vector<llvm::Type*> strlenArgs({llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context))});
                llvm::FunctionType *strlenType = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), strlenArgs, false);
                llvm::FunctionCallee strlenFunc = module->getOrInsertFunction("strlen", strlenType);
                
                llvm::Value *stringLen = builder.CreateCall(strlenFunc, {stringPtr}, "string_len");
                llvm::Value *stringLenInt = builder.CreateTrunc(stringLen, llvm::Type::getInt32Ty(context), "string_len_i32");
                
                // Bounds check
                createBoundsCheck(indexVal, stringLenInt, varName);
                
                // Get character
                llvm::Value *charPtr = builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(context), stringPtr, indexVal, "char_ptr");
                return builder.CreateLoad(llvm::Type::getInt8Ty(context), charPtr, "char_val");
            }
            // Array access
            else if (varType == VarDeclStmt::INT_ARRAY || varType == VarDeclStmt::FLOAT_ARRAY || varType == VarDeclStmt::STRING_ARRAY) {
                llvm::AllocaInst *arrayAlloca = findVariable(varName);
                if (!arrayAlloca) return nullptr;
                
                llvm::Value *arrayPtr = builder.CreateLoad(arrayAlloca->getAllocatedType(), arrayAlloca, "array_load");
                
                // Get array size for bounds checking
                auto sizeIt = arraySizes.find(varName);
                if (sizeIt != arraySizes.end()) {
                    llvm::Value *arraySize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), sizeIt->second);
                    createBoundsCheck(indexVal, arraySize, varName);
                }
                
                llvm::Type *elemType;
                if (varType == VarDeclStmt::INT_ARRAY) {
                    elemType = llvm::Type::getInt32Ty(context);
                } else if (varType == VarDeclStmt::FLOAT_ARRAY) {
                    elemType = llvm::Type::getFloatTy(context);
                } else {
                    elemType = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
                }
                
                llvm::Value *elemPtr = builder.CreateInBoundsGEP(elemType, arrayPtr, indexVal, "elem_ptr");
                return builder.CreateLoad(elemType, elemPtr, "array_elem");
            } else {
                std::cerr << "Error: variable array no encontrada '" << varExpr->name << "'\n";
                std::cerr << "Compilación terminada debido a errores.\n";
                std::exit(1);
            }
        }
        
        return nullptr;
    }
    else if (auto *call = dynamic_cast<const CallExpr*>(expr)) {
        // Look up the function
        auto funcIt = functions.find(call->callee);
        if (funcIt == functions.end()) {
            std::cerr << "Error fatal: función no declarada '" << call->callee << "'\n";
            std::cerr << "Compilación terminada debido a errores.\n";
            std::exit(1);
        }
        
        llvm::Function *func = funcIt->second;
        
        // Generate arguments
        std::vector<llvm::Value*> args;
        for (const auto &arg : call->args) {
            llvm::Value *argVal = generate(arg.get());
            if (!argVal) return nullptr;
            args.push_back(argVal);
        }
        
        // Call function
        if (func->getReturnType()->isVoidTy()) {
            builder.CreateCall(func, args);
            return nullptr; // No return value
        } else {
            return builder.CreateCall(func, args, "call_result");
        }
    }
    return nullptr;
}

// Generate element-wise array operations (v + w, v * w)
llvm::Value* CodeGenerator::generateArrayOperation(const Expr *left, const Expr *right, BinaryExpr::Op op) {
    llvm::Value *leftArray = generate(left);
    llvm::Value *rightArray = generate(right);
    if (!leftArray || !rightArray) return nullptr;
    
    // Get array sizes (assuming both arrays have same size for now)
    std::string leftName, rightName;
    if (auto *varExpr = dynamic_cast<const VarExpr*>(left)) {
        leftName = varExpr->name;
    }
    if (auto *varExpr = dynamic_cast<const VarExpr*>(right)) {
        rightName = varExpr->name;
    }
    
    int arraySize = 3; // Default size for [1,2,3] style arrays
    if (!leftName.empty() && arraySizes.find(leftName) != arraySizes.end()) {
        arraySize = arraySizes[leftName];
    }
    
    // Determine result type
    VarDeclStmt::Kind leftType = typeAnalyzer.inferType(left);
    VarDeclStmt::Kind rightType = typeAnalyzer.inferType(right);
    bool isFloatResult = (leftType == VarDeclStmt::FLOAT_ARRAY || rightType == VarDeclStmt::FLOAT_ARRAY);
    
    // Create result array
    llvm::Type *elementType = isFloatResult ? llvm::Type::getFloatTy(context) : llvm::Type::getInt32Ty(context);
    llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, arraySize);
    llvm::AllocaInst *resultArray = createEntryBlockAlloca("array_result_tmp", arrayType);
    
    // Generate loop to perform element-wise operation
    for (int i = 0; i < arraySize; i++) {
        // Get left element
        llvm::Value *leftElemPtr = builder.CreateGEP(elementType, leftArray, 
            builder.getInt32(i), "left_elem_ptr");
        llvm::Value *leftElem = builder.CreateLoad(elementType, leftElemPtr, "left_elem");
        
        // Get right element
        llvm::Value *rightElemPtr = builder.CreateGEP(elementType, rightArray, 
            builder.getInt32(i), "right_elem_ptr");
        llvm::Value *rightElem = builder.CreateLoad(elementType, rightElemPtr, "right_elem");
        
        // Perform operation
        llvm::Value *result;
        if (op == BinaryExpr::Op::ADD) {
            result = isFloatResult ? builder.CreateFAdd(leftElem, rightElem, "add_result")
                                   : builder.CreateAdd(leftElem, rightElem, "add_result");
        } else if (op == BinaryExpr::Op::MUL) {
            result = isFloatResult ? builder.CreateFMul(leftElem, rightElem, "mul_result")
                                   : builder.CreateMul(leftElem, rightElem, "mul_result");
        } else {
            return nullptr;
        }
        
        // Store result
        llvm::Value *resultElemPtr = builder.CreateGEP(arrayType, resultArray, 
            {builder.getInt32(0), builder.getInt32(i)}, "result_elem_ptr");
        builder.CreateStore(result, resultElemPtr);
    }
    
    // Return pointer to first element
    return builder.CreateGEP(arrayType, resultArray, 
        {builder.getInt32(0), builder.getInt32(0)}, "array_ptr");
}

// Generate array * scalar operations (v * n)
llvm::Value* CodeGenerator::generateArrayScalarOperation(const Expr *array, const Expr *scalar, BinaryExpr::Op op) {
    llvm::Value *arrayVal = generate(array);
    llvm::Value *scalarVal = generate(scalar);
    if (!arrayVal || !scalarVal) return nullptr;
    
    // Get array size
    std::string arrayName;
    if (auto *varExpr = dynamic_cast<const VarExpr*>(array)) {
        arrayName = varExpr->name;
    }
    
    int arraySize = 3; // Default size
    if (!arrayName.empty() && arraySizes.find(arrayName) != arraySizes.end()) {
        arraySize = arraySizes[arrayName];
    }
    
    // Determine types
    VarDeclStmt::Kind arrayType = typeAnalyzer.inferType(array);
    VarDeclStmt::Kind scalarType = typeAnalyzer.inferType(scalar);
    bool isFloatResult = (arrayType == VarDeclStmt::FLOAT_ARRAY || scalarType == VarDeclStmt::FLOAT);
    
    // Create result array
    llvm::Type *elementType = isFloatResult ? llvm::Type::getFloatTy(context) : llvm::Type::getInt32Ty(context);
    llvm::ArrayType *resultArrayType = llvm::ArrayType::get(elementType, arraySize);
    llvm::AllocaInst *resultArray = createEntryBlockAlloca("scalar_result_tmp", resultArrayType);
    
    // Convert scalar if needed
    if (isFloatResult && scalarVal->getType()->isIntegerTy()) {
        scalarVal = builder.CreateSIToFP(scalarVal, llvm::Type::getFloatTy(context), "scalar_to_float");
    }
    
    // Generate loop to multiply each element by scalar
    for (int i = 0; i < arraySize; i++) {
        // Get array element
        llvm::Value *elemPtr = builder.CreateGEP(elementType, arrayVal, 
            builder.getInt32(i), "elem_ptr");
        llvm::Value *elem = builder.CreateLoad(elementType, elemPtr, "elem");
        
        // Multiply by scalar
        llvm::Value *result;
        if (op == BinaryExpr::Op::MUL) {
            result = isFloatResult ? builder.CreateFMul(elem, scalarVal, "scalar_mul_result")
                                   : builder.CreateMul(elem, scalarVal, "scalar_mul_result");
        } else {
            return nullptr;
        }
        
        // Store result
        llvm::Value *resultElemPtr = builder.CreateGEP(resultArrayType, resultArray, 
            {builder.getInt32(0), builder.getInt32(i)}, "result_elem_ptr");
        builder.CreateStore(result, resultElemPtr);
    }
    
    // Return pointer to first element
    return builder.CreateGEP(resultArrayType, resultArray, 
        {builder.getInt32(0), builder.getInt32(0)}, "array_ptr");
}

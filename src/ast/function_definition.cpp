#include "function_definition.h"

std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition) {
    definition.print(stream);
    return stream;
}

void FunctionDefinition::print(std::ostream& stream) {
    stream << this->_declaration << '\n';
    this->_block.print(stream);
}

void FunctionDefinition::typecheck(ScopePtr& scope) {
    auto function = this->_declaration.toType(scope);

    if (function.type->kind != TypeKind::TY_FUNCTION) {
        errorloc(this->_declaration.loc, "Internal error: Expected function definition to have function type");
    }

    auto function_type = std::static_pointer_cast<FunctionType>(function.type);

    // Add this function's signature to the scope given as an argument
    if (scope->addFunctionDeclaration(function)) {
        errorloc(this->_declaration.loc, "Duplicate function");
    }

    // Create inner function scope and add function arguments
    auto function_scope = function_type->scope;
    function_scope->setLabels(this->_labels);

    // 6.9.1.3: The return type of a function shall be void or a complete object type other than array type.
    auto return_type = function_type->return_type;
    if (return_type->kind != TypeKind::TY_VOID && !(return_type->isObjectType() && return_type->isComplete())) {
        errorloc(this->_declaration.declarator->loc, "Function return type must be void or a complete object type");
    }

    if (function_type->has_params) {
        auto param_function = std::static_pointer_cast<ParamFunctionType>(function_type);

        for (auto& param : param_function->params) {
            if (param.isAbstract()) {
                errorloc(this->_declaration.declarator->loc, "parameters must not be abstract");
            }
            if (function_scope->addDeclaration(param, true)) {
                errorloc(this->_declaration.declarator->loc, "parameter names have to be unique");
            }
        }
    }

    this->_block.typecheckInner(function_scope);
    this->_type = function_type;
}

void FunctionDefinition::compile(CompileScopePtr compile_scope) {
    // TODO cleanup needed
    std::shared_ptr<FunctionType> func_type_ptr = this->_type;
    auto llvm_type = this->_type->toLLVMType(compile_scope);
    std::string name(*(this->_declaration.declarator->getName().value()));
    // Check if function was already declared, if yes, choose it, otherwise create a new one
    llvm::Function* Func = compile_scope->module.getFunction(name);
    if (Func == nullptr) {
        Func = llvm::Function::Create(
            llvm_type /* FunctionType *Ty */,
            llvm::GlobalValue::ExternalLinkage /* LinkageType */,
            name /* const Twine &N="" */,
            compile_scope->module /* Module *M=0 */
        );
    }

    llvm::Function::arg_iterator FuncArgIt = Func->arg_begin();
    auto inner_compile_scope = std::make_shared<CompileScope>(compile_scope, Func);

    int count = 0;
    auto param_func_type = std::static_pointer_cast<ParamFunctionType>(func_type_ptr);
    while (FuncArgIt != Func->arg_end()) {
        llvm::Argument* Arg = FuncArgIt;
        if (param_func_type->params[count].name.has_value()) {
            Arg->setName(*(param_func_type->params[count].name.value()));
        } else {
            Arg->setName("");
        }
        FuncArgIt++;
        count++;
    }
    llvm::BasicBlock* FuncEntryBB = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "entry" /* const Twine &Name="" */,
        Func /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    compile_scope->builder.SetInsertPoint(FuncEntryBB);
    compile_scope->alloca_builder.SetInsertPoint(FuncEntryBB);

    count = 0;
    FuncArgIt = Func->arg_begin();
    auto FuncLLVMTypeIt = llvm_type->param_begin();
    while (FuncArgIt != Func->arg_end()) {
        compile_scope->alloca_builder.SetInsertPoint(
            compile_scope->alloca_builder.GetInsertBlock(), compile_scope->alloca_builder.GetInsertBlock()->begin()
        );
        llvm::Argument* Arg = FuncArgIt;
        llvm::Value* ArgVarPtr = compile_scope->alloca_builder.CreateAlloca(Arg->getType());
        compile_scope->builder.CreateStore(Arg, ArgVarPtr);
        // fill compile_scope
        auto name = param_func_type->params[count].name.value();
        inner_compile_scope->addAlloca(name, ArgVarPtr);
        inner_compile_scope->addType(name, FuncLLVMTypeIt[count]);
        FuncArgIt++;
        count++;
    }

    for (auto label : this->_labels) {
        llvm::BasicBlock* labeledBlock = llvm::BasicBlock::Create(
            inner_compile_scope->ctx, *label + "_BLOCK", inner_compile_scope->function.value()
        );
        inner_compile_scope->addLabeledBlock(label, labeledBlock);
    }

    this->_block.compile(inner_compile_scope);

    if (compile_scope->builder.GetInsertBlock()->getTerminator() == nullptr) {
        llvm::Type* CurFuncReturnType = compile_scope->builder.getCurrentFunctionReturnType();
        if (CurFuncReturnType->isVoidTy()) {
            compile_scope->builder.CreateRetVoid();
        } else {
            compile_scope->builder.CreateRet(llvm::Constant::getNullValue(CurFuncReturnType));
        }
    }
}

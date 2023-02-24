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
    auto llvm_type = this->_type->toLLVMType(compile_scope);

    std::string function_name(*(this->_declaration.declarator->getName().value()));

    // Get the function from the compile scope if it was already declared, otherwise create a new one
    llvm::Function* llvm_function = compile_scope->module.getFunction(function_name);
    if (llvm_function == nullptr) {
        llvm_function =
            llvm::Function::Create(llvm_type, llvm::GlobalValue::ExternalLinkage, function_name, compile_scope->module);
    }

    auto inner_compile_scope = std::make_shared<CompileScope>(compile_scope, llvm_function);


    if (this->_type->has_params) {
        // Set the parameter names
        auto llvm_arg_iter = llvm_function->arg_begin();
        int param_count = 0;

        auto param_function_type = std::static_pointer_cast<ParamFunctionType>(this->_type);

        while (llvm_arg_iter != llvm_function->arg_end()) {
            llvm::Argument* llvm_arg = llvm_arg_iter;
            auto arg_name = param_function_type->params[param_count].name;
            if (arg_name.has_value()) {
                llvm_arg->setName(*arg_name.value());
            } else {
                llvm_arg->setName("");
            }
            llvm_arg_iter++;
            param_count++;
        }
    }

    llvm::BasicBlock* function_entry_block = llvm::BasicBlock::Create(compile_scope->ctx, "entry", llvm_function);
    compile_scope->builder.SetInsertPoint(function_entry_block);
    compile_scope->alloca_builder.SetInsertPoint(function_entry_block);

    if (this->_type->has_params) {
        // Store all the function arguments in allocas
        auto llvm_arg_iter = llvm_function->arg_begin();
        auto llvm_type_iter = llvm_type->param_begin();
        int param_count = 0;

        auto param_function_type = std::static_pointer_cast<ParamFunctionType>(this->_type);

        while (llvm_arg_iter != llvm_function->arg_end()) {
            compile_scope->alloca_builder.SetInsertPoint(
                compile_scope->alloca_builder.GetInsertBlock(), compile_scope->alloca_builder.GetInsertBlock()->begin()
            );
            llvm::Argument* llvm_arg = llvm_arg_iter;
            llvm::Value* llvm_arg_ptr = compile_scope->alloca_builder.CreateAlloca(llvm_arg->getType());
            compile_scope->builder.CreateStore(llvm_arg, llvm_arg_ptr);

            // Add parameter to compile scope
            auto name = param_function_type->params[param_count].name.value();
            inner_compile_scope->addAlloca(name, llvm_arg_ptr);
            inner_compile_scope->addType(name, llvm_type_iter[param_count]);

            llvm_arg_iter++;
            param_count++;
        }
    }

    for (auto label : this->_labels) {
        llvm::BasicBlock* labeled_block = llvm::BasicBlock::Create(
            inner_compile_scope->ctx, *label + "_BLOCK", inner_compile_scope->function.value()
        );
        inner_compile_scope->addLabeledBlock(label, labeled_block);
    }

    this->_block.compile(inner_compile_scope);

    if (compile_scope->builder.GetInsertBlock()->getTerminator() == nullptr) {
        llvm::Type* llvm_return_type = compile_scope->builder.getCurrentFunctionReturnType();
        if (llvm_return_type->isVoidTy()) {
            compile_scope->builder.CreateRetVoid();
        } else {
            compile_scope->builder.CreateRet(llvm::Constant::getNullValue(llvm_return_type));
        }
    }
}

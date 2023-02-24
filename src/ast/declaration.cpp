#include "declaration.h"

#include "types.h"

std::ostream& operator<<(std::ostream& stream, Declaration& declaration) {
    declaration.print(stream);
    return stream;
}

void Declaration::print(std::ostream& stream) {
    stream << this->specifier;

    auto declarator_empty = this->declarator->kind == DeclaratorKind::PRIMITIVE && this->declarator->isAbstract();

    if (!declarator_empty) {
        stream << ' ' << this->declarator;
    }
}

void Declaration::typecheck(ScopePtr& scope) {
    auto decl = this->toType(scope);
    if (this->declarator->isAbstract()) {
        if (decl.type->kind != TypeKind::TY_STRUCT) {
            errorloc(this->loc, "Declaration without declarator");
        }
        this->_typeDecl = decl;
        return;
    }
    // If this declares a function, check if there are duplicate parameter names (and if so, throw an error)
    if (decl.type->kind == TypeKind::TY_FUNCTION) {
        auto function_type = std::static_pointer_cast<FunctionType>(decl.type);
        auto function_scope = std::make_shared<Scope>(scope);
        if (function_type->has_params) {
            auto param_function = std::static_pointer_cast<ParamFunctionType>(function_type);
            for (auto& param : param_function->params) {
                if (!param.isAbstract() && function_scope->addDeclaration(param, true)) {
                    errorloc(this->declarator->loc, "parameter names have to be unique");
                }
            }
        }
    }
    if (scope->addDeclaration(decl)) {
        errorloc(this->declarator->loc, "Duplicate variable");
    }
    this->_typeDecl = decl;
}

TypeDecl Declaration::getTypeDecl() {
    return this->_typeDecl.value();
}

TypeDecl Declaration::toType(ScopePtr& scope) {
    auto name = this->declarator->getName();
    auto type = this->declarator->wrapType(this->specifier->toType(scope), scope);
    return TypeDecl(name, type);
}

void Declaration::compile(CompileScopePtr compile_scope) {
    // For abstract declarators, we generally don't need to generate any code
    if (this->declarator->isAbstract()) {
        // However, if it's a struct, we need to add it to the declared structs
        if (this->specifier->kind == SpecifierKind::STRUCT) {
            std::shared_ptr<Type> type = this->getTypeDecl().type;
            type->toLLVMType(compile_scope);
        }
        return;
    }

    auto type_decl = this->getTypeDecl();
    TypePtr type = type_decl.type;
    auto name = type_decl.name.value();

    if (type->kind == TypeKind::TY_FUNCTION) {
        // If the declaration declares a function, create the new LLVM function
        std::shared_ptr<FunctionType> function_type = std::static_pointer_cast<FunctionType>(type);
        auto llvm_type = function_type->toLLVMType(compile_scope);

        llvm::Function::Create(llvm_type, llvm::GlobalValue::ExternalLinkage, *name, compile_scope->module);
    } else {
        // Otherwise, declare a variable
        llvm::Type* llvm_type = type->toLLVMType(compile_scope);
        compile_scope->addType(name, llvm_type);

        /* Create a global variable */
        new llvm::GlobalVariable(
            compile_scope->module,
            llvm_type,
            /* isConstant: */ false,
            llvm::GlobalValue::CommonLinkage,
            /* initializer: */ llvm::Constant::getNullValue(llvm_type),
            *name
        );
    }
}

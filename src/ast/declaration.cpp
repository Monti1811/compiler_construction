#include "declaration.h"

#include "types.h"

std::ostream& operator<<(std::ostream& stream, Declaration& declaration) {
    declaration.print(stream);
    return stream;
}

void Declaration::print(std::ostream& stream) {
    stream << this->_specifier;

    auto declarator_empty = this->_declarator->kind == DeclaratorKind::PRIMITIVE && this->_declarator->isAbstract();

    if (!declarator_empty) {
        stream << ' ' << this->_declarator;
    }
}

void Declaration::typecheck(ScopePtr& scope) {
    auto decl = this->toType(scope);
    if (this->_declarator->isAbstract()) {
        if (decl.type->kind != TypeKind::TY_STRUCT) {
            errorloc(this->_loc, "Declaration without declarator");
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
                    errorloc(this->_declarator->loc, "parameter names have to be unique");
                }
            }
        }
    }
    if (scope->addDeclaration(decl)) {
        errorloc(this->_declarator->loc, "Duplicate variable");
    }
    this->_typeDecl = decl;
}

TypeDecl Declaration::getTypeDecl() {
    return this->_typeDecl.value();
}

TypeDecl Declaration::toType(ScopePtr& scope) {
    auto name = this->_declarator->getName();
    auto type = this->_declarator->wrapType(this->_specifier->toType(scope), scope);
    return TypeDecl(name, type);
}

void Declaration::compile(std::shared_ptr<CompileScope> compile_scope_ptr) {
    // does not declare a variable
    if (this->_declarator->isAbstract()) {
        // If it's a struct, add it to the declared structs
        if (this->_specifier->_kind == SpecifierKind::STRUCT) {
            std::shared_ptr<Type> type = this->getTypeDecl().type;
            type->toLLVMType(compile_scope_ptr->_Builder, compile_scope_ptr->_Ctx);
        }
        return;
    }

    // The same thing as for concrete function definitions
    auto type_decl = this->getTypeDecl();
    std::shared_ptr<Type> type = type_decl.type;
    auto name = type_decl.name.value();

    if (type->kind == TypeKind::TY_FUNCTION) {
        std::shared_ptr<FunctionType> func_type_ptr = std::static_pointer_cast<FunctionType>(type);
        auto llvm_type = func_type_ptr->toLLVMType(compile_scope_ptr->_Builder, compile_scope_ptr->_Ctx);

        llvm::Function::Create(
            llvm_type /* FunctionType *Ty */,
            llvm::GlobalValue::ExternalLinkage /* LinkageType */,
            *name /* const Twine &N="" */,
            compile_scope_ptr->_Module /* Module *M=0 */
        );
    } else {
        llvm::Type* llvm_type = type->toLLVMType(compile_scope_ptr->_Builder, compile_scope_ptr->_Ctx);

        compile_scope_ptr->addType(name, llvm_type);

        /* Create a global variable */
        new llvm::GlobalVariable(
            compile_scope_ptr->_Module /* Module & */,
            llvm_type /* Type * */,
            false /* bool isConstant */,
            llvm::GlobalValue::CommonLinkage /* LinkageType */,
            llvm::Constant::getNullValue(llvm_type) /* Constant * Initializer */,
            *name /* const Twine &Name = "" */,
            /* --------- We do not need this part (=> use defaults) ---------- */
            0 /* GlobalVariable *InsertBefore = 0 */,
            llvm::GlobalVariable::NotThreadLocal /* ThreadLocalMode TLMode = NotThreadLocal */,
            0 /* unsigned AddressSpace = 0 */,
            false /* bool isExternallyInitialized = false */
        );
    }
}

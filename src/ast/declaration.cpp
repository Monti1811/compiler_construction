#include "declaration.h"

#include "types.h"

std::ostream& operator<<(std::ostream& stream, Declaration& declaration) {
    declaration.print(stream);
    return stream;
}

void Declaration::print(std::ostream& stream) {
    stream << this->_specifier;

    auto declarator_empty =
        this->_declarator->kind == DeclaratorKind::PRIMITIVE && this->_declarator->isAbstract();

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
    std::shared_ptr<Type> type = this->getTypeDecl().type;
    llvm::Type* llvm_type = type->toLLVMType(compile_scope_ptr->_Builder, compile_scope_ptr->_Ctx);

    auto isFunction = this->_declarator->kind == DeclaratorKind::FUNCTION || llvm_type->isFunctionTy();
    if (isFunction) {
        std::shared_ptr<FunctionType> func_type_ptr = std::static_pointer_cast<FunctionType>(type);
        
        auto llvm_type = 
        !func_type_ptr->has_params 
        ?
        func_type_ptr->toLLVMType(compile_scope_ptr->_Builder, compile_scope_ptr->_Ctx) 
        :
        std::static_pointer_cast<ParamFunctionType>(func_type_ptr)->toLLVMType(compile_scope_ptr->_Builder, compile_scope_ptr->_Ctx);

        auto name = this->_declarator->getName().value();
        std::string _name(*name);
        llvm::Function *Func = llvm::Function::Create(
            llvm_type                                       /* FunctionType *Ty */,
            llvm::GlobalValue::ExternalLinkage              /* LinkageType */,
            _name                                           /* const Twine &N="" */,
            compile_scope_ptr->_Module                      /* Module *M=0 */
        );
            
        llvm::Function::arg_iterator FuncArgIt = Func->arg_begin();
        if (func_type_ptr->has_params) {
            int count = 0;
            auto param_func_type = std::static_pointer_cast<ParamFunctionType>(func_type_ptr);
            while (FuncArgIt != Func->arg_end()) {
                llvm::Argument *Arg = FuncArgIt;
                if (param_func_type->params[count].name.has_value()) {
                    Arg->setName(*(param_func_type->params[count].name.value()));
                } else {
                    Arg->setName("");
                }
                FuncArgIt++;
                count++;
            }
        }
    } else {
        auto name = this->_declarator->getName().value();

        compile_scope_ptr->addType(name, llvm_type);

        /* Create a global variable */
        new llvm::GlobalVariable(
            compile_scope_ptr->_Module                      /* Module & */,
            llvm_type                                       /* Type * */,
            false                                           /* bool isConstant */,
            llvm::GlobalValue::CommonLinkage                /* LinkageType */,
            llvm::Constant::getNullValue(llvm_type)         /* Constant * Initializer */,
            *name                                            /* const Twine &Name = "" */,
            /* --------- We do not need this part (=> use defaults) ---------- */
            0                                               /* GlobalVariable *InsertBefore = 0 */,
            llvm::GlobalVariable::NotThreadLocal            /* ThreadLocalMode TLMode = NotThreadLocal */,
            0                                               /* unsigned AddressSpace = 0 */,
            false                                           /* bool isExternallyInitialized = false */
        );
    }
}

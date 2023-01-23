#include "program.h"

#include "llvm/IR/Module.h"                /* Module */
#include "llvm/IR/Function.h"              /* Function */
#include "llvm/IR/IRBuilder.h"             /* IRBuilder */
#include "llvm/IR/LLVMContext.h"           /* LLVMContext */
#include "llvm/IR/GlobalValue.h"           /* GlobaleVariable, LinkageTypes */
#include "llvm/IR/Verifier.h"              /* verifyFunction, verifyModule */
#include "llvm/Support/Signals.h"          /* Nice stacktrace output */
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

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
        errorloc(this->_declaration._loc, "Internal error: Expected function definition to have function type");
    }

    auto function_type = std::static_pointer_cast<FunctionType>(function.type);

    // Add this function's signature to the scope given as an argument
    if (scope->addFunctionDeclaration(function)) {
        errorloc(this->_declaration._loc, "Duplicate function");
    }

    // Create inner function scope and add function arguments
    auto function_scope = function_type->scope;
    function_scope->setLabels(this->_labels);

    // 6.9.1.3: The return type of a function shall be void or a complete object type other than array type.
    auto return_type = function_type->return_type;
    if (return_type->kind != TypeKind::TY_VOID && !(return_type->isObjectType() && return_type->isComplete())) {
        errorloc(this->_declaration._declarator->loc, "Function return type must be void or a complete object type");
    }

    if (function_type->has_params) {
        auto param_function = std::static_pointer_cast<ParamFunctionType>(function_type);

        for (auto& param : param_function->params) {
            if (param.isAbstract()) {
                errorloc(this->_declaration._declarator->loc, "parameters must not be abstract");
            }
            if (function_scope->addDeclaration(param)) {
                errorloc(this->_declaration._declarator->loc, "parameter names have to be unique");
            }
        }
    }

    this->_block.typecheckInner(function_scope);
    // for compilation
    this->type = function_type;
}

void Program::addDeclaration(Declaration declaration) {
    this->_declarations.push_back(std::move(declaration));
    this->_is_declaration.push_back(true);
}

void Program::addFunctionDefinition(FunctionDefinition definition) {
    this->_functions.push_back(std::move(definition));
    this->_is_declaration.push_back(false);
}

std::ostream& operator<<(std::ostream& stream, Program& program) {
    program.print(stream);
    return stream;
}

void Program::print(std::ostream& stream) {
    auto decl_iter = this->_declarations.begin();
    auto func_iter = this->_functions.begin();

    bool first = true;

    for (bool is_decl : this->_is_declaration) {
        if (first) {
            first = false;
        } else {
            stream << "\n\n";
        }

        if (is_decl) {
            if (decl_iter == this->_declarations.end()) {
                error("Internal error: Tried to read non-existent declaration");
            }
            stream << *decl_iter.base() << ';';
            decl_iter++;
        } else {
            if (func_iter == this->_functions.end()) {
                error("Internal error: Tried to read non-existent function definition");
            }
            stream << *func_iter.base();
            func_iter++;
        }
    }
}

void Program::typecheck() {
    auto decl_iter = this->_declarations.begin();
    auto func_iter = this->_functions.begin();

    auto scope = std::make_shared<Scope>();

    for (bool is_decl : this->_is_declaration) {
        if (is_decl) {
            if (decl_iter == this->_declarations.end()) {
                error("Internal error: Tried to read non-existent declaration");
            }
            decl_iter.base()->typecheck(scope);
            decl_iter++;
        } else {
            if (func_iter == this->_functions.end()) {
                error("Internal error: Tried to read non-existent function definition");
            }
            func_iter.base()->typecheck(scope);
            func_iter++;
        }
    }
}

void Program::compile(int argc, char const* argv[], std::string filename) {
    // change filename "x/y.c" to "x.ll"
    std::string filename_to_print("");
    for (size_t i = 0; i < filename.length(); i++) {
        if (filename.at(i) == '/') {
            filename_to_print = "";
        } else if (filename.at(i) == '.') {
            if (filename.substr(i, filename.length() - i) == ".c") {
                filename_to_print += ".ll";
                break;
            }
        } else {
            filename_to_print += filename.at(i);
        }
    }

    llvm::sys::PrintStackTraceOnErrorSignal(filename);
    llvm::PrettyStackTraceProgram X(argc, argv);

    /* Make a global context (only one needed) */
    llvm::LLVMContext Ctx;

     /* Create a Module (only one needed) */
    llvm::Module M(filename, Ctx);

    /* Two IR-Builder to output intermediate instructions but also types, ... */
    llvm::IRBuilder<> Builder(Ctx), AllocaBuilder(Ctx);
    auto decl_iter = this->_declarations.begin();
    auto func_iter = this->_functions.begin();

    for (bool is_decl : this->_is_declaration) {
        if (is_decl) {
            std::cout << "compiling decl\n";
            if (decl_iter == this->_declarations.end()) {
                error("Internal error: Tried to read non-existent declaration");
            }
            // does not declare a variable
            if (decl_iter.base()->_declarator->isAbstract()) {
                continue;
            }
            std::shared_ptr<Type> type = decl_iter.base()->getTypeDecl().type;
            auto llvm_type = type->toLLVMType(Builder, Ctx);
            auto name = decl_iter.base()->_declarator->getName().value();

            /* Create a global variable */
            new llvm::GlobalVariable(
                M                                               /* Module & */,
                llvm_type                                       /* Type * */,
                false                                           /* bool isConstant */,
                llvm::GlobalValue::CommonLinkage                /* LinkageType */,
                llvm::Constant::getNullValue(llvm_type)         /* Constant * Initializer */,
                *name                                            /* const Twine &Name = "" */,
                /* --------- We do not need this part (=> use defaults) ---------- */
                0                                               /* GlobalVariable *InsertBefore = 0 */,
                llvm::GlobalVariable::NotThreadLocal            /* ThreadLocalMode TLMode = NotThreadLocal */,
                0                                               /* unsigned AddressSpace = 0 */,
                false                                           /* bool isExternallyInitialized = false */);
            decl_iter++;
        } else {
            if (func_iter == this->_functions.end()) {
                error("Internal error: Tried to read non-existent function definition");
            }
            std::shared_ptr<FunctionType> func_type_ptr = func_iter.base()->getFunctionType();
            auto llvm_type = func_type_ptr->toLLVMType(Builder, Ctx);
            auto name = func_iter.base()->_declaration._declarator->getName().value();
            llvm::Function *Func = llvm::Function::Create(
                llvm_type                                       /* FunctionType *Ty */,
                llvm::GlobalValue::ExternalLinkage              /* LinkageType */,
                *name                                           /* const Twine &N="" */,
                &M                                              /* Module *M=0 */);
            llvm::Function::arg_iterator FuncArgIt = Func->arg_begin();
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
            llvm::BasicBlock *FuncEntryBB = llvm::BasicBlock::Create(
                Ctx                                     /* LLVMContext &Context */,
                "entry"                                 /* const Twine &Name="" */,
                Func                                    /* Function *Parent=0 */,
                0                                       /* BasicBlock *InsertBefore=0 */);
            Builder.SetInsertPoint(FuncEntryBB);
            AllocaBuilder.SetInsertPoint(FuncEntryBB);

            FuncArgIt = Func->arg_begin();
            while (FuncArgIt != Func->arg_end()) {
                AllocaBuilder.SetInsertPoint(AllocaBuilder.GetInsertBlock(),
                                AllocaBuilder.GetInsertBlock()->begin());
                llvm::Argument *Arg = FuncArgIt;
                llvm::Value *ArgVarPtr = AllocaBuilder.CreateAlloca(Arg->getType());
                Builder.CreateStore(Arg, ArgVarPtr);
                FuncArgIt++;
            }
            
            func_iter.base()->_block.compile(Builder, AllocaBuilder, M);

            if (Builder.GetInsertBlock()->getTerminator() == nullptr) {
                llvm::Type *CurFuncReturnType = Builder.getCurrentFunctionReturnType();
                if (CurFuncReturnType->isVoidTy()) {
                    Builder.CreateRetVoid();
                } else {
                    Builder.CreateRet(llvm::Constant::getNullValue(CurFuncReturnType));
                }
            }
            func_iter++;
        }
    }
    /* Ensure that we created a 'valid' module */
    verifyModule(M);
    
    std::error_code EC;
    llvm::raw_fd_ostream stream(filename_to_print, EC, llvm::sys::fs::OpenFlags::OF_Text);
    M.print(stream, nullptr); /* M is a llvm::Module */

    /* Dump the final module to std::cerr */
    M.dump();
}

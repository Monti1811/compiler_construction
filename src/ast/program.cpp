#include "program.h"

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

// change filename "x/y.c" to "x.ll"
std::string getCompilerOutputFilename(std::string filename) {
    auto slash = filename.find_last_of('/');

    if (slash != std::string::npos) {
        // There is a slash in the filename:
        // Trim everything up to and including the first slash
        filename = filename.substr(slash + 1);
    }

    auto dot = filename.find_last_of('.');

    if (dot != std::string::npos) {
        // There is a dot in the filename:
        // Trim the dot and everything after it
        filename = filename.substr(0, dot);
    }

    return filename + ".ll";
}

void Program::compile(int argc, char const* argv[], std::string filename) {
    std::string filename_to_print = getCompilerOutputFilename(filename);

    llvm::sys::PrintStackTraceOnErrorSignal(filename);
    llvm::PrettyStackTraceProgram X(argc, argv);

    /* Make a global context (only one needed) */
    llvm::LLVMContext Ctx;

    /* Create a Module (only one needed) */
    llvm::Module M(filename, Ctx);

    /* Two IR-Builder to output intermediate instructions but also types, ... */
    llvm::IRBuilder<> Builder(Ctx), AllocaBuilder(Ctx);
    auto compile_scope_ptr = std::make_shared<CompileScope>(Builder, AllocaBuilder, M, Ctx);
    auto decl_iter = this->_declarations.begin();
    auto func_iter = this->_functions.begin();

    for (bool is_decl : this->_is_declaration) {
        if (is_decl) {
            if (decl_iter == this->_declarations.end()) {
                error("Internal error: Tried to read non-existent declaration");
            }
            decl_iter.base()->compile(compile_scope_ptr);
            decl_iter++;
        } else {
            if (func_iter == this->_functions.end()) {
                error("Internal error: Tried to read non-existent function definition");
            }
            func_iter.base()->compile(compile_scope_ptr);
            func_iter++;
        }
    }
    /* Ensure that we created a 'valid' module */
    verifyModule(M);

    std::error_code EC;
    llvm::raw_fd_ostream stream(filename_to_print, EC, llvm::sys::fs::OpenFlags::OF_Text);
    M.print(stream, nullptr); /* M is a llvm::Module */
}

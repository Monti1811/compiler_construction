#include "program.h"

std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition) {
    definition.print(stream);
    return stream;
}

void FunctionDefinition::print(std::ostream& stream) {
    stream << this->_declaration << '\n';
    this->_block.print(stream);
}

void FunctionDefinition::typecheck(ScopePtr& scope) {
    // TODO: Add this function's signature to the scope given as an argument

    // TODO: New scope with function arguments
    this->_block.typecheck(scope);
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

    auto scope = std::make_unique<Scope>();

    for (bool is_decl : this->_is_declaration) {
        if (is_decl) {
            // TODO: Typecheck declaration
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

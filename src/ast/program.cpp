#include "program.h"

void Program::addDeclaration(Declaration declaration) {
    this->_declarations.push_back(declaration);
    this->_is_declaration.push_back(true);
}

void Program::addFunctionDefinition(FunctionDefinition definition) {
    this->_functions.push_back(definition);
    this->_is_declaration.push_back(false);
}

std::ostream& operator<<(std::ostream& stream, Program program) {
    program.print(stream);
    return stream;
}

void Program::print(std::ostream& stream) {
    auto decl_iter = this->_declarations.begin();
    auto func_iter = this->_functions.begin();

    for (bool is_decl : this->_is_declaration) {
        if (is_decl) {
            if (decl_iter == this->_declarations.end()) {
                error("Internal error: Tried to read non-existent declaration");
            }
            stream << decl_iter.base() << std::endl;
            decl_iter++;
        } else {
            if (func_iter == this->_functions.end()) {
                error("Internal error: Tried to read non-existent function definition");
            }
            stream << func_iter.base() << std::endl;
            func_iter++;
        }
    }
}

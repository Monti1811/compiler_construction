#include "program.h"

std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition) {
    definition.print(stream);
    return stream;
}

void FunctionDefinition::print(std::ostream& stream) {
    stream << this->_declaration << '\n';
    if (!this->isAbstract()) {
        this->_block.value().print(stream);
    }
}

void FunctionDefinition::typecheck(ScopePtr& scope) {
    // Add this function's signature to the scope given as an argument
    auto function = this->_declaration.toType(scope);
    auto duplicate = scope->addDeclaration(function.first, function.second);
    if (duplicate) {
        errorloc(this->_declaration._loc, "Duplicate function");
    }

    auto& decl = this->_declaration._declarator;
    if (decl->kind != DeclaratorKind::FUNCTION) {
        error("Internal error: Expected function definition to have a function declarator");
    }
    auto function_decl = static_cast<FunctionDeclarator*>(decl.get());

    if (this->isAbstract()) {
        // filter void params, maybe structs as well
        bool first_param_void = false;
        bool first_loop_iter = true;
        // first param: reject void, if decl isn't abstract (maybe not necessary)
        // reject any param if first param is void and reject any void
        for (auto& field : function_decl->_parameters) {
            auto param = field.toType(scope);
            if (first_loop_iter) {
                first_loop_iter = false;
                if (param.second->kind == TY_VOID) {
                    first_param_void = true;
                }
            } else {
                if (first_param_void) {
                    errorloc(field._loc, "there must not be additional params if first param is void");
                }
                if (param.second->kind == TY_VOID) {
                    errorloc(field._loc, "second or greater param must not be void");
                }
            }
        }
        return;
    }

    // Create inner function scope and add function arguments
    auto function_scope = std::make_shared<Scope>(scope, this->_labels);

    bool first_param_void = false;
    bool first_loop_iter = true;

    for (auto& field : function_decl->_parameters) {
        auto param = field.toType(scope);

        if (first_loop_iter) {
            first_loop_iter = false;
            // specials case if first param is of type void, else proceed normally
            if (param.second->kind == TY_VOID) {
                if (!field._declarator->isAbstract()) {
                    errorloc(field._loc, "param void must be abstract");
                }
                first_param_void = true;
                continue;
            }
        }

        if (first_param_void && !first_loop_iter) {
            errorloc(field._loc, "if first param is void there must not be additional params");
        }
        if (field._declarator->isAbstract()) {
            errorloc(field._loc, "param must not be abstract");
        }
        if (param.second->kind == TY_VOID) {
            errorloc(field._loc, "second or greater param must not be of type void");
        }
        if (function_scope->addDeclaration(param.first, param.second)) {
            errorloc(field._loc, "parameter names have to be unique");
        }
    }

    function_scope->addFunctionReturnType(function.second);
    this->_block.value().typecheck(function_scope);
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

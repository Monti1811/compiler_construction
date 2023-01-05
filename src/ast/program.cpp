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
    auto function = this->_declaration.toType(scope);

    auto function_name = function.first;
    auto function_def_type = function.second;

    auto function_type_opt = function_def_type->getFunctionType();
    if (!function_type_opt.has_value()) {
        errorloc(this->_declaration._loc, "Internal error: Expected function definition to be a function pointer");
    }
    auto function_type = function_type_opt.value();

    // Add this function's signature to the scope given as an argument
    bool duplicate = scope->addFunctionDeclaration(function_name, function_def_type);
    if (duplicate) {
        errorloc(this->_declaration._loc, "Duplicate function");
    }

    // Create inner function scope and add function arguments
    auto function_scope = std::make_shared<Scope>(scope, this->_labels);
    function_scope->setFunctionReturnType(function_type->return_type);

    // TODO: Check return type
    // 6.9.1.3: The return type of a function shall be void or a complete object type other than array type.

    if (function_type->has_params) {
        auto param_function = std::static_pointer_cast<ParamFunctionType>(function_type);

        for (auto& param : param_function->params) {
            if (param.name == nullptr) {
                errorloc(this->_declaration._declarator->loc, "parameters must not be abstract");
            }
            if (function_scope->addDeclaration(param.name, param.type)) {
                errorloc(this->_declaration._declarator->loc, "parameter names have to be unique");
            }
        }
    }

    this->_block.typecheckInner(function_scope);
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

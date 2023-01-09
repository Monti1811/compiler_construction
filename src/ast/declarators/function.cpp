#include "function.h"

void FunctionDeclarator::print(std::ostream& stream) {
    if (_name->isAbstract()) {
        stream << '(';
    } else {
        stream << '(' << _name << '(';
    }

    for (size_t i = 0; i < _parameters.size(); i++) {
        auto& par = _parameters[i];
        stream << par;
        if (i < _parameters.size() - 1) {
            stream << ", ";
        }
    }

    if (_name->isAbstract()) {
        stream << ')';
    } else {
        stream << "))";
    }
}

std::optional<Symbol> FunctionDeclarator::getName() {
    return this->_name->getName();
}

TypePtr FunctionDeclarator::wrapType(TypePtr const& type, ScopePtr& scope) {
    auto functionPointer = [](TypePtr func_type) {
        return std::make_shared<PointerType>(func_type);
    };

    if (this->_parameters.empty()) {
        auto function_type = std::make_shared<FunctionType>(type);
        return functionPointer(function_type);
    }

    auto function_type = std::make_shared<ParamFunctionType>(type);

    if (this->_parameters.size() == 1) {
        auto param = this->_parameters[0].toType(scope);

        if (param.type->kind == TypeKind::TY_VOID) {
            if (!param.isAbstract()) {
                errorloc(this->loc, "void function parameter must be abstract");
            }
            return functionPointer(function_type);
        }
    }
    auto function_scope = std::make_shared<Scope>(scope);

    for (auto& param_decl : this->_parameters) {
        auto param = param_decl.toType(function_scope);

        if (param.type->kind == TypeKind::TY_VOID) {
            errorloc(this->loc, "function parameters cannot be void, unless void is the only parameter");
        }
        function_type->addParameter(param);
    }

    return functionPointer(function_type);
}

bool FunctionDeclarator::isFunction() {
    return true;
}

void FunctionDeclarator::addParameter(Declaration param) {
    this->_parameters.push_back(std::move(param));
}

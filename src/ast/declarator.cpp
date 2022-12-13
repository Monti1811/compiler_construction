#include "declarator.h"
#include "statement.h"

std::ostream& operator<<(std::ostream& stream, const DeclaratorPtr& declarator) {
    declarator->print(stream);
    return stream;
}

bool Declarator::isAbstract() {
    return this->_abstract;
}

std::ostream& operator<<(std::ostream& stream, Declaration& declaration) {
    declaration.print(stream);
    return stream;
}

void Declaration::print(std::ostream& stream) {
    stream << _specifier;
    if (_declarator->kind == DeclaratorKind::POINTER || !_declarator->isAbstract()) {
        stream << ' ';
    }
    stream << _declarator;
}

void Declaration::typecheck(ScopePtr&) {
    // TODO: Add this declaration to scope
}

void PrimitiveDeclarator::print(std::ostream& stream) {
    if (!isAbstract()) {
        stream << *_ident;
    }
}

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

void FunctionDeclarator::addParameter(Declaration param) {
    this->_parameters.push_back(std::move(param));
}

void PointerDeclarator::print(std::ostream& stream) {
    stream << "(*" << _inner << ')';
}

std::ostream& operator<<(std::ostream& stream, const TypeSpecifierPtr& type) {
    type->print(stream);
    return stream;
}

void VoidSpecifier::print(std::ostream& stream) {
    stream << "void";
}

void CharSpecifier::print(std::ostream& stream) {
    stream << "char";
}

void IntSpecifier::print(std::ostream& stream) {
    stream << "int";
}

void StructSpecifier::print(std::ostream& stream) {
    stream << "struct";
    if (_tag.has_value()) {
        stream << ' ' << *_tag.value();
    } 

    if (_components.size() > 0) {
        IdentManager& ident = IdentManager::getInstance();
        stream << '\n' << ident << "{\n";
        ident.increaseCurrIdentation(1);
        for (auto& component : _components) {
            stream << ident << component << ";\n";
        }
        ident.decreaseCurrIdentation(1);
        stream << ident << '}';
    }
}

void StructSpecifier::addComponent(Declaration declaration) {
    this->_components.push_back(std::move(declaration));
}

#include "declarator.h"

std::ostream& operator<<(std::ostream& stream, const DeclaratorPtr& declarator) {
    declarator->print(stream);
    return stream;
}

void Declarator::makeAbstract() {
    this->_abstract = true;
}

bool Declarator::isAbstract() {
    return this->_abstract;
}

std::ostream& operator<<(std::ostream& stream, Declaration& declaration) {
    declaration.print(stream);
    return stream;
}

void Declaration::print(std::ostream& stream) {
    stream << _specifier << " (" << _declarator << ')';
}

void PrimitiveDeclarator::print(std::ostream& stream) {
    stream << *_ident;
}

void FunctionDeclarator::print(std::ostream& stream) {
    stream << _decl << '(';
    for (size_t i = 0; i < _parameters.size(); i++) {
        auto& par = _parameters[i];
        stream << par._specifier << ' ' << par._declarator;
        if (i < _parameters.size() - 1) {
            stream << ',';
        }
    }
    stream << ')';
}

void FunctionDeclarator::addParameter(Declaration param) {
    this->_parameters.push_back(std::move(param));
}

void PointerDeclarator::print(std::ostream& stream) {
    stream << "*" << _inner;
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
    // TODO print struct components
}

void StructSpecifier::addComponent(Declaration declaration) {
    this->_components.push_back(std::move(declaration));
}

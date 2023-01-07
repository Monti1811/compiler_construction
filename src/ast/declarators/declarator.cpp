#include "declarator.h"

std::ostream& operator<<(std::ostream& stream, const DeclaratorPtr& declarator) {
    declarator->print(stream);
    return stream;
}

bool Declarator::isAbstract() {
    return this->_abstract;
}

void PrimitiveDeclarator::print(std::ostream& stream) {
    if (!isAbstract()) {
        stream << *_ident.value();
    }
}

std::optional<Symbol> PrimitiveDeclarator::getName() {
    return this->_ident;
}

TypePtr PrimitiveDeclarator::wrapType(TypePtr const& type, ScopePtr&) {
    return type;
}

bool PrimitiveDeclarator::isAbstract() {
    return !this->_ident.has_value();
}

bool PrimitiveDeclarator::isFunction() {
    return false;
}

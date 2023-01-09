#include "pointer.h"

#include "../types.h"

void PointerDeclarator::print(std::ostream& stream) {
    stream << "(*" << _inner << ')';
}

std::optional<Symbol> PointerDeclarator::getName() {
    return this->_inner->getName();
}

TypePtr PointerDeclarator::wrapType(TypePtr const& type, ScopePtr& scope) {
    auto wrapped = std::make_shared<PointerType>(type);
    return this->_inner->wrapType(wrapped, scope);
}

bool PointerDeclarator::isFunction() {
    return this->_inner->isFunction();
}

#include "declaration.h"

std::ostream& operator<<(std::ostream& stream, Declaration& declaration) {
    declaration.print(stream);
    return stream;
}

void Declaration::print(std::ostream& stream) {
    stream << this->_specifier;

    auto declarator_empty =
        this->_declarator->kind == DeclaratorKind::PRIMITIVE && this->_declarator->isAbstract();

    if (!declarator_empty) {
        stream << ' ' << this->_declarator;
    }
}

void Declaration::typecheck(ScopePtr& scope) {
    auto decl = this->toType(scope);
    if (this->_declarator->isAbstract()) {
        if (decl.type->kind != TypeKind::TY_STRUCT) {
            errorloc(this->_loc, "Declaration without declarator");
        }
        return;
    }
    if (scope->addDeclaration(decl)) {
        errorloc(this->_declarator->loc, "Duplicate variable");
    }
    this->_typeDecl = decl;
}

TypeDecl Declaration::getTypeDecl() {
    return this->_typeDecl.value();
}

TypeDecl Declaration::toType(ScopePtr& scope) {
    auto name = this->_declarator->getName();
    auto type = this->_declarator->wrapType(this->_specifier->toType(scope), scope);
    return TypeDecl(name, type);
}



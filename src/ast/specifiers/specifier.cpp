#include "specifier.h"

std::ostream& operator<<(std::ostream& stream, const TypeSpecifierPtr& type) {
    type->print(stream);
    return stream;
}

void TypeSpecifier::print(std::ostream& stream) {
    switch (this->_kind) {
        case SpecifierKind::CHAR:
            stream << "char";
            return;
        case SpecifierKind::INT:
            stream << "int";
            return;
        case SpecifierKind::VOID:
            stream << "void";
            return;
        default:
            error("Internal error: Unknown specifier kind");
    }
}

TypePtr TypeSpecifier::toType(ScopePtr&) {
    switch (this->_kind) {
        case SpecifierKind::CHAR:
            return CHAR_TYPE;
        case SpecifierKind::INT:
            return INT_TYPE;
        case SpecifierKind::VOID:
            return VOID_TYPE;
        default:
            error("Internal error: Unknown specifier kind");
    }
}

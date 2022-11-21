#include "declarator.h"

std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<TypeSpecifier>& type) {
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
}

bool Declarator::isEmptyDeclarator() {
    // TODO
}

void FunctionDeclarator::addParameter(Declaration param) {
    // TODO
}

void StructSpecifier::addComponent(Declaration spec_decl) {
    // TODO
}

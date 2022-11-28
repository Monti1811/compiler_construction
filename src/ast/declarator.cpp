#include "declarator.h"

std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<TypeSpecifier>& type) {
    type->print(stream);
    return stream;
}


void Declaration::print(std::ostream& stream) {
    this->specifier.get()->print(stream); 
    // TODO 
    // this->declarator.get()->print(stream);
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

bool Declarator::isEmptyDeclarator() {
    // TODO
    return false;
}

void FunctionDeclarator::addParameter(std::unique_ptr<Declaration> param) {
    // TODO
}

void StructSpecifier::addComponent(std::unique_ptr<Declaration> spec_decl) {
    // TODO
}

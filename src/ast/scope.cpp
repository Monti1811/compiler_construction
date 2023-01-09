#include "scope.h"

std::optional<TypePtr> Scope::getVarType(Symbol ident) {
    if (this->vars.find(ident) == this->vars.end()) {
        if (!this->parent.has_value()) {
            return std::nullopt;
        }
        return this->parent.value()->getVarType(ident);
    }
    return this->vars.at(ident);
}

std::optional<std::shared_ptr<StructType>> Scope::getStructType(Symbol ident) {
    if (this->structs.find(ident) == this->structs.end()) {
        if (!this->parent.has_value()) {
            return std::nullopt;
        }
        return this->parent.value()->getStructType(ident);
    }
    return this->structs.at(ident);
}

bool Scope::isLabelDefined(Symbol label) {
    if (this->labels.find(label) == this->labels.end()) {
        if (!parent.has_value()) {
            return false;
        }
        return parent.value()->isLabelDefined(label);
    }
    return true;
}

void Scope::setLabels(std::unordered_set<Symbol> labels) {
    this->labels = labels;
}

bool Scope::addDeclaration(TypeDecl& decl) {
    if (decl.isAbstract()) {
        return false;
    }

    return !(this->vars.insert({ decl.name.value(), decl.type }).second);
}

bool Scope::addFunctionDeclaration(TypeDecl& decl) {
    if (decl.isAbstract()) {
        return false;
    }
    auto name = decl.name.value();

    // If function was already declared:
    // assert that the declared type is the same as the one for this definition
    auto decl_type = this->vars.find(name);
    if (decl_type != this->vars.end() && !(decl_type->second->equals(decl.type))) {
        return true;
    }

    // Add the function declaration to the scope's variables
    this->vars.insert({ name, decl.type });
    // Mark this function as defined - if it was already before, return true.
    bool function_newly_defined = this->defined_functions.insert(name).second;
    return !function_newly_defined;
}

bool Scope::isStructDefined(Symbol& name) {
    return this->structs.find(name) != this->structs.end() && this->structs.at(name)->isComplete();
}

bool Scope::addStruct(std::shared_ptr<StructType> type) {
    // TODO:
    // 6.7.2.1.8:
    // The presence of a struct-declaration-list in a struct-or-union-specifier declares a new type,
    // *within a translation unit*

    if (!type->tag.has_value()) {
        return false;
    }
    auto name = type->tag.value();

    if (this->isStructDefined(name)) {
        // The struct has already been completed in this scope
        return true;
    }

    this->structs.insert({ name, type });
    return false;
}

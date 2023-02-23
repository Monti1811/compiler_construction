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

std::optional<std::shared_ptr<CompleteStructType>> Scope::getCompleteStruct(StructType struct_type) {
    if (!struct_type.tag.has_value()) {
        return std::nullopt;
    }
    auto tag = struct_type.tag.value();
    if (this->structs.find(tag) != this->structs.end()) {
        auto found_type = this->structs.at(tag);
        if (struct_type.scope_counter == found_type->scope_counter && found_type->isComplete()) {
            return std::static_pointer_cast<CompleteStructType>(found_type);
        }
    }
    if (this->parent.has_value()) {
        return this->parent.value()->getCompleteStruct(struct_type);
    }
    return std::nullopt;
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

    auto var = this->vars.insert({ decl.name.value(), decl.type });
    auto var_type = var.first;
    auto is_var_new = var.second;

    if (this->_root) {
        // 6.9.2: External definitions may be redefined (for some reason)
        // Only throw an error if type is not the same as previous declaration
        if (var_type != this->vars.end()) {
            return !decl.type->strong_equals(var_type->second);
        } else {
            return false;
        }
    } else {
        return !is_var_new;
    }
}

bool Scope::addFunctionDeclaration(TypeDecl& decl) {
    if (decl.isAbstract()) {
        return false;
    }
    auto name = decl.name.value();

    // If function was already declared:
    // assert that the declared type is the same as the one for this definition
    auto decl_type = this->vars.find(name);
    if (decl_type != this->vars.end() && !(decl_type->second->strong_equals(decl.type))) {
        return true;
    }

    // Add the function declaration to the scope's variables
    this->vars.insert({ name, decl.type });
    // Mark this function as defined - if it was already before, return true.
    bool function_newly_defined = this->defined_functions.insert(name).second;
    return !function_newly_defined;
}

bool Scope::isFunctionDesignator(Symbol& name) {
    auto type = this->getVarType(name);
    return type.has_value() && type.value()->kind == TypeKind::TY_FUNCTION;
}

bool Scope::isStructDefined(Symbol& name) {
    return this->structs.find(name) != this->structs.end() && this->structs.at(name)->isComplete();
}

bool Scope::addStruct(std::shared_ptr<StructType> type) {
    if (!type->tag.has_value()) {
        return false;
    }
    auto name = type->tag.value();

    if (this->isStructDefined(name)) {
        // The struct has already been completed in this scope
        return true;
    }
    auto def_struct_it = this->structs.find(name);
    if (def_struct_it != this->structs.end()) {
        def_struct_it->second = type;
    } else {
        this->structs.insert({ name, type });
    }
    return false;
}

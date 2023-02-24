#include "scope.h"

std::optional<TypePtr> Scope::getVarType(Symbol ident) {
    if (this->_vars.find(ident) == this->_vars.end()) {
        if (this->_root) {
            return std::nullopt;
        }
        return this->_parent.value()->getVarType(ident);
    }
    return this->_vars.at(ident);
}

std::optional<std::shared_ptr<StructType>> Scope::getStructType(Symbol ident) {
    if (this->_structs.find(ident) == this->_structs.end()) {
        if (this->_root) {
            return std::nullopt;
        }
        return this->_parent.value()->getStructType(ident);
    }
    return this->_structs.at(ident);
}

std::optional<std::shared_ptr<CompleteStructType>> Scope::getCompleteStruct(StructType struct_type) {
    if (!struct_type.tag.has_value()) {
        return std::nullopt;
    }
    auto tag = struct_type.tag.value();
    if (this->_structs.find(tag) != this->_structs.end()) {
        auto found_type = this->_structs.at(tag);
        if (struct_type.scope_counter == found_type->scope_counter && found_type->isComplete()) {
            return std::static_pointer_cast<CompleteStructType>(found_type);
        }
    }
    if (this->_root) {
        return std::nullopt;
    }
    return this->_parent.value()->getCompleteStruct(struct_type);
}

bool Scope::isLabelDefined(Symbol label) {
    if (this->_labels.find(label) == this->_labels.end()) {
        if (this->_root) {
            return false;
        }
        return this->_parent.value()->isLabelDefined(label);
    }
    return true;
}

void Scope::setLabels(std::unordered_set<Symbol> labels) {
    this->_labels = labels;
}

bool Scope::addDeclaration(TypeDecl& decl, bool function_param) {
    if (decl.isAbstract()) {
        return false;
    }

    if (decl.type->kind == TypeKind::TY_FUNCTION && !this->_root && !function_param) {
        return this->_parent.value()->addDeclaration(decl);
    }

    auto var = this->_vars.insert({decl.name.value(), decl.type});
    auto var_type = var.first;
    auto is_var_new = var.second;

    if (this->_root) {
        // 6.9.2: External definitions may be redefined (for some reason)
        // Only throw an error if type is not the same as previous declaration
        if (var_type != this->_vars.end()) {
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
    auto decl_type = this->_vars.find(name);
    if (decl_type != this->_vars.end() && !(decl_type->second->strong_equals(decl.type))) {
        return true;
    }

    // Add the function declaration to the scope's variables
    this->_vars.insert({name, decl.type});

    // Mark this function as defined - if it was already defined before, return true.
    bool function_newly_defined = this->_defined_functions.insert(name).second;
    return !function_newly_defined;
}

bool Scope::isFunctionDesignator(Symbol& name) {
    auto type = this->getVarType(name);
    return type.has_value() && type.value()->kind == TypeKind::TY_FUNCTION;
}

bool Scope::isStructDefined(Symbol& name) {
    return this->_structs.find(name) != this->_structs.end() && this->_structs.at(name)->isComplete();
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

    auto found_struct = this->_structs.find(name);
    if (found_struct != this->_structs.end()) {
        found_struct->second = type;
    } else {
        this->_structs.insert({name, type});
    }
    return false;
}

#include "struct.h"

#include "../indentation.h"

void StructSpecifier::print(std::ostream& stream) {
    stream << "struct";
    if (_tag.has_value()) {
        stream << ' ' << *_tag.value();
    } 

    if (this->_components.has_value()) {
        IndentManager& indent = IndentManager::getInstance();
        stream << '\n' << indent << "{\n";
        indent.increaseCurrIndentation(1);
        for (auto& component : this->_components.value()) {
            stream << indent << component << ";\n";
        }
        indent.decreaseCurrIndentation(1);
        stream << indent << '}';
    }
}

TypePtr StructSpecifier::toType(ScopePtr& scope) {
    if (this->_tag.has_value() && !this->_components.has_value()) {
        // Try to retrieve already defined struct from scope
        auto type = scope->getStructType(this->_tag.value());

        if (type.has_value()) {
            return type.value();
        }
    }

    if (!this->_components.has_value()) {
        // Struct is incomplete
        auto type = std::make_shared<StructType>(this->_tag, scope->scope_counter);

        if (scope->addStruct(type)) {
            errorloc(this->_loc, "Cannot redefine already defined struct");
        }

        return type;
    }

    auto type = std::make_shared<CompleteStructType>(this->_tag, scope->scope_counter);

    for (auto& field_decl : this->_components.value()) {
        auto field = field_decl.toType(scope);

        if (!field.type->isComplete()) {
            errorloc(field_decl._loc, "Struct fields must be complete");
        }

        if (field.type->kind == TY_FUNCTION) {
            errorloc(field_decl._loc, "Struct fields cannot have function type");
        }

        if (field.type->kind == TY_STRUCT) {
            // We know that the child struct is complete, because all fields must be complete
            // and we already checked that above.
            auto child_struct = std::static_pointer_cast<CompleteStructType>(field.type);

            // 6.7.2.1.13:
            // An unnamed member whose type specifier is a structure specifier with no tag is called an
            // anonymous structure. [...] The members of an anonymous structure or union are considered
            // to be members of the containing structure or union.
            // This applies recursively if the containing structure or union is also anonymous.
            if (field.isAbstract() && !child_struct->tag.has_value()) {
                type->combineWith(*child_struct);
                continue;
            }
        }

        if (type->addField(field)) {
            errorloc(field_decl._loc, "duplicate field");
        }
    }

    if (!type->validateFields()) {
        errorloc(this->_loc, "Structs must have at least one named field, and must not have unnamed fields at the beginning");
    }

    if (scope->addStruct(type)) {
        errorloc(this->_loc, "Cannot redefine already defined struct");
    }

    return type;
}

void StructSpecifier::makeComplete() {
    this->_components = std::vector<Declaration>();
}

void StructSpecifier::addComponent(Declaration declaration) {
    this->_components.value().push_back(std::move(declaration));
}

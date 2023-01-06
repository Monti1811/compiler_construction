#include "declarator.h"
#include "statement.h"

std::ostream& operator<<(std::ostream& stream, const DeclaratorPtr& declarator) {
    declarator->print(stream);
    return stream;
}

bool Declarator::isAbstract() {
    return this->_abstract;
}

std::ostream& operator<<(std::ostream& stream, Declaration& declaration) {
    declaration.print(stream);
    return stream;
}

void Declaration::print(std::ostream& stream) {
    stream << _specifier;
    if (_declarator->kind == DeclaratorKind::POINTER || !_declarator->isAbstract()) {
        stream << ' ';
    }
    stream << _declarator;
}

void Declaration::typecheck(ScopePtr& scope) {
    auto decl = this->toType(scope);
    if (this->_declarator->isAbstract()) {
        return;
    }
    if (scope->addDeclaration(decl)) {
        errorloc(this->_declarator->loc, "Duplicate variable");
    }
}

TypeDecl Declaration::toType(ScopePtr& scope) {
    auto name = this->_declarator->getName();
    auto type = this->_declarator->wrapType(this->_specifier->toType(scope), scope);
    return TypeDecl(name, type);
}

void PrimitiveDeclarator::print(std::ostream& stream) {
    if (!isAbstract()) {
        stream << *_ident.value();
    }
}

void FunctionDeclarator::print(std::ostream& stream) {
    if (_name->isAbstract()) {
        stream << '(';
    } else {
        stream << '(' << _name << '(';
    }

    for (size_t i = 0; i < _parameters.size(); i++) {
        auto& par = _parameters[i];
        stream << par;
        if (i < _parameters.size() - 1) {
            stream << ", ";
        }
    }

    if (_name->isAbstract()) {
        stream << ')';
    } else {
        stream << "))";
    }
}

void FunctionDeclarator::addParameter(Declaration param) {
    this->_parameters.push_back(std::move(param));
}

void PointerDeclarator::print(std::ostream& stream) {
    stream << "(*" << _inner << ')';
}

std::ostream& operator<<(std::ostream& stream, const TypeSpecifierPtr& type) {
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
    if (_tag.has_value()) {
        stream << ' ' << *_tag.value();
    } 

    if (this->_components.has_value()) {
        IdentManager& ident = IdentManager::getInstance();
        stream << '\n' << ident << "{\n";
        ident.increaseCurrIdentation(1);
        for (auto& component : this->_components.value()) {
            stream << ident << component << ";\n";
        }
        ident.decreaseCurrIdentation(1);
        stream << ident << '}';
    }
}

void StructSpecifier::makeComplete() {
    this->_components = std::vector<Declaration>();
}

void StructSpecifier::addComponent(Declaration declaration) {
    this->_components.value().push_back(std::move(declaration));
}

TypePtr StructSpecifier::toType(ScopePtr& scope) {
    if (this->_tag.has_value()) {
        // Try to retrieve already defined struct from scope
        auto type = scope->getStructType(this->_tag.value());

        if (type.has_value() && type.value()->isComplete()) {
            if (this->_components.has_value()) {
                errorloc(this->_loc, "Cannot redefine already defined struct");
            }

            return type.value();
        }
    }

    if (!this->_components.has_value()) {
        // Struct is incomplete
        auto type = std::make_shared<StructType>(this->_tag);

        if (scope->addStruct(type)) {
            errorloc(this->_loc, "Duplicate struct");
        }

        return type;
    }

    auto type = std::make_shared<CompleteStructType>(this->_tag);

    for (auto& field_decl : this->_components.value()) {
        auto field_scope = std::make_shared<Scope>(); // TODO is this needed?
        auto field = field_decl.toType(field_scope);

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

            if (child_struct->isAnonymous()) {
                // Add fields of anonymous struct to the struct directly
                type->combineWith(*child_struct);
                continue;
            } else if (!type->isAnonymous() && !child_struct->isAnonymous()) {
                // Disallow recursive structs
                std::string this_tag(*type->tag.value());
                std::string child_tag(*child_struct->tag.value());
                if (strcmp(this_tag.c_str(), child_tag.c_str()) == 0) {
                    errorloc(field_decl._loc, "struct must not contain instance of itself");
                }
            }
        }

        if (type->addField(field)) {
            errorloc(field_decl._loc, "duplicate field");
        }
    }

    if (!type->hasNamedFields()) {
        errorloc(this->_loc, "Struct does not have any named fields");
    }

    if (scope->addStruct(type)) {
        errorloc(this->_loc, "Duplicate struct");
    }

    return type;
}

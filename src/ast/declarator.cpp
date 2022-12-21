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
    auto pair = this->toType(scope);
    std::cout << "type checking declaration : " << *(this->_declarator->getName()) << "\n";
    if (this->_declarator->isAbstract()) {
        return;
    }
    auto duplicate = scope->addDeclaration(pair.first, pair.second);
    if (duplicate) {
        errorloc(this->_declarator->loc, "Duplicate variable");
    }
}

std::pair<Symbol, TypePtr> Declaration::toType(ScopePtr& scope) {
    auto name = this->_declarator->getName();
    auto type = this->_declarator->wrapType(this->_specifier->toType(scope), scope);
    return std::make_pair(name, type);
}

void PrimitiveDeclarator::print(std::ostream& stream) {
    if (!isAbstract()) {
        stream << *_ident;
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

    if (_components.size() > 0) {
        IdentManager& ident = IdentManager::getInstance();
        stream << '\n' << ident << "{\n";
        ident.increaseCurrIdentation(1);
        for (auto& component : _components) {
            stream << ident << component << ";\n";
        }
        ident.decreaseCurrIdentation(1);
        stream << ident << '}';
    }
}

void StructSpecifier::addComponent(Declaration declaration) {
    this->_components.push_back(std::move(declaration));
}

TypePtr StructSpecifier::toType(ScopePtr& scope) {
    auto type = std::make_shared<StructType>();
    for (auto& field : this->_components) {
        auto pair = field.toType(scope);
        // check wether field is a struct
        if (pair.second->kind == TY_STRUCT) {
            // check if struct is not instantiated
            if (!field._declarator->getName()) {
                auto anonymous_struct = std::static_pointer_cast<StructType>(pair.second);
                if (!anonymous_struct->isAnonymous()) {
                    continue;
                }
                for (auto& struct_field : anonymous_struct->fields) {
                    // TODO: loc not quite correct
                    if (type->addField(struct_field.first, std::move(struct_field.second))) {
                        errorloc(field._loc, "duplicate field");
                    }
                }
                continue;
            }
        }
        if (field._declarator->isAbstract()) {
            errorloc(field._loc, "abstract field in a struct");
        }
        if (type->addField(pair.first, std::move(pair.second))) {
            errorloc(field._loc, "duplicate field");
        }
    }
    if (this->_tag.has_value()) {
        type->setTag(this->_tag.value());
        // check wether it's just a variable with struct type
        // TODO: know from the start
        if (type->fields.size() == 0) return type;
        auto duplicate = scope->addStruct(this->_tag.value(), type);
        if (duplicate && this->_components.size() > 0) {
            errorloc(this->_loc, "Duplicate struct");
        }
    } else {
        type->setAnonymous(true);
    }
    return type;
}

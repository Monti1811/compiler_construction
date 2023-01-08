#include "types.h"

std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<Type>& type) {
    type->print(stream);
    return stream;
}

void Type::print(std::ostream& stream) {
    switch (this->kind) {
        case TypeKind::TY_INT: {
            stream << "int";
            return;
        }
        case TypeKind::TY_VOID: {
            stream << "void";
            return;
        }
        case TypeKind::TY_CHAR: {
            stream << "char";
            return;
        }
        case TypeKind::TY_NULLPTR: {
            stream << "nullptr";
            return;
        }
        case TypeKind::TY_POINTER: {
            auto ptr_type = static_cast<PointerType*>(this);
            stream << "*(";
            ptr_type->inner->print(stream);
            stream << ")";
            return;
        }
        case TypeKind::TY_STRUCT: {
            auto struct_type = static_cast<StructType*>(this);
            stream << "struct";
            if (struct_type->isComplete()) {
                auto complete_type = static_cast<CompleteStructType*>(this);
                stream << " { ";
                for (auto& field : complete_type->fields) {
                    stream << field.type << " ";
                    if (field.name.has_value()) {
                        stream << *field.name.value();
                    } else {
                        stream << "<anon>";
                    }
                    stream << "; ";
                }
                stream << "}";
            }
            if (struct_type->tag.has_value()) {
                stream << " " << *struct_type->tag.value();
            }
            return;
        }
        case TypeKind::TY_FUNCTION: {
            auto fn_type = static_cast<FunctionType*>(this);
            stream << "fn (";
            if (fn_type->has_params) {
                auto par_fn_type = static_cast<ParamFunctionType*>(this);
                bool first = true;
                for (auto& param : par_fn_type->params) {
                    if (first) {
                        first = false;
                    } else {
                        stream << ", ";
                    }
                    stream << param.type;
                    if (param.name) {
                        stream << " " << *param.name;
                    }
                }
                stream << ")";
            }
            stream << " -> " << fn_type->return_type;
            return;
        }
    }
}

bool Type::equals(TypePtr const& other) {
    if (this->kind != other->kind) {
        if (
            (this->isInteger() && other->isInteger())
            || (this->kind == TY_NULLPTR && other->kind == TY_POINTER)
            || (this->kind == TY_POINTER && other->kind == TY_NULLPTR)
        ) {
            return true;
        }
        return false;
    }
    return true;
}

bool Type::strong_equals(TypePtr const& other) {
    if (this->kind != other->kind) {
        return false;
    }
    return true;
}

bool Type::isPointer() {
    return this->kind == TypeKind::TY_POINTER || this->kind == TypeKind::TY_NULLPTR;
}

bool Type::isInteger() {
    return this->kind == TypeKind::TY_INT || this->kind == TypeKind::TY_NULLPTR;
}

bool Type::isScalar() {
    switch (this->kind) {
        case TypeKind::TY_INT:
        case TypeKind::TY_CHAR:
        case TypeKind::TY_NULLPTR:
        case TypeKind::TY_POINTER:
            return true;
        default:
            return false;
    }
}

bool Type::isArithmetic() {
    switch (this->kind) {
        case TypeKind::TY_INT:
        case TypeKind::TY_CHAR:
        case TypeKind::TY_NULLPTR:
            return true;
        default:
            return false;
    }
}

bool Type::isObjectType() {
    switch (this->kind)
    {
        case TypeKind::TY_FUNCTION:
        case TypeKind::TY_VOID:
            return false;
        default:
            return true;
    }
}

bool Type::isComplete() {
    switch (this->kind) {
        case TypeKind::TY_VOID:
        case TypeKind::TY_STRUCT:
            return false;
        default:
            return true;
    }
}

std::optional<std::shared_ptr<FunctionType>> Type::getFunctionType() {
    if (this->kind != TypeKind::TY_POINTER) {
        return std::nullopt;
    }

    auto function_ptr_type = static_cast<PointerType*>(this);
    auto function_ptr_inner_type = function_ptr_type->inner;

    if (function_ptr_inner_type->kind != TypeKind::TY_FUNCTION) {
        return std::nullopt;
    }

    return std::static_pointer_cast<FunctionType>(function_ptr_inner_type);
}

bool PointerType::equals(TypePtr const& other) {
    // return true when other or this pointer is a nullptr
    if (other->kind == TypeKind::TY_NULLPTR || inner->kind == TypeKind::TY_VOID) {
        return true;
    }
    if (other->kind == TypeKind::TY_POINTER) {
        auto other_pointer = std::static_pointer_cast<PointerType>(other);
        if (other_pointer->inner->kind == TY_VOID) {
            return true;
        }
        return this->inner->equals(other_pointer->inner);
    } else {
        return false;
    }
}

bool PointerType::strong_equals(TypePtr const& other) {
    if (other->kind != TypeKind::TY_POINTER) {
        return false;
    }
    auto other_pointer = std::static_pointer_cast<PointerType>(other);
    return this->inner->strong_equals(other_pointer->inner);
}

bool StructType::equals(TypePtr const& other) {
    if (other->kind != TypeKind::TY_STRUCT) {
        return false;
    }
    auto other_structtype = std::static_pointer_cast<StructType>(other);

    if (other_structtype->tag != this->tag) {
        return false;
    }
    return true;
}

bool StructType::strong_equals(TypePtr const& other) {
    return this->equals(other);
}

bool CompleteStructType::isComplete() {
    return true;
}

bool CompleteStructType::addField(StructField field) {
    if (field.name.has_value() && !this->_field_names.insert({ field.name.value(), this->fields.size() }).second) {
        return true;
    }

    this->fields.push_back(field);
    return false;
}

bool CompleteStructType::combineWith(CompleteStructType const& other) {
    for (auto& field : other.fields) {
        if (this->addField(field)) {
            return true;
        }
    }
    return false;
}

bool CompleteStructType::validateFields() {
    // 6.7.2.1.8:
    // If the struct-declaration-list does not contain any
    // named members, either directly or via an anonymous structure or anonymous union, the
    // behavior is undefined.
    // (we throw an error because why not)

    if (this->_field_names.empty()) {
        return false;
    }

    // 6.7.2.1.15:
    // There may be unnamed padding within a structure object, but not at its beginning.
    if (this->fields.at(0).isAbstract()) {
        return false;
    } else {
        return true;
    }
}

std::optional<TypePtr> CompleteStructType::typeOfField(Symbol& ident) {
    if (this->_field_names.find(ident) == this->_field_names.end()) {
        return std::nullopt;
    }
    return this->fields.at(this->_field_names.at(ident)).type;
}

bool FunctionType::equals(TypePtr const& other) {
    if (other->kind != TypeKind::TY_FUNCTION) {
        return false;
    }

    auto other_type = std::static_pointer_cast<FunctionType>(other);
    return this->return_type->strong_equals(other_type->return_type);
}

bool FunctionType::strong_equals(TypePtr const& other) {
    return this->equals(other);
}

bool ParamFunctionType::equals(TypePtr const& other) {
    if (!FunctionType::equals(other)) {
        return false;
    }

    auto other_type = std::static_pointer_cast<ParamFunctionType>(other);

    if (this->params.size() != other_type->params.size()) {
        return false;
    }

    for (size_t i = 0; i < this->params.size(); i++) {
        auto param1 = this->params[i];
        auto param2 = other_type->params[i];
        if (!(param1.type->strong_equals(param2.type))) {
            return false;
        }
    }
    return true;
}

bool ParamFunctionType::strong_equals(TypePtr const& other) {
    return this->equals(other);
}

void ParamFunctionType::addParameter(FunctionParam const& param) {
    this->params.push_back(param);
}

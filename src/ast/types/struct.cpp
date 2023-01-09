#include "struct.h"

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

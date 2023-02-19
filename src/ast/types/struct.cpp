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

llvm::Type* StructType::toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx) {
    // declarations of structs should have CompleteStructType already, so this shouldn't get called at all
    auto struct_name = "struct." + *tag.value();
    auto def_structtype = llvm::StructType::getTypeByName(Ctx, struct_name);
    if (def_structtype) {
        return def_structtype;
    }
    llvm::StructType *StructXType = llvm::StructType::create(Ctx, struct_name);
    std::vector<llvm::Type *> StructMemberTypes;
    StructXType->setBody(StructMemberTypes);
    return StructXType;
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

llvm::Type* CompleteStructType::toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx) {
    if (!tag.has_value()) {
        return this->toLLVMTypeAnonymous(Builder, Ctx);
    }
    std::string struct_name = "struct." + *tag.value(); 
    
    // Check if struct already exists
    auto def_structtype = llvm::StructType::getTypeByName(Ctx, struct_name);
    if (def_structtype) {
        return def_structtype;
    }
    llvm::StructType *StructXType = llvm::StructType::create(Ctx, struct_name);
    std::vector<llvm::Type *> StructMemberTypes;
    for (auto field : this->fields) {
        if (field.type->kind == TY_STRUCT) {
            auto complete_struct_ty = std::static_pointer_cast<CompleteStructType>(field.type);
            complete_struct_ty->alt_tag = struct_name + '.' +  *field.name.value();
            if (!complete_struct_ty->tag.has_value()) {
                StructMemberTypes.push_back(complete_struct_ty->toLLVMTypeAnonymous(Builder, Ctx));
                continue;
            }
        }
        StructMemberTypes.push_back(field.type->toLLVMType(Builder, Ctx));
    }
    StructXType->setBody(StructMemberTypes);
    return StructXType;
}

llvm::Type* CompleteStructType::toLLVMTypeAnonymous(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx) {
    // Check if struct already exists
    auto def_structtype = llvm::StructType::getTypeByName(Ctx, this->alt_tag);
    if (def_structtype) {
        return def_structtype;
    }
    llvm::StructType *StructXType = llvm::StructType::create(Ctx, this->alt_tag);
    std::vector<llvm::Type *> StructMemberTypes;
    for (auto field : this->fields) {
        if (field.type->kind == TY_STRUCT) {
            auto complete_struct_ty = std::static_pointer_cast<CompleteStructType>(field.type);
            complete_struct_ty->alt_tag = this->alt_tag + '.' +  *field.name.value();
            if (!complete_struct_ty->tag.has_value()) {
                StructMemberTypes.push_back(complete_struct_ty->toLLVMTypeAnonymous(Builder, Ctx));
                continue;
            }
        }
        StructMemberTypes.push_back(field.type->toLLVMType(Builder, Ctx));
    }
    StructXType->setBody(StructMemberTypes);
    return StructXType;
}

size_t CompleteStructType::getIndexOfField(Symbol name) {
    // std::cerr << "looking for name " << *name << std::endl;
    if (this->_field_names.find(name) == this->_field_names.end()) {
        // std::cerr << "could not find field" << std::endl;
        return 0;
    }
    return this->_field_names.at(name);
}

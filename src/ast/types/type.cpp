#include "type.h"

#include "function.h"
#include "struct.h"
#include "pointer.h"

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
            if (struct_type->tag.has_value()) {
                stream << " " << *struct_type->tag.value();
            }
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
                        stream << " " << **param.name;
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

bool Type::isString() {
    if (this->kind != TypeKind::TY_POINTER) return false;
    auto pointer_type = static_cast<PointerType*>(this);
    return pointer_type->inner->kind == TypeKind::TY_CHAR;
}

std::optional<std::shared_ptr<FunctionType>> Type::unwrapFunctionPointer() {
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

llvm::Type* Type::toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx) {
    switch(this->kind) {
        case TY_INT:
            return Builder.getInt32Ty();
        case TY_CHAR:
            return Builder.getInt8Ty();
        case TY_STRUCT:
            if (this->isComplete()) {
                return (static_cast<CompleteStructType*>(this))->toLLVMType(Builder, Ctx);
            } else {
                return (static_cast<StructType*>(this))->toLLVMType(Builder, Ctx);
            }
            
        case TY_FUNCTION:
            return (static_cast<FunctionType*>(this))->toLLVMType(Builder, Ctx);
        case TY_NULLPTR:
        case TY_POINTER:
            return (static_cast<PointerType*>(this))->toLLVMType(Builder, Ctx);
        default:
            // TODO: to be done
            return Builder.getVoidTy();
    }
}

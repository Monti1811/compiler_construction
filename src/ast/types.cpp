#include "types.h"

std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<Type>& type) {
    type->print(stream);
    return stream;
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
            if (!struct_type->isAnonymous()) {
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

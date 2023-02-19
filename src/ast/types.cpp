#include "types.h"

std::optional<TypePtr> unifyTypes(TypePtr left_type, TypePtr right_type) {
    if (left_type->strong_equals(right_type)) {
        return std::nullopt;
    }

    auto left = left_type->kind;
    auto right = right_type->kind;

    if ((left == TypeKind::TY_CHAR && right == TypeKind::TY_INT)
        || (left == TypeKind::TY_INT && right == TypeKind::TY_CHAR)
    ) {
        return INT_TYPE;
    }

    if ((left == TypeKind::TY_INT && right == TypeKind::TY_NULLPTR) 
        || (left == TypeKind::TY_NULLPTR && right == TypeKind::TY_INT)
    ) {
        return INT_TYPE;
    }

    if ((left == TypeKind::TY_CHAR && right == TypeKind::TY_NULLPTR)
        || (left == TypeKind::TY_NULLPTR && right == TypeKind::TY_CHAR)
    ) {
        return INT_TYPE;
    }

    return std::nullopt;
}

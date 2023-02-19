#include "types.h"

std::optional<TypePtr> unifyTypes(TypePtr left_type, TypePtr right_type) {
    if (left_type->equals(right_type)) {
        return std::nullopt;
    }

    auto left = left_type->kind;
    auto right = right_type->kind;

    if (left == TypeKind::TY_CHAR && right == TypeKind::TY_INT) {
        return INT_TYPE;
    } else if (left == TypeKind::TY_INT && right == TypeKind::TY_CHAR) {
        return INT_TYPE;
    }

    return std::nullopt;
}

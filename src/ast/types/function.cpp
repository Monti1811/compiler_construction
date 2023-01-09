#include "function.h"

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

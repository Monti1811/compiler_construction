#include "expression.h"

// print functions

std::ostream& operator<<(std::ostream& stream, const ExpressionPtr& expr) {
    expr->print(stream);
    return stream;
}

void IdentExpression::print(std::ostream& stream) {
    stream << (*this->_ident);
}

std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<IdentExpression>& expr) {
    expr->print(stream);
    return stream;
}

void IntConstantExpression::print(std::ostream& stream) {
    stream << this->_value;
}

void NullPtrExpression::print(std::ostream& stream) {
    stream << this->_value;
} 

void CharConstantExpression::print(std::ostream& stream) {
    stream << this->_value;
}

void StringLiteralExpression::print(std::ostream& stream) {
    stream << this->_value;
}

void IndexExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '[' << this->_index << "])";
}

void CallExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '(';

    const int length = this->_arguments.size();
    for (int i = 0; i < length; i++) {
        this->_arguments.at(i)->print(stream);
        if (i != length - 1) {
            stream << ", ";
        }
    }

    stream << "))";
}

void DotExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '.' << this->_ident << ')';
}

void ArrowExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << "->" << this->_ident << ')';
}

void UnaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_op_str << this->_inner << ')';
}

void SizeofTypeExpression::print(std::ostream& stream) {
    stream << "(sizeof(" << this->_type << "))";
}

void BinaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_left << ' ' << this->_op_str << ' ' << this->_right << ')';
}

void TernaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_condition << " ? " << this->_left << " : " << this->_right << ')';
}

// typecheck functions

TypePtr IdentExpression::typecheck(ScopePtr& scope) {
        auto type = scope->getVarType(this->_ident);
        if (!type.has_value()) {
            errorloc(this->loc, "Variable ", *this->_ident, " is not defined");
        }
        return type.value();
    }

TypePtr IntConstantExpression::typecheck(ScopePtr&) { return INT_TYPE; }

TypePtr CharConstantExpression::typecheck(ScopePtr&) { return CHAR_TYPE; }

TypePtr StringLiteralExpression::typecheck(ScopePtr&) { return STRING_TYPE; }

TypePtr NullPtrExpression::typecheck(ScopePtr&) { return NULLPTR_TYPE; }

TypePtr IndexExpression::typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheck(scope);
        auto index_type = this->_index->typecheck(scope);

        TypePtr pointer_type;

        if (expr_type->isPointer() && index_type->isInteger()) {
            // Regular index expression: arr[0]
            pointer_type = expr_type;
        } else if (expr_type->isInteger() && index_type->isPointer()) {
            // Swapped index expression: 0[arr]
            pointer_type = index_type;
        } else {
            errorloc(this->loc, "Index expressions must consist of a pointer and an integer");
        }

        TypePtr indexed_type;

        if (pointer_type->kind == TypeKind::TY_NULLPTR) {
            indexed_type = VOID_TYPE;
        } else {
            indexed_type = std::static_pointer_cast<PointerType>(pointer_type)->inner;
        }

        if (!indexed_type->isComplete() || !indexed_type->isObjectType()) {
            errorloc(this->loc, "Cannot index an incomplete or non-object type");
        }
        
        return indexed_type;
    }

TypePtr CallExpression::typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheck(scope);
        auto function_type_opt = expr_type->getFunctionType();
        if (!function_type_opt.has_value()) {
            errorloc(this->_expression->loc, "Call epression needs to be called on a function pointer");
        }
        auto function_type = function_type_opt.value();

        // A function without specified parameters can be called with any arguments
        if (!function_type->has_params) {
            for (auto& arg : this->_arguments) {
                arg->typecheck(scope);
            }

            return function_type->return_type;
        }

        auto param_function_type = std::static_pointer_cast<ParamFunctionType>(function_type);
        auto params = param_function_type->params;

        if (this->_arguments.size() != params.size()) {
            errorloc(this->loc, "Incorrect number of arguments");
        }

        for (size_t i = 0; i < params.size(); i++) {
            auto arg_type = this->_arguments[i]->typecheck(scope);
            if (!arg_type->equals(params[i].type)) {
                errorloc(this->_arguments[i]->loc, "Incorrect argument type");
            }
        }

        return function_type->return_type;
    }

TypePtr DotExpression::typecheck(ScopePtr& scope) {
        auto expr_type = _expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_STRUCT) {
            errorloc(this->loc, "Cannot access a field of a non-struct expression");
        }
        if (!expr_type->isComplete()) {
            errorloc(this->loc, "Cannot access a field of an incomplete type");
        }
        auto struct_type = std::static_pointer_cast<CompleteStructType>(expr_type);

        auto ident = this->_ident->_ident;
        auto field_type = struct_type->typeOfField(ident);

        if (!field_type.has_value()) {
            errorloc(this->loc, "Field " + *ident + " does not exist on this struct");
        }
        return field_type.value();
    }

TypePtr ArrowExpression::typecheck(ScopePtr& scope) {
        auto expr_type = _expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Cannot access non-pointers using the arrow operator");
        }
        auto pointer_type = std::static_pointer_cast<PointerType>(expr_type);

        if (pointer_type->inner->kind != TypeKind::TY_STRUCT) {
            errorloc(this->loc, "Cannot access a field of a non-struct expression");
        }
        if (!pointer_type->inner->isComplete()) {
            errorloc(this->loc, "Cannot access a field of an incomplete type");
        }
        auto struct_type = std::static_pointer_cast<CompleteStructType>(pointer_type->inner);

        auto ident = this->_ident->_ident;
        auto field_type = struct_type->typeOfField(ident);

        if (!field_type.has_value()) {
            errorloc(this->loc, "Field " + *ident + " does not exist on this struct");
        }
        return field_type.value();
    }

TypePtr SizeofExpression::typecheck(ScopePtr& scope) {
        auto inner_type = this->_inner->typecheck(scope);

        if (inner_type->kind == TY_FUNCTION) {
            errorloc(this->_inner->loc, "inner of a sizeof expression must not have function type");
        }
        if (inner_type->kind == TY_STRUCT) {
            auto struct_type = std::static_pointer_cast<StructType>(inner_type);
            if (!struct_type->isComplete()) {
                errorloc(this->_inner->loc, "inner of sizeof expression must not have incomplete type");
            }
        }
        return INT_TYPE;
    }

TypePtr SizeofTypeExpression::typecheck(ScopePtr&) {
        // TODO: Additional checks
        return INT_TYPE;
    }

TypePtr ReferenceExpression::typecheck(ScopePtr& scope) {
        auto inner_type = this->_inner->typecheck(scope);
        // indexExpression and pointerExpression are defined as l-values
        if (this->_inner->isLvalue()) {
            return std::make_shared<PointerType>(inner_type);
        } 
        // function designator allowed as well
        // function designator will be typchecked as a pointertype to a functiontype
        errorloc(this->loc, "expression to be referenced must be an l-value");
    }

TypePtr PointerExpression::typecheck(ScopePtr& scope) {
        auto inner_type = this->_inner->typecheck(scope);
        if (inner_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Cannot dereference a non-pointer");
        }
        auto pointer_type = std::static_pointer_cast<PointerType>(inner_type);
        return pointer_type->inner;
    }

TypePtr NegationExpression::typecheck(ScopePtr& scope) {
        auto inner_type = this->_inner->typecheck(scope);
        if (!inner_type->isArithmetic()) {
            errorloc(this->loc, "type to be negated has to be arithmetic");
        }
        return INT_TYPE;
    }

TypePtr LogicalNegationExpression::typecheck(ScopePtr& scope) {
        auto inner_type = _inner->typecheck(scope);
        if (!inner_type->isScalar()) {
            errorloc(this->loc, "type to be logcially negated has to be scalar");
        }
        return INT_TYPE;
    }

TypePtr BinaryExpression::typecheck(ScopePtr& scope) {
        auto left_type = _left->typecheck(scope);
        auto right_type = _right->typecheck(scope);

        if (!left_type->isArithmetic() || !right_type->isArithmetic()) {
            errorloc(this->loc, "both sides of an arithmetic binary expression must be of arithemic type");
        }
        return INT_TYPE;
    }

TypePtr AddExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheck(scope);
    auto right_type = _right->typecheck(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        return INT_TYPE;
    }
    if (left_type->kind == TY_POINTER && right_type->isArithmetic()) {
        return left_type;
    }
    if (left_type->isArithmetic() && right_type->kind == TY_POINTER) {
        return right_type;
    }

    errorloc(this->loc, "Illegal addition operation");
}

TypePtr SubstractExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheck(scope);
    auto right_type = _right->typecheck(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        return INT_TYPE;
    }
    if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER && left_type->equals(right_type)) {
        auto left_pointer = std::static_pointer_cast<PointerType>(left_type);
        auto right_pointer = std::static_pointer_cast<PointerType>(right_type);
        if ( !(left_pointer->inner->isObjectType() && right_pointer->inner->isObjectType()) ) {
            errorloc(this->loc, "both pointers have to point to object types");
        }
        if (left_pointer->inner->kind == TY_STRUCT && right_pointer->inner->kind == TY_STRUCT) {
            auto left_struct = std::static_pointer_cast<StructType>(left_pointer->inner);
            auto right_struct = std::static_pointer_cast<StructType>(right_pointer->inner);
            if (!left_struct->isComplete() || !right_struct->isComplete()) {
                errorloc(this->loc, "both pointers have to point to object complete types");
            }
        }
        return INT_TYPE;
    }
    // left side must be complete object type
    if (left_type->kind == TY_POINTER && right_type->isArithmetic()) {
        auto pointer_type = std::static_pointer_cast<PointerType>(left_type);
        // not an object type
        if (!pointer_type->inner->isObjectType()) {
            errorloc(this->loc, "Illegal substraction operation");
        }
        if (pointer_type->inner->kind == TY_STRUCT) {
            // not a complete object
            auto struct_type = std::static_pointer_cast<StructType>(pointer_type->inner);
            if (!struct_type->isComplete()) {
                 errorloc(this->loc, "Illegal substraction operation");
            }
        }
        return left_type;
    }

    errorloc(this->loc, "Illegal substraction operation");
}

TypePtr LessThanExpression::typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheck(scope);
        auto right_type = this->_right->typecheck(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare two values of different types");
        }
        return INT_TYPE;
    }

TypePtr EqualExpression::typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheck(scope);
        auto right_type = this->_right->typecheck(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare two values of different types");
        }
        return INT_TYPE;
    }

TypePtr UnequalExpression::typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheck(scope);
        auto right_type = this->_right->typecheck(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare two values of different types");
        }
        return INT_TYPE;
    }

TypePtr AndExpression::typecheck(ScopePtr& scope) {
        if (!this->_left->typecheck(scope)->isScalar() || !this->_right->typecheck(scope)->isScalar()) {
            errorloc(this->loc, "Both sides of a logical and expression must be scalar types");
        }
        return INT_TYPE;
    }

TypePtr OrExpression::typecheck(ScopePtr& scope) {
        if (!this->_left->typecheck(scope)->isScalar() || !this->_right->typecheck(scope)->isScalar()) {
            errorloc(this->loc, "Both sides of a logical or expression must be scalar types");
        }
        return INT_TYPE;
    }

TypePtr TernaryExpression::typecheck(ScopePtr& scope) {
        auto condition_type = this->_condition->typecheck(scope);
        if (!condition_type->isScalar()) {
            errorloc(this->loc, "Condition type must be scalar");
        }
        auto left_type = this->_left->typecheck(scope);
        auto right_type = this->_right->typecheck(scope);
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Left and right type of ternary expression must be equal");
        }
        return left_type;
    }

TypePtr AssignExpression::typecheck(ScopePtr& scope) {
        auto right_type = this->_right->typecheck(scope);
        auto left_type = this->_left->typecheck(scope);

        // 6.5.16.0.2:
        // An assignment operator shall have a modifiable lvalue as its left operand.
        if (!this->_left->isLvalue()) {
            errorloc(this->loc, "Cannot assign to rvalue");
        }

        // 6.5.16.0.3:
        // The type of an assignment expression is the type the left operand would have
        // after lvalue conversion.

        // 6.5.16.1.1:
        // One of the following shall hold:

        // the left operand has [...] arithmetic type,
        // and the right has arithmetic type;
        if (left_type->isArithmetic() && right_type->isArithmetic()) {
            return left_type;
        }

        // the left operand has [...] structure or union type compatible with the type of the right;
        if (left_type->kind == TY_STRUCT && right_type->kind == TY_STRUCT) {
            if (left_type->equals(right_type)) {
                return left_type;
            }
            errorloc(this->loc, "left and right struct of an assign expression must be of compatible type");
        }

        // the left operand has [...] pointer type,
        // and (considering the type the left operand would have after lvalue conversion)
        // both operands are pointers to [...] compatible types [...];
        if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER) {
            if (left_type->equals(right_type)) {
                return left_type;
            }
        }

        // the left operand has [...] pointer type,
        // and (considering the type the left operand would have after lvalue conversion)
        // one operand is a pointer to an object type, and the other is a pointer to [...] void [...];
        if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER) {
            auto left = std::static_pointer_cast<PointerType>(left_type)->inner;
            auto right = std::static_pointer_cast<PointerType>(right_type)->inner;

            if (
                (left->isObjectType() && right->kind == TypeKind::TY_VOID)
                || (left->kind == TypeKind::TY_VOID && right->isObjectType())
            ) {
                return left_type;
            }
        }

        // the left operand is a [...] pointer, and the right is a null pointer constant [...]
        if (left_type->kind == TY_POINTER && right_type->kind == TY_NULLPTR) {
            return left_type;
        }

        errorloc(this->loc, "wrong assign");
    }

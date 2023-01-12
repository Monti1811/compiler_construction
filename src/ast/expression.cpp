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

TypePtr Expression::typecheckWrap(ScopePtr& scope) {
    auto type = this->typecheck(scope);
    if (type->kind == TypeKind::TY_FUNCTION) {
        return std::make_shared<PointerType>(type);
    }
    return type;
}

bool Expression::isLvalue(ScopePtr&) {
    return false;
}

TypePtr IdentExpression::typecheck(ScopePtr& scope) {
        auto type = scope->getVarType(this->_ident);
        if (!type.has_value()) {
            errorloc(this->loc, "Variable ", *this->_ident, " is not defined");
        }
        return type.value();
    }

bool IdentExpression::isLvalue(ScopePtr& scope) {
    // 6.5.1.0.2:
    // An identifier is a primary expression,
    // provided it has been declared as designating an object (in which case it is an lvalue)
    // or a function (in which case it is a function designator).
    return !scope->isFunctionDesignator(this->_ident);
}

TypePtr IntConstantExpression::typecheck(ScopePtr&) { return INT_TYPE; }

TypePtr NullPtrExpression::typecheck(ScopePtr&) { return NULLPTR_TYPE; }

TypePtr CharConstantExpression::typecheck(ScopePtr&) { return CHAR_TYPE; }

TypePtr StringLiteralExpression::typecheck(ScopePtr&) { return STRING_TYPE; }

bool StringLiteralExpression::isLvalue(ScopePtr&) {
    // 6.5.1.0.4:
    // A string literal is a primary expression. It is an lvalue with type as detailed in 6.4.5.
    return true;
}

TypePtr IndexExpression::typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheckWrap(scope);
        auto index_type = this->_index->typecheckWrap(scope);

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

bool IndexExpression::isLvalue(ScopePtr&) {
    return true;
}

TypePtr CallExpression::typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheckWrap(scope);

        auto function_type_opt = expr_type->unwrapFunctionPointer();
        if (!function_type_opt.has_value()) {
            errorloc(this->loc, "Call expression needs to be called on a function pointer");
        }
        auto function_type = function_type_opt.value();

        if (!function_type->return_type->isComplete() && function_type->return_type->kind != TypeKind::TY_VOID) {
            errorloc(this->loc, "Cannot call a function that returns a non-void incomplete type");
        }

        // A function without specified parameters can be called with any arguments
        if (!function_type->has_params) {
            for (auto& arg : this->_arguments) {
                arg->typecheckWrap(scope);
            }

            return function_type->return_type;
        }

        auto param_function_type = std::static_pointer_cast<ParamFunctionType>(function_type);
        auto params = param_function_type->params;

        if (this->_arguments.size() != params.size()) {
            errorloc(this->loc, "Incorrect number of arguments");
        }

        for (size_t i = 0; i < params.size(); i++) {
            auto arg_type = this->_arguments[i]->typecheckWrap(scope);
            if (!arg_type->equals(params[i].type)) {
                errorloc(this->_arguments[i]->loc, "Incorrect argument type");
            }
        }

        return function_type->return_type;
    }

TypePtr DotExpression::typecheck(ScopePtr& scope) {
        auto expr_type = _expression->typecheckWrap(scope);
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

bool DotExpression::isLvalue(ScopePtr& scope) {
    // 6.5.2.3.3:
    // [...] is an lvalue if the first expression is an lvalue.
    return this->_expression->isLvalue(scope);
}

TypePtr ArrowExpression::typecheck(ScopePtr& scope) {
        auto expr_type = _expression->typecheckWrap(scope);
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

bool ArrowExpression::isLvalue(ScopePtr&) {
    // 6.5.2.3.4:
    // The value [...] is an lvalue.
    return true;
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

        // 6.5.3.2.1:
        // The operand of the unary & operator shall be either a function designator,
        // the result of a [] or unary * operator, or an lvalue

        // TODO: Allow function designators

        // IndexExpression and PointerExpression are always lvalues
        if (this->_inner->isLvalue(scope)) {
            return std::make_shared<PointerType>(inner_type);
        }

        errorloc(this->loc, "expression to be referenced must be an lvalue");
    }

TypePtr PointerExpression::typecheck(ScopePtr& scope) {
        auto inner_type = this->_inner->typecheckWrap(scope);
        if (inner_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Cannot dereference a non-pointer");
        }
        auto pointer_type = std::static_pointer_cast<PointerType>(inner_type);
        return pointer_type->inner;
    }

bool PointerExpression::isLvalue(ScopePtr&) {
    // 6.5.3.2.4:
    // If the operand points to a function, the result is a function designator;
    // if it points to an object, the result is an lvalue designating the object.

    // NOTE: Always returning true here technically isn't quite correct.
    // However, it should not make a noticeable difference,
    // because AssignExpression throws an error when it encounters a function type.
    return true;
}

TypePtr NegationExpression::typecheck(ScopePtr& scope) {
        auto inner_type = this->_inner->typecheckWrap(scope);
        if (!inner_type->isArithmetic()) {
            errorloc(this->loc, "type to be negated has to be arithmetic");
        }
        return INT_TYPE;
    }

TypePtr LogicalNegationExpression::typecheck(ScopePtr& scope) {
        auto inner_type = _inner->typecheckWrap(scope);
        if (!inner_type->isScalar()) {
            errorloc(this->loc, "type to be logcially negated has to be scalar");
        }
        return INT_TYPE;
    }

TypePtr BinaryExpression::typecheck(ScopePtr& scope) {
        auto left_type = _left->typecheckWrap(scope);
        auto right_type = _right->typecheckWrap(scope);

        if (!left_type->isArithmetic() || !right_type->isArithmetic()) {
            errorloc(this->loc, "both sides of an arithmetic binary expression must be of arithemic type");
        }
        return INT_TYPE;
    }

TypePtr AddExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

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
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

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
        auto left_type = this->_left->typecheckWrap(scope);
        auto right_type = this->_right->typecheckWrap(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare two values of different types");
        }
        return INT_TYPE;
    }

TypePtr EqualExpression::typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheckWrap(scope);
        auto right_type = this->_right->typecheckWrap(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare two values of different types");
        }
        return INT_TYPE;
    }

TypePtr UnequalExpression::typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheckWrap(scope);
        auto right_type = this->_right->typecheckWrap(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare two values of different types");
        }
        return INT_TYPE;
    }

TypePtr AndExpression::typecheck(ScopePtr& scope) {
        if (!this->_left->typecheckWrap(scope)->isScalar() || !this->_right->typecheckWrap(scope)->isScalar()) {
            errorloc(this->loc, "Both sides of a logical and expression must be scalar types");
        }
        return INT_TYPE;
    }

TypePtr OrExpression::typecheck(ScopePtr& scope) {
        if (!this->_left->typecheckWrap(scope)->isScalar() || !this->_right->typecheckWrap(scope)->isScalar()) {
            errorloc(this->loc, "Both sides of a logical or expression must be scalar types");
        }
        return INT_TYPE;
    }

TypePtr TernaryExpression::typecheck(ScopePtr& scope) {
        auto condition_type = this->_condition->typecheckWrap(scope);
        if (!condition_type->isScalar()) {
            errorloc(this->loc, "Condition type must be scalar");
        }
        auto left_type = this->_left->typecheckWrap(scope);
        auto right_type = this->_right->typecheckWrap(scope);
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Left and right type of ternary expression must be equal");
        }
        return left_type;
    }

TypePtr AssignExpression::typecheck(ScopePtr& scope) {
        auto right_type = this->_right->typecheckWrap(scope);
        auto left_type = this->_left->typecheckWrap(scope);

        // 6.5.16.0.2:
        // An assignment operator shall have a modifiable lvalue as its left operand.
        if (!this->_left->isLvalue(scope)) {
            errorloc(this->loc, "Cannot assign to rvalue");
        }

        // 6.3.2.1.1:
        // A modifiable lvalue is an lvalue that [...] does not have an incomplete type
        if (!left_type->isComplete()) {
            errorloc(this->loc, "Cannot assign to an incomplete type");
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
        if (left_type->kind == TY_STRUCT) {
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

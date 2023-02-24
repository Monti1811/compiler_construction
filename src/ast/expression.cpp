#include "expression.h"

llvm::Value* toBoolTy(llvm::Value* to_convert, std::shared_ptr<CompileScope> CompileScopePtr);

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
    if (this->_swapped) {
        stream << '(' << this->_index << '[' << this->_expression << "])";
    } else {
        stream << '(' << this->_expression << '[' << this->_index << "])";
    }
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
    stream << "(sizeof(" << this->_decl << "))";
}

void BinaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_left << ' ' << this->_op_str << ' ' << this->_right << ')';
}

void TernaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_condition << " ? " << this->_left << " : " << this->_right << ')';
}

void CastExpression::print(std::ostream& stream) {
    stream << this->_inner;
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
    this->type = type.value();
    return this->type;
}

bool IdentExpression::isLvalue(ScopePtr& scope) {
    // 6.5.1.0.2:
    // An identifier is a primary expression,
    // provided it has been declared as designating an object (in which case it is an lvalue)
    // or a function (in which case it is a function designator).
    return !scope->isFunctionDesignator(this->_ident);
}

TypePtr IntConstantExpression::typecheck(ScopePtr&) {
    this->type = INT_TYPE;
    return this->type;
}

TypePtr NullPtrExpression::typecheck(ScopePtr&) {
    this->type = NULLPTR_TYPE;
    return this->type;
}

TypePtr CharConstantExpression::typecheck(ScopePtr&) {
    // 6.4.4.4.10:
    // An integer character constant has type int.
    this->type = INT_TYPE;
    return this->type;
}

TypePtr StringLiteralExpression::typecheck(ScopePtr&) {
    this->type = STRING_TYPE;
    return this->type;
}

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
        this->_index = castExpression(std::move(this->_index), INT_TYPE);
    } else if (expr_type->isInteger() && index_type->isPointer()) {
        // Swapped index expression: 0[arr]
        pointer_type = index_type;
        auto expr = std::move(this->_index);
        this->_index = castExpression(std::move(this->_expression), INT_TYPE);
        this->_expression = std::move(expr);
        this->_swapped = true;
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

    this->type = indexed_type;
    return this->type;
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
        this->type = function_type->return_type;
        return this->type;
    }

    auto param_function_type = std::static_pointer_cast<ParamFunctionType>(function_type);
    auto params = param_function_type->params;

    if (this->_arguments.size() != params.size()) {
        errorloc(this->loc, "Incorrect number of arguments");
    }

    for (size_t i = 0; i < params.size(); i++) {
        auto arg_type = this->_arguments[i]->typecheckWrap(scope);
        auto unified_type = unifyTypes(arg_type, params[i].type);

        if (arg_type->equals(params[i].type) || unified_type.has_value()) {
            this->_arguments[i] = castExpression(std::move(this->_arguments[i]), params[i].type);
        } else if (params[i].type->kind == TY_FUNCTION && arg_type->kind == TY_POINTER) {
            // This case is OK, functions can be implicitly cast to function pointers
        } else {
            errorloc(this->_arguments[i]->loc, "Incorrect argument type, expected ", params[i].type, ", is ", arg_type);
        }
    }

    this->type = function_type->return_type;
    return this->type;
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
        errorloc(this->loc, "Field ", *ident, " does not exist on ", expr_type);
    }
    this->type = field_type.value();
    return this->type;
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
    auto inner_type = pointer_type->inner;
    // Check if the struct has a pointer to a struct as a field
    if (inner_type->kind == TY_STRUCT && !inner_type->isComplete()) {
        auto struct_type = std::static_pointer_cast<StructType>(inner_type);
        // If yes, check the scope to see if it was defined and use this definition to define the type
        std::optional<std::shared_ptr<CompleteStructType>> complete_type = scope->getCompleteStruct(*struct_type);
        if (complete_type.has_value()) {
            // Replace the struct type with the complete type
            pointer_type->inner = complete_type.value();
            // Replace the struct that should be used to calculate the type with the complete definition
            inner_type = complete_type.value();
        }
    }
    if (!inner_type->isComplete()) {
        errorloc(this->loc, "Cannot access a field of an incomplete type");
    }
    auto struct_type = std::static_pointer_cast<CompleteStructType>(inner_type);

    auto ident = this->_ident->_ident;
    auto field_type = struct_type->typeOfField(ident);

    if (!field_type.has_value()) {
        errorloc(this->loc, "Field ", *ident, " does not exist on ", inner_type);
    }
    this->type = field_type.value();
    return this->type;
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
        if (struct_type->tag.has_value()) {
            // We retrieve the struct again from the scope, because we might have gotten an incomplete struct
            // that was redefined at some other point
            auto type = scope->getStructType(struct_type->tag.value());
            if (type.has_value()) {
                struct_type = type.value();
            }
        }
        if (!struct_type->isComplete()) {
            errorloc(this->_inner->loc, "inner of sizeof expression must not have incomplete type");
        }
    }
    this->type = INT_TYPE;
    return this->type;
}

TypePtr SizeofTypeExpression::typecheck(ScopePtr& scope) {
    this->_inner_type = this->_decl.toType(scope).type;
    this->type = INT_TYPE;
    return this->type;
}

TypePtr ReferenceExpression::typecheck(ScopePtr& scope) {
    auto inner_type = this->_inner->typecheck(scope);

    // 6.5.3.2.1:
    // The operand of the unary & operator shall be either a function designator,
    // the result of a [] or unary * operator, or an lvalue

    // IndexExpression and DerefExpression are always lvalues
    if (this->_inner->isLvalue(scope) || inner_type->kind == TypeKind::TY_FUNCTION) {
        this->type = std::make_shared<PointerType>(inner_type);
        return this->type;
    }

    errorloc(this->loc, "expression to be referenced must be a function designator or an lvalue");
}

TypePtr DerefExpression::typecheck(ScopePtr& scope) {
    auto inner_type = this->_inner->typecheckWrap(scope);
    if (inner_type->kind != TypeKind::TY_POINTER) {
        errorloc(this->loc, "Cannot dereference a non-pointer");
    }
    auto pointer_type = std::static_pointer_cast<PointerType>(inner_type);
    this->type = pointer_type->inner;
    return this->type;
}

bool DerefExpression::isLvalue(ScopePtr&) {
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
    this->type = INT_TYPE;
    return this->type;
}

TypePtr LogicalNegationExpression::typecheck(ScopePtr& scope) {
    auto inner_type = _inner->typecheckWrap(scope);
    if (!inner_type->isScalar()) {
        errorloc(this->loc, "type to be logcially negated has to be scalar");
    }
    this->type = INT_TYPE;
    return this->type;
}

TypePtr BinaryExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (!left_type->isArithmetic() || !right_type->isArithmetic()) {
        errorloc(this->loc, "both sides of an arithmetic binary expression must be of arithemic type");
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (unified_type.has_value()) {
        this->type = unified_type.value();

        this->_left = castExpression(std::move(this->_left), unified_type.value());
        this->_right = castExpression(std::move(this->_right), unified_type.value());
    } else {
        this->type = INT_TYPE;
    }

    return this->type;
}

TypePtr AddExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        auto unified_type = unifyTypes(left_type, right_type);
        if (unified_type.has_value()) {
            this->type = unified_type.value();

            this->_left = castExpression(std::move(this->_left), unified_type.value());
            this->_right = castExpression(std::move(this->_right), unified_type.value());
        } else {
            this->type = INT_TYPE;
        }
        return this->type;
    }

    if (left_type->kind == TY_POINTER && right_type->isArithmetic()) {
        this->type = left_type;
        this->_right = castExpression(std::move(this->_right), INT_TYPE);

        return this->type;
    }

    if (left_type->isArithmetic() && right_type->kind == TY_POINTER) {
        this->type = right_type;
        this->_left = castExpression(std::move(this->_left), INT_TYPE);

        return this->type;
    }

    errorloc(this->loc, "Illegal addition operation");
}

TypePtr SubtractExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        auto unified_type = unifyTypes(left_type, right_type);
        if (unified_type.has_value()) {
            this->type = unified_type.value();

            this->_left = castExpression(std::move(this->_left), unified_type.value());
            this->_right = castExpression(std::move(this->_right), unified_type.value());
        } else {
            this->type = INT_TYPE;
        }

        return this->type;
    }
    if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER && left_type->equals(right_type)) {
        auto left_pointer = std::static_pointer_cast<PointerType>(left_type);
        auto right_pointer = std::static_pointer_cast<PointerType>(right_type);
        if (!(left_pointer->inner->isObjectType() && right_pointer->inner->isObjectType())) {
            errorloc(this->loc, "both pointers have to point to object types");
        }
        if (left_pointer->inner->kind == TY_STRUCT && right_pointer->inner->kind == TY_STRUCT) {
            auto left_struct = std::static_pointer_cast<StructType>(left_pointer->inner);
            auto right_struct = std::static_pointer_cast<StructType>(right_pointer->inner);
            if (!left_struct->isComplete() || !right_struct->isComplete()) {
                errorloc(this->loc, "both pointers have to point to object complete types");
            }
        }
        this->type = INT_TYPE;
        return this->type;
    }
    // left side must be complete object type
    if (left_type->kind == TY_POINTER && right_type->isArithmetic()) {
        auto pointer_type = std::static_pointer_cast<PointerType>(left_type);
        // not an object type
        if (!pointer_type->inner->isObjectType()) {
            errorloc(this->loc, "Illegal subtraction operation");
        }
        if (pointer_type->inner->kind == TY_STRUCT) {
            // not a complete object
            auto struct_type = std::static_pointer_cast<StructType>(pointer_type->inner);
            if (!struct_type->isComplete()) {
                errorloc(this->loc, "Illegal subtraction operation");
            }
        }
        this->type = left_type;
        return this->type;
    }

    errorloc(this->loc, "Illegal subtraction operation");
}

TypePtr LessThanExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    this->type = INT_TYPE;

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

TypePtr EqualExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    this->type = INT_TYPE;

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

TypePtr UnequalExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    this->type = INT_TYPE;

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

TypePtr AndExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);
    if (!left_type->isScalar() || !right_type->isScalar()) {
        errorloc(this->loc, "Both sides of a logical and expression must be scalar types");
    }

    this->type = INT_TYPE;

    if (left_type->isPointer() && right_type->isPointer()) {
        return this->type;
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot apply logical and operator to values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

TypePtr OrExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);
    if (!left_type->isScalar() || !right_type->isScalar()) {
        errorloc(this->loc, "Both sides of a logical or expression must be scalar types");
    }

    this->type = INT_TYPE;

    if (left_type->isPointer() && right_type->isPointer()) {
        return this->type;
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot apply logical or operator to values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

TypePtr TernaryExpression::typecheck(ScopePtr& scope) {
    auto condition_type = this->_condition->typecheckWrap(scope);
    if (!condition_type->isScalar()) {
        errorloc(this->loc, "Condition type must be scalar");
    }

    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    if (left_type->kind == TypeKind::TY_VOID || right_type->kind == TypeKind::TY_VOID) {
        this->_left = castExpression(std::move(this->_left), VOID_TYPE);
        this->_right = castExpression(std::move(this->_right), VOID_TYPE);

        this->type = VOID_TYPE;
        return this->type;
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(
                this->loc,
                "Second and third operand of ternary expression are incompatible; cannot unify ",
                left_type,
                " with ",
                right_type
            );
        }
        unified_type = left_type;
    }

    this->type = unified_type.value();

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

TypePtr AssignExpression::typecheck(ScopePtr& scope) {
    auto right_type = this->_right->typecheckWrap(scope);
    auto left_type = this->_left->typecheckWrap(scope);

    // 6.5.16.0.2:
    // An assignment operator shall have a modifiable lvalue as its left operand.
    if (!this->_left->isLvalue(scope)) {
        errorloc(this->loc, "Can only assign to lvalues");
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
        this->type = left_type;

        if (!right_type->strong_equals(left_type)) {
            this->_right = castExpression(std::move(this->_right), left_type);
        }

        return this->type;
    }

    // the left operand has [...] structure or union type compatible with the type of the right;
    if (left_type->kind == TY_STRUCT) {
        if (left_type->equals(right_type)) {
            this->type = left_type;
            return this->type;
        }
        errorloc(this->loc, "left and right struct of an assign expression must be of compatible type");
    }

    // the left operand has [...] pointer type,
    // and (considering the type the left operand would have after lvalue conversion)
    // both operands are pointers to [...] compatible types [...];
    if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER) {
        if (left_type->equals(right_type)) {
            this->type = left_type;
            return this->type;
        }
    }

    // the left operand has [...] pointer type,
    // and (considering the type the left operand would have after lvalue conversion)
    // one operand is a pointer to an object type, and the other is a pointer to [...] void [...];
    if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER) {
        auto left = std::static_pointer_cast<PointerType>(left_type)->inner;
        auto right = std::static_pointer_cast<PointerType>(right_type)->inner;

        if ((left->isObjectType() && right->kind == TypeKind::TY_VOID)
            || (left->kind == TypeKind::TY_VOID && right->isObjectType())) {
            this->type = left_type;
            return this->type;
        }
    }

    // the left operand is a [...] pointer, and the right is a null pointer constant [...]
    if (left_type->kind == TY_POINTER && right_type->kind == TY_NULLPTR) {
        this->type = left_type;
        return this->type;
    }

    errorloc(this->loc, "wrong assign");
}

TypePtr CastExpression::typecheck(ScopePtr&) {
    // Cast Expression does not exist during typecheck phase, so just return type that is being cast to
    return this->type;
}

llvm::Value* IdentExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // identifier should always exist since we typechecked the program already
    llvm::Value* saved_alloca = this->compileLValue(CompileScopePtr);
    llvm::Type* var_type = CompileScopePtr->getType(this->_ident).value();

    // If the identifier denotes a function, return the function directly
    if (var_type->isFunctionTy()) {
        return saved_alloca;
    }

    return CompileScopePtr->_Builder.CreateLoad(var_type, saved_alloca);
}

llvm::Value* IdentExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* saved_alloca = CompileScopePtr->getAlloca(this->_ident).value();
    return saved_alloca;
}

llvm::Value* IntConstantExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return CompileScopePtr->_Builder.getInt32(this->_value);
}

llvm::Value* IntConstantExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* NullPtrExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // We assume that all null pointer constants that are of type int or char have already been cast
    auto type = CompileScopePtr->_Builder.getPtrTy();
    return llvm::ConstantPointerNull::get(type);
}

llvm::Value* NullPtrExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

const std::unordered_map<char, char> ESCAPE_CHARS = {
    {'\'', '\''},
    {'"', '"'},
    {'?', '?'},
    {'\\', '\\'},
    {'a', '\a'},
    {'b', '\b'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'v', '\v'}};

char CharConstantExpression::getChar() {
    // Skip the first char since it is always a single quote
    auto result = this->_value[1];

    // If the first char is a backslash, we are dealing with an escape sequence
    if (result == '\\') {
        auto ch = this->_value[2];
        if (ESCAPE_CHARS.find(ch) == ESCAPE_CHARS.end()) {
            errorloc(this->loc, "Unknown escape code \\", ch);
        }
        result = ESCAPE_CHARS.at(ch);
    }

    return result;
}

llvm::Value* CharConstantExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return CompileScopePtr->_Builder.getInt32(this->getChar());
}

llvm::Value* CharConstantExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

std::string StringLiteralExpression::getString() {
    auto result = std::string("");

    // Loop through all chars, except for the first and last one (those are always double quotes)
    for (size_t i = 1; i < this->_value.length() - 1; i++) {
        auto ch = this->_value[i];
        if (ch != '\\') {
            result += ch;
            continue;
        }
        // Escape codes
        i++;
        ch = this->_value[i];

        if (ESCAPE_CHARS.find(ch) == ESCAPE_CHARS.end()) {
            errorloc(this->loc, "Unknown escape code \\", ch);
        }

        result += ESCAPE_CHARS.at(ch);
    }

    return result;
}

std::optional<size_t> StringLiteralExpression::getStringLength(void) {
    return this->getString().length() + 1;
}

llvm::Value* StringLiteralExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return CompileScopePtr->_Builder.CreateGlobalStringPtr(this->getString());
}

llvm::Value* StringLiteralExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* IndexExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // Get the type of this expression
    llvm::Type* index_type = this->type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    llvm::Value* offset_ptr = this->compileLValue(CompileScopePtr);
    // Load the value at the index
    return CompileScopePtr->_Builder.CreateLoad(index_type, offset_ptr);
}

llvm::Value* IndexExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // Get the type of this expression
    llvm::Type* index_type = this->type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    // Get the pointer of the array
    llvm::Value* array_alloca = this->_expression->compileRValue(CompileScopePtr);
    // Get the value of the index
    llvm::Value* index_value = this->_index->compileRValue(CompileScopePtr);
    // Get a pointer to the element at the index
    return CompileScopePtr->_Builder.CreateInBoundsGEP(index_type, array_alloca, index_value);
}

llvm::Value* CallExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto fun = this->_expression->compileRValue(CompileScopePtr);

    auto function_type = this->_expression->type->unwrapFunctionPointer();
    if (!function_type.has_value()) {
        if (this->_expression->type->kind == TypeKind::TY_FUNCTION) {
            function_type = std::static_pointer_cast<FunctionType>(this->_expression->type);
        } else {
            errorloc(this->loc, "Could not unwrap function pointer during codegen");
        }
    }

    llvm::FunctionType* llvm_function_type =
        function_type.value()->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);

    std::vector<llvm::Value*> args;
    for (auto& arg : this->_arguments) {
        llvm::Value* val = arg->compileRValue(CompileScopePtr);
        args.push_back(val);
    }

    return CompileScopePtr->_Builder.CreateCall(llvm_function_type, fun, llvm::makeArrayRef(args));
}

llvm::Value* CallExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* DotExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* field_alloca = this->compileLValue(CompileScopePtr);
    // Get type of the element
    llvm::Type* index_type = this->type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    return CompileScopePtr->_Builder.CreateLoad(index_type, field_alloca);
}

llvm::Value* DotExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto expr_type = this->_expression->type;

    if (expr_type->kind != TY_STRUCT) {
        errorloc(this->loc, "Tried to access field of non-struct during codegen");
    }

    auto struct_ty = std::static_pointer_cast<StructType>(expr_type);
    if (!struct_ty->isComplete()) {
        errorloc(this->loc, "Tried to access field of incomplete struct during codegen");
    }

    auto complete_struct_ty = std::static_pointer_cast<CompleteStructType>(struct_ty);
    auto index = complete_struct_ty->getIndexOfField(this->_ident->_ident);

    std::vector<llvm::Value*> ElementIndexes;
    ElementIndexes.push_back(CompileScopePtr->_Builder.getInt32(0));
    ElementIndexes.push_back(CompileScopePtr->_Builder.getInt32(index));

    llvm::Type* struct_type = complete_struct_ty->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    llvm::Value* struct_alloca = this->_expression->compileLValue(CompileScopePtr);

    return CompileScopePtr->_Builder.CreateInBoundsGEP(struct_type, struct_alloca, ElementIndexes);
}

llvm::Value* ArrowExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* field_alloca = this->compileLValue(CompileScopePtr);
    // Get type of the element
    llvm::Type* index_type = this->type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    return CompileScopePtr->_Builder.CreateLoad(index_type, field_alloca);
}

llvm::Value* ArrowExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto expr_type = this->_expression->type;

    if (expr_type->kind != TY_POINTER) {
        errorloc(this->loc, "Tried to access field of non-pointer during codegen");
    }

    auto struct_ptr_ty = std::static_pointer_cast<PointerType>(expr_type);
    if (struct_ptr_ty->inner->kind != TY_STRUCT) {
        errorloc(this->loc, "Tried to access field of non-struct during codegen");
    }
    auto struct_ty = std::static_pointer_cast<StructType>(struct_ptr_ty->inner);
    if (!struct_ty->isComplete()) {
        errorloc(this->loc, "Tried to access field of incomplete struct during codegen");
    }

    auto complete_struct_ty = std::static_pointer_cast<CompleteStructType>(struct_ty);
    auto index = complete_struct_ty->getIndexOfField(this->_ident->_ident);

    std::vector<llvm::Value*> ElementIndexes;
    ElementIndexes.push_back(CompileScopePtr->_Builder.getInt32(0));
    ElementIndexes.push_back(CompileScopePtr->_Builder.getInt32(index));

    llvm::Type* struct_type = complete_struct_ty->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    llvm::Value* struct_alloca = this->_expression->compileRValue(CompileScopePtr);
    // llvm::Value* struct_alloca = CompileScopePtr->_Builder.CreateLoad(struct_type, struct_alloca_ptr);

    return CompileScopePtr->_Builder.CreateInBoundsGEP(struct_type, struct_alloca, ElementIndexes);
}

llvm::Value* SizeofExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto string_length = this->_inner->getStringLength();
    if (this->_inner->type->isString() && string_length.has_value()) {
        // String literals and expressions like *&"foo"
        return CompileScopePtr->_Builder.getInt32(string_length.value());
    }
    auto inner_type = this->_inner->type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    return CompileScopePtr->_Builder.getInt32(CompileScopePtr->_Module.getDataLayout().getTypeAllocSize(inner_type));
}

llvm::Value* SizeofExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* SizeofTypeExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto llvm_type = this->_inner_type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    return CompileScopePtr->_Builder.getInt32(CompileScopePtr->_Module.getDataLayout().getTypeAllocSize(llvm_type));
}

llvm::Value* SizeofTypeExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

std::optional<size_t> ReferenceExpression::getStringLength(void) {
    auto inner = dynamic_cast<StringLiteralExpression*>(this->_inner.get());
    if (!inner) {
        return std::nullopt;
    }
    return this->_inner->getStringLength();
}

llvm::Value* ReferenceExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return this->_inner->compileLValue(CompileScopePtr);
}

llvm::Value* ReferenceExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

std::optional<size_t> DerefExpression::getStringLength(void) {
    auto inner = dynamic_cast<ReferenceExpression*>(this->_inner.get());
    if (!inner) {
        return std::nullopt;
    }
    return this->_inner->getStringLength();
}

llvm::Value* DerefExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto expr_value = this->compileLValue(CompileScopePtr);
    auto expr_type = this->_inner->type;

    // If we try to dereference a function, don't do anything and just return the function
    if (expr_type->kind == TypeKind::TY_FUNCTION) {
        return expr_value;
    }

    if (expr_type->kind != TypeKind::TY_POINTER) {
        errorloc(this->loc, "Tried to dereference an expression of type ", expr_type, " during codegen");
    }

    auto ptr_ty = std::static_pointer_cast<PointerType>(expr_type);

    // If we try to dereference a function, don't do anything and just return the function
    if (ptr_ty->inner->kind == TypeKind::TY_FUNCTION) {
        return expr_value;
    }

    auto inner_expr_type = ptr_ty->inner->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    return CompileScopePtr->_Builder.CreateLoad(inner_expr_type, expr_value);
}

llvm::Value* DerefExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return this->_inner->compileRValue(CompileScopePtr);
}

llvm::Value* NegationExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* inner_value = this->_inner->compileRValue(CompileScopePtr);
    inner_value =
        CompileScopePtr->_Builder.CreateIntCast(inner_value, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), true);
    return CompileScopePtr->_Builder.CreateMul(CompileScopePtr->_Builder.getInt32(-1), inner_value);
}

llvm::Value* NegationExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* LogicalNegationExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* inner_value = toBoolTy(this->_inner->compileRValue(CompileScopePtr), CompileScopePtr);
    auto result = CompileScopePtr->_Builder.CreateICmpEQ(CompileScopePtr->_Builder.getInt1(0), inner_value);
    return CompileScopePtr->_Builder.CreateIntCast(result, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), false);
}

llvm::Value* LogicalNegationExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* BinaryExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* MultiplyExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateMul(
        CompileScopePtr->_Builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), true),
        CompileScopePtr->_Builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), true)
    );
}

llvm::Value* AddExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);

    // If one of them is pointer, get the pointer shifted by the value of the second operand
    if (this->_left->type->isPointer()) {
        auto ptr_type = std::static_pointer_cast<PointerType>(this->_left->type);
        auto inner_llvm_type = ptr_type->inner->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
        return CompileScopePtr->_Builder.CreateInBoundsGEP(
            inner_llvm_type,
            value_lhs,
            CompileScopePtr->_Builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), false)
        );
    }

    if (this->_right->type->isPointer()) {
        auto ptr_type = std::static_pointer_cast<PointerType>(this->_right->type);
        auto inner_llvm_type = ptr_type->inner->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
        return CompileScopePtr->_Builder.CreateInBoundsGEP(
            inner_llvm_type,
            value_rhs,
            CompileScopePtr->_Builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), false)
        );
    }

    return CompileScopePtr->_Builder.CreateAdd(
        CompileScopePtr->_Builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), true),
        CompileScopePtr->_Builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), true)
    );
}

llvm::Value* SubtractExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);

    if (value_lhs->getType()->isPointerTy() && value_rhs->getType()->isPointerTy()) {
        auto type = this->_left->type;
        if (type->kind == TypeKind::TY_NULLPTR)
            type = this->_right->type;

        auto ptr_type = std::static_pointer_cast<PointerType>(type);
        auto inner_type = ptr_type->inner;
        auto inner_llvm_type = inner_type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);

        auto int_diff = CompileScopePtr->_Builder.CreatePtrDiff(inner_llvm_type, value_lhs, value_rhs);
        return CompileScopePtr->_Builder.CreateIntCast(int_diff, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), false);
    }

    if (this->_left->type->isPointer()) {
        auto ptr_type = std::static_pointer_cast<PointerType>(this->_left->type);
        auto inner_llvm_type = ptr_type->inner->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
        return CompileScopePtr->_Builder.CreateInBoundsGEP(
            inner_llvm_type,
            value_lhs,
            CompileScopePtr->_Builder.CreateMul(
                CompileScopePtr->_Builder.getInt32(-1),
                CompileScopePtr->_Builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), false)
            )
        );
    }

    return CompileScopePtr->_Builder.CreateSub(
        CompileScopePtr->_Builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), true),
        CompileScopePtr->_Builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), true)
    );
}

llvm::Value* LessThanExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateICmpSLT(value_lhs, value_rhs);
}

llvm::Value* EqualExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateICmpEQ(value_lhs, value_rhs);
}

llvm::Value* UnequalExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateICmpNE(value_lhs, value_rhs);
}

llvm::Value* AndExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = toBoolTy(this->_left->compileRValue(CompileScopePtr), CompileScopePtr);
    auto lhs_block = CompileScopePtr->_Builder.GetInsertBlock();
    /* Add a basic block for the consequence of the OrExpression */
    llvm::BasicBlock* OrConsequenceBlock = llvm::BasicBlock::Create(
        CompileScopePtr->_Ctx /* LLVMContext &Context */,
        "and-consequence" /* const Twine &Name="" */,
        CompileScopePtr->_ParentFunction.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Add a basic block for the end of the TernaryExpression (after the Ternary) */
    llvm::BasicBlock* OrEndBlock = llvm::BasicBlock::Create(
        CompileScopePtr->_Ctx /* LLVMContext &Context */,
        "and-end" /* const Twine &Name="" */,
        CompileScopePtr->_ParentFunction.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    // If first expr is true, we jump to the second term, otherwise we skip the second term.
    CompileScopePtr->_Builder.CreateCondBr(value_lhs, OrConsequenceBlock, OrEndBlock);
    // Create the compiled value of the second term
    CompileScopePtr->_Builder.SetInsertPoint(OrConsequenceBlock);
    llvm::Value* value_rhs = toBoolTy(this->_right->compileRValue(CompileScopePtr), CompileScopePtr);
    auto rhs_block = CompileScopePtr->_Builder.GetInsertBlock();
    /* Insert the jump to the Ternary end block */
    CompileScopePtr->_Builder.CreateBr(OrEndBlock);
    /* Continue in the Ternary end block */
    CompileScopePtr->_Builder.SetInsertPoint(OrEndBlock);
    llvm::PHINode* phi = CompileScopePtr->_Builder.CreatePHI(CompileScopePtr->_Builder.getInt1Ty(), 2);
    phi->addIncoming(value_lhs, lhs_block);
    phi->addIncoming(value_rhs, rhs_block);
    return CompileScopePtr->_Builder.CreateIntCast(phi, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), false);
}

llvm::Value* OrExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = toBoolTy(this->_left->compileRValue(CompileScopePtr), CompileScopePtr);
    auto lhs_block = CompileScopePtr->_Builder.GetInsertBlock();
    /* Add a basic block for the consequence of the OrExpression */
    llvm::BasicBlock* OrConsequenceBlock = llvm::BasicBlock::Create(
        CompileScopePtr->_Ctx /* LLVMContext &Context */,
        "or-consequence" /* const Twine &Name="" */,
        CompileScopePtr->_ParentFunction.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Add a basic block for the end of the TernaryExpression (after the Ternary) */
    llvm::BasicBlock* OrEndBlock = llvm::BasicBlock::Create(
        CompileScopePtr->_Ctx /* LLVMContext &Context */,
        "or-end" /* const Twine &Name="" */,
        CompileScopePtr->_ParentFunction.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    // If first expr is true, we skip the second term, otherwise we jump to the second term.
    CompileScopePtr->_Builder.CreateCondBr(value_lhs, OrEndBlock, OrConsequenceBlock);
    // Create the compiled value of the second term
    CompileScopePtr->_Builder.SetInsertPoint(OrConsequenceBlock);
    llvm::Value* value_rhs = toBoolTy(this->_right->compileRValue(CompileScopePtr), CompileScopePtr);
    auto rhs_block = CompileScopePtr->_Builder.GetInsertBlock();
    /* Insert the jump to the Ternary end block */
    CompileScopePtr->_Builder.CreateBr(OrEndBlock);
    /* Continue in the Ternary end block */
    CompileScopePtr->_Builder.SetInsertPoint(OrEndBlock);
    llvm::PHINode* phi = CompileScopePtr->_Builder.CreatePHI(CompileScopePtr->_Builder.getInt1Ty(), 2);
    phi->addIncoming(value_lhs, lhs_block);
    phi->addIncoming(value_rhs, rhs_block);
    return CompileScopePtr->_Builder.CreateIntCast(phi, llvm::Type::getInt32Ty(CompileScopePtr->_Ctx), false);
}

llvm::Value* TernaryExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto condition_value = toBoolTy(this->_condition->compileRValue(CompileScopePtr), CompileScopePtr);
    /* Add a basic block for the consequence of the TernaryExpression */
    llvm::BasicBlock* TernaryConsequenceBlock = llvm::BasicBlock::Create(
        CompileScopePtr->_Ctx /* LLVMContext &Context */,
        "ternary-consequence" /* const Twine &Name="" */,
        CompileScopePtr->_ParentFunction.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );

    /* Add a basic block for the alternative of the TernaryExpression */
    llvm::BasicBlock* TernaryAlternativeBlock = llvm::BasicBlock::Create(
        CompileScopePtr->_Ctx /* LLVMContext &Context */,
        "ternary-alternative" /* const Twine &Name="" */,
        CompileScopePtr->_ParentFunction.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Add a basic block for the end of the TernaryExpression (after the Ternary) */
    llvm::BasicBlock* TernaryEndBlock = llvm::BasicBlock::Create(
        CompileScopePtr->_Ctx /* LLVMContext &Context */,
        "ternary-end" /* const Twine &Name="" */,
        CompileScopePtr->_ParentFunction.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Create the conditional branch */
    CompileScopePtr->_Builder.CreateCondBr(condition_value, TernaryConsequenceBlock, TernaryAlternativeBlock);
    /* Set the header of the TernaryConsequenceBlock as the new insert point */
    CompileScopePtr->_Builder.SetInsertPoint(TernaryConsequenceBlock);
    auto consequence_compile_scope_ptr = std::make_shared<CompileScope>(CompileScopePtr);
    auto true_value = this->_left->compileRValue(CompileScopePtr);
    auto true_block = CompileScopePtr->_Builder.GetInsertBlock();
    /* Insert the jump to the Ternary end block */
    CompileScopePtr->_Builder.CreateBr(TernaryEndBlock);
    /* Set the header of the TernaryAlternativeBlock as the new insert point */
    CompileScopePtr->_Builder.SetInsertPoint(TernaryAlternativeBlock);
    auto alternative_compile_scope_ptr = std::make_shared<CompileScope>(CompileScopePtr);
    auto false_value = this->_right->compileRValue(CompileScopePtr);
    auto false_block = CompileScopePtr->_Builder.GetInsertBlock();
    /* Insert the jump to the Ternary end block */
    CompileScopePtr->_Builder.CreateBr(TernaryEndBlock);
    /* Continue in the Ternary end block */
    CompileScopePtr->_Builder.SetInsertPoint(TernaryEndBlock);

    if (this->_left->type->kind == TypeKind::TY_VOID || this->_right->type->kind == TypeKind::TY_VOID) {
        return nullptr;
    }
    llvm::PHINode* phi = CompileScopePtr->_Builder.CreatePHI(true_value->getType(), 2);
    phi->addIncoming(true_value, true_block);
    phi->addIncoming(false_value, false_block);
    return phi;
}

llvm::Value* TernaryExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* AssignExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* to_be_stored_in = this->_left->compileLValue(CompileScopePtr);
    llvm::Value* value_to_be_stored = this->_right->compileRValue(CompileScopePtr);
    CompileScopePtr->_Builder.CreateStore(value_to_be_stored, to_be_stored_in);
    return value_to_be_stored;
}

llvm::Value* AssignExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc, "cannot compute l-value of this expression");
}

llvm::Value* CastExpression::compileRValue(std::shared_ptr<CompileScope> compile_scope) {
    auto converted_expr = this->convertNullptrs(compile_scope);
    if (converted_expr.has_value()) {
        return converted_expr.value();
    }

    auto value = this->_inner->compileRValue(compile_scope);
    return this->castArithmetics(compile_scope, value);
}

llvm::Value* CastExpression::compileLValue(std::shared_ptr<CompileScope> compile_scope) {
    auto converted_expr = this->convertNullptrs(compile_scope);
    if (converted_expr.has_value()) {
        return converted_expr.value();
    }

    auto value = this->_inner->compileLValue(compile_scope);
    return this->castArithmetics(compile_scope, value);
}

std::optional<llvm::Value*> CastExpression::convertNullptrs(std::shared_ptr<CompileScope> compile_scope) {
    if (this->_inner->type->kind != TypeKind::TY_NULLPTR) {
        return std::nullopt;
    }

    if (this->type->kind == TypeKind::TY_INT) {
        return compile_scope->_Builder.getInt32(0);
    } else if (this->type->kind == TypeKind::TY_CHAR) {
        return compile_scope->_Builder.getInt8(0);
    } else if (this->type->kind == TypeKind::TY_NULLPTR) {
        return std::nullopt;
    } else if (this->type->kind == TypeKind::TY_POINTER) {
        return std::nullopt;
    } else if (this->type->kind == TypeKind::TY_VOID) {
        return std::nullopt;
    } else {
        errorloc(this->loc, "Invalid usage of null pointer constant");
    }
}

llvm::Value* CastExpression::castArithmetics(std::shared_ptr<CompileScope> compile_scope, llvm::Value* value) {
    auto from_type = this->_inner->type;
    auto to_type = this->type;

    if (from_type->equals(to_type)) {
        return value;
    }

    auto llvm_type = to_type->toLLVMType(compile_scope->_Builder, compile_scope->_Ctx);

    if ((from_type->kind == TypeKind::TY_CHAR && to_type->isInteger())
        || (from_type->isInteger() && to_type->kind == TypeKind::TY_CHAR)) {
        return compile_scope->_Builder.CreateIntCast(value, llvm_type, true);
    }

    if (from_type->isInteger() && to_type->isPointer()) {
        return compile_scope->_Builder.CreateIntToPtr(value, llvm_type);
    } else if (from_type->isPointer() && to_type->isInteger()) {
        return compile_scope->_Builder.CreatePtrToInt(value, llvm_type);
    }

    return value;
}

ExpressionPtr castExpression(ExpressionPtr expr, TypePtr type) {
    auto loc = expr->loc;
    auto cast = std::make_unique<CastExpression>(loc, std::move(expr));
    cast->type = type;
    return cast;
}

llvm::Value* toBoolTy(llvm::Value* to_convert, std::shared_ptr<CompileScope> CompileScopePtr) {
    if (to_convert->getType()->isIntegerTy(32)) {
        return CompileScopePtr->_Builder.CreateICmpNE(CompileScopePtr->_Builder.getInt32(0), to_convert);
    } else if (to_convert->getType()->isIntegerTy(8)) {
        return CompileScopePtr->_Builder.CreateICmpNE(CompileScopePtr->_Builder.getInt8(0), to_convert);
    } else if (to_convert->getType()->isPointerTy()) {
        // return 0 if nullptr, 1 otherwise
        llvm::Value* int_to_convert =
            CompileScopePtr->_Builder.CreatePtrToInt(to_convert, CompileScopePtr->_Builder.getInt32Ty());
        return CompileScopePtr->_Builder.CreateICmpNE(CompileScopePtr->_Builder.getInt32(0), int_to_convert);
    } else {
        return to_convert;
    }
}

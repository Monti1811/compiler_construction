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
    this->type = CHAR_TYPE;
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
            return this->type ;
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
            errorloc(this->loc, "Field " + *ident + " does not exist on this struct");
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
        auto struct_type_to_use = pointer_type->inner;
        // Check if the struct has a pointer to a struct as a field
        if (struct_type_to_use->kind == TY_STRUCT && !struct_type_to_use->isComplete()) {
            auto struct_type = std::static_pointer_cast<StructType>(struct_type_to_use);
            // If yes, check the scope to see if it was defined and use this definition to define the type
            if (struct_type->tag.has_value()) {
                std::optional<std::shared_ptr<StructType>> complete_type = scope->getStructType(struct_type->tag.value());
                if (complete_type.has_value() && complete_type.value()->isComplete()) {
                    // Replace the struct type with the complete type
                    pointer_type->inner = complete_type.value();
                    // Replace the struct that should be used to calculate the type with the complete definition
                    struct_type_to_use = complete_type.value();
                }
            } 
        }
        if (!struct_type_to_use->isComplete()) {
            errorloc(this->loc, "Cannot access a field of an incomplete type");
        }
        auto struct_type = std::static_pointer_cast<CompleteStructType>(struct_type_to_use);

        auto ident = this->_ident->_ident;
        auto field_type = struct_type->typeOfField(ident);

        if (!field_type.has_value()) {
            errorloc(this->loc, "Field " + *ident + " does not exist on this struct");
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
            if (!struct_type->isComplete()) {
                errorloc(this->_inner->loc, "inner of sizeof expression must not have incomplete type");
            }
        }
        this->type = INT_TYPE;
        return this->type;
    }

TypePtr SizeofTypeExpression::typecheck(ScopePtr&) {
        // TODO: Additional checks
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
    this->type = INT_TYPE;
    // Set correct type for nullptr
    if (left_type->kind == TY_NULLPTR) {
        _left->type = INT_TYPE;
    }
    if (right_type->kind == TY_NULLPTR) {
        _right->type = INT_TYPE;
    }
    return this->type;
}

TypePtr AddExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        // Set correct type for nullptr
        if (left_type->kind == TY_NULLPTR) {
            _left->type = INT_TYPE;
        }
        if (right_type->kind == TY_NULLPTR) {
            _right->type = INT_TYPE;
        }
        this->type = INT_TYPE;
        return this->type;
    }
    if (left_type->kind == TY_POINTER && right_type->isArithmetic()) {
        this->type = left_type;
        return this->type;
    }
    if (left_type->isArithmetic() && right_type->kind == TY_POINTER) {
        this->type = right_type;
        return this->type;
    }

    errorloc(this->loc, "Illegal addition operation");
}

TypePtr SubstractExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        // Set correct type for nullptr
        if (left_type->kind == TY_NULLPTR) {
            _left->type = INT_TYPE;
        }
        if (right_type->kind == TY_NULLPTR) {
            _right->type = INT_TYPE;
        }
        this->type = INT_TYPE;
        return this->type;
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
        this->type = INT_TYPE;
        return this->type;
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
        this->type = left_type;
        return this->type;
    }

    errorloc(this->loc, "Illegal substraction operation");
}

TypePtr LessThanExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    if (!left_type->equals(right_type)) {
        errorloc(this->loc, "Cannot compare two values of different types");
    }
    // Set correct type for nullptr
    if (left_type->kind == TY_NULLPTR && right_type->kind != TY_POINTER) {
        _left->type = INT_TYPE;
    }
    if (right_type->kind == TY_NULLPTR && right_type->kind != TY_POINTER) {
        _right->type = INT_TYPE;
    }
    this->type = INT_TYPE;
    return this->type;
}

TypePtr EqualExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    if (!left_type->equals(right_type)) {
        errorloc(this->loc, "Cannot compare two values of different types");
    }
    // Set correct type for nullptr
    if (left_type->kind == TY_NULLPTR && right_type->kind != TY_POINTER) {
        _left->type = INT_TYPE;
    }
    if (right_type->kind == TY_NULLPTR && left_type->kind != TY_POINTER) {
        _right->type = INT_TYPE;
    }
    this->type = INT_TYPE;
    return this->type;
}

TypePtr UnequalExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    if (!left_type->equals(right_type)) {
        errorloc(this->loc, "Cannot compare two values of different types");
    }
    // Set correct type for nullptr
    if (left_type->kind == TY_NULLPTR && right_type->kind != TY_POINTER) {
        _left->type = INT_TYPE;
    }
    if (right_type->kind == TY_NULLPTR && left_type->kind != TY_POINTER) {
        _right->type = INT_TYPE;
    }
    this->type = INT_TYPE;
    return this->type;
}

TypePtr AndExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);
    if (!left_type->isScalar() || !right_type->isScalar()) {
        errorloc(this->loc, "Both sides of a logical and expression must be scalar types");
    }
    // Set correct type for nullptr
    if (left_type->kind == TY_NULLPTR && right_type->kind != TY_POINTER) {
        _left->type = INT_TYPE;
    }
    if (right_type->kind == TY_NULLPTR && left_type->kind != TY_POINTER) {
        _right->type = INT_TYPE;
    }
    this->type = INT_TYPE;
    return this->type;
}

TypePtr OrExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);
    if (!left_type->isScalar() || !right_type->isScalar()) {
        errorloc(this->loc, "Both sides of a logical and expression must be scalar types");
    }
    // Set correct type for nullptr
    if (left_type->kind == TY_NULLPTR && right_type->kind != TY_POINTER) {
        _left->type = INT_TYPE;
    }
    if (right_type->kind == TY_NULLPTR && left_type->kind != TY_POINTER) {
        _right->type = INT_TYPE;
    }
    this->type = INT_TYPE;
    return this->type;
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
    // Set correct type for nullptr
    if (left_type->kind == TY_NULLPTR && right_type->kind != TY_POINTER) {
        _left->type = INT_TYPE;
    }
    if (right_type->kind == TY_NULLPTR && left_type->kind != TY_POINTER) {
        _right->type = INT_TYPE;
    }
    this->type = left_type;
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
        // Set correct type for nullptr
        if (left_type->kind == TY_NULLPTR) {
            _left->type = INT_TYPE;
        }
        if (right_type->kind == TY_NULLPTR) {
            _right->type = INT_TYPE;
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

        if (
            (left->isObjectType() && right->kind == TypeKind::TY_VOID)
            || (left->kind == TypeKind::TY_VOID && right->isObjectType())
        ) {
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


llvm::Value* IdentExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // If it's a function, don't load the function
    auto fun = CompileScopePtr->_Module.getFunction(*(this->_ident));
    if (fun != NULL) {
        return fun;
    }
    // identifier should always exist since we typechecked the program already
    llvm::Value* saved_alloca = CompileScopePtr->getAlloca(this->_ident).value();
    llvm::Type* var_type = CompileScopePtr->getType(this->_ident).value();
    return CompileScopePtr->_Builder.CreateLoad(var_type, saved_alloca);
}

llvm::Value* IdentExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* saved_alloca = CompileScopePtr->getAlloca(this->_ident).value();
    return saved_alloca;
}

llvm::Value* IntConstantExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return CompileScopePtr->_Builder.getInt32(this->_value);
}

llvm::Value* IntConstantExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* NullPtrExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // TODO: Currently type is NULLPTR so isPointer will always give back true
    if (this->type->isPointer()) {
        auto type = CompileScopePtr->_Builder.getPtrTy();
        return llvm::ConstantPointerNull::get(type);
    } else {
        return CompileScopePtr->_Builder.getInt32(0);
    }
    //return CompileScopePtr->_Builder.getInt32(0);
}

llvm::Value* NullPtrExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* CharConstantExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return CompileScopePtr->_Builder.getInt8(this->_value[0]);
}

llvm::Value* CharConstantExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* StringLiteralExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return CompileScopePtr->_Builder.CreateGlobalStringPtr(this->_value);
}

llvm::Value* StringLiteralExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* IndexExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // Get the pointer to the index that should be used
    auto expr_value = this->compileLValue(CompileScopePtr);
    // Get the type of the index by checking the type saved in the pointer of the array
    auto expr_type = this->_expression->type;
    auto pointer_ty = std::static_pointer_cast<PointerType>(expr_type);
    llvm::Type* index_type = pointer_ty->inner->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    // Load the value at the index
    return CompileScopePtr->_Builder.CreateLoad(index_type, expr_value);
}

llvm::Value* IndexExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    // Get the pointer of the array
    llvm::Value* array_alloca = this->_expression->compileLValue(CompileScopePtr);
    // Get the value of the index that should be used
    llvm::Value* index_value = this->_index->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateInBoundsGEP(array_alloca->getType(), array_alloca, index_value); 
}

llvm::Value* CallExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    Symbol name(static_cast<IdentExpression*>(this->_expression.get())->_ident);
    llvm::Function* fun = CompileScopePtr->_Module.getFunction(*name);
    if (fun == NULL) {
        //fun = std::static_pointer_cast<llvm::Function*>(CompileScopePtr->getAlloca(name).value());
        llvm::Value* fun_ptr = CompileScopePtr->getAlloca(name).value();
        llvm::Value* fun_val = CompileScopePtr->_Builder.CreateLoad(fun_ptr->getType(), fun_ptr);
        fun = static_cast<llvm::Function*>(fun_val);
    }
    std::vector<llvm::Value*> args;
    for (size_t i = 0; i < this->_arguments.size(); i++) {
        llvm::Value* val = this->_arguments[i]->compileRValue(CompileScopePtr);
        args.push_back(val);
    }
    
    return CompileScopePtr->_Builder.CreateCall(fun, llvm::makeArrayRef(args));
}

llvm::Value* CallExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
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
 
    std::vector<llvm::Value *> ElementIndexes;
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
        // TODO: Structs that declare themselves as pointers in their fields are incomplete
        errorloc(this->loc, "Tried to access field of incomplete struct during codegen");
    }
    
    auto complete_struct_ty = std::static_pointer_cast<CompleteStructType>(struct_ty);
    auto index = complete_struct_ty->getIndexOfField(this->_ident->_ident);
 
    std::vector<llvm::Value *> ElementIndexes;
    ElementIndexes.push_back(CompileScopePtr->_Builder.getInt32(0));
    ElementIndexes.push_back(CompileScopePtr->_Builder.getInt32(index));

    llvm::Type* struct_type = complete_struct_ty->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    llvm::Value* struct_alloca = this->_expression->compileRValue(CompileScopePtr);
    // llvm::Value* struct_alloca = CompileScopePtr->_Builder.CreateLoad(struct_type, struct_alloca_ptr);

    return CompileScopePtr->_Builder.CreateInBoundsGEP(struct_type, struct_alloca, ElementIndexes);
}

llvm::Value* SizeofExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    if (this->_inner->type == STRING_TYPE) {
        // TODO: Strings that are like this: *&"foos"
        auto string = static_cast<StringLiteralExpression*>(this->_inner.get());
        return CompileScopePtr->_Builder.getInt32(string->_value.length()-1);
    }
    switch (this->_inner->type->kind) {
        case TY_INT: return CompileScopePtr->_Builder.getInt32(4);
        case TY_CHAR: return CompileScopePtr->_Builder.getInt32(1);
        case TY_VOID: return CompileScopePtr->_Builder.getInt32(1);
        case TY_POINTER: return CompileScopePtr->_Builder.getInt32(8);
        case TY_NULLPTR: return CompileScopePtr->_Builder.getInt32(8);
        case TY_STRUCT: {
            auto inner_type = this->type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
            return CompileScopePtr->_Builder.getInt32(CompileScopePtr->_Module.getDataLayout().getTypeAllocSize(inner_type));
        }
        default:
            error("sizeof type cannot be compiled");
    }
}

llvm::Value* SizeofExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* SizeofTypeExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    switch (this->_type->_kind) {
        case TY_INT: return CompileScopePtr->_Builder.getInt32(4);
        case TY_CHAR: return CompileScopePtr->_Builder.getInt32(1);
        case TY_VOID: return CompileScopePtr->_Builder.getInt32(1);
        case TY_POINTER: return CompileScopePtr->_Builder.getInt32(8);
        case TY_NULLPTR: return CompileScopePtr->_Builder.getInt32(8);
        case TY_STRUCT: {
            auto inner_type = this->type->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
            return CompileScopePtr->_Builder.getInt32(CompileScopePtr->_Module.getDataLayout().getTypeAllocSize(inner_type));
        }
        default:
            error("sizeof type cannot be compiled");
    }
}

llvm::Value* SizeofTypeExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* ReferenceExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return this->_inner->compileLValue(CompileScopePtr);
}

llvm::Value* ReferenceExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* DerefExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto expr_value = this->compileLValue(CompileScopePtr);
    auto expr_type = this->_inner->type;
    if (expr_type->kind != TY_POINTER) {
        errorloc(this->loc, "Tried to dereference non-pointer during codegen");
    }

    auto ptr_ty = std::static_pointer_cast<PointerType>(expr_type);
    auto inner_expr_type = ptr_ty->inner->toLLVMType(CompileScopePtr->_Builder, CompileScopePtr->_Ctx);
    return CompileScopePtr->_Builder.CreateLoad(inner_expr_type, expr_value);
}

llvm::Value* DerefExpression::compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    return this->_inner->compileRValue(CompileScopePtr);
}

llvm::Value* NegationExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* inner_value = this->_inner->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateMul(CompileScopePtr->_Builder.getInt32(-1), inner_value);
}

llvm::Value* NegationExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* LogicalNegationExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* inner_value = this->_inner->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateICmpEQ(CompileScopePtr->_Builder.getInt32(0), inner_value);
}

llvm::Value* LogicalNegationExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* BinaryExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* MultiplyExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateMul(value_lhs, value_rhs);
}

llvm::Value* AddExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    // If one of them is pointer, get the pointer shifted by the value of the second operand
    if (value_lhs->getType()->isPointerTy()) {
        return CompileScopePtr->_Builder.CreateInBoundsGEP(CompileScopePtr->_Builder.getInt32Ty(), value_lhs, value_rhs);
    }
    if (value_rhs->getType()->isPointerTy()) {
        return CompileScopePtr->_Builder.CreateInBoundsGEP(CompileScopePtr->_Builder.getInt32Ty(), value_rhs, value_lhs);
    }
    return CompileScopePtr->_Builder.CreateAdd(value_lhs, value_rhs);
}

llvm::Value* SubstractExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    if (value_lhs->getType()->isPointerTy() && value_rhs->getType()->isPointerTy()) {
        value_lhs = CompileScopePtr->_Builder.CreatePtrToInt(value_lhs, CompileScopePtr->_Builder.getInt32Ty());
        value_rhs = CompileScopePtr->_Builder.CreatePtrToInt(value_rhs, CompileScopePtr->_Builder.getInt32Ty());
        llvm::Value* sub_exp = CompileScopePtr->_Builder.CreateSub(value_lhs, value_rhs);
        return CompileScopePtr->_Builder.CreateExactSDiv(sub_exp, CompileScopePtr->_Builder.getInt32(4));
    }
    return CompileScopePtr->_Builder.CreateSub(value_lhs, value_rhs);
}

llvm::Value* LessThanExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateICmpULT(value_lhs, value_rhs);
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
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateLogicalAnd(value_lhs, value_rhs);
}

llvm::Value* OrExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* value_lhs = this->_left->compileRValue(CompileScopePtr);
    llvm::Value* value_rhs = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateLogicalOr(value_lhs, value_rhs);
}

llvm::Value* TernaryExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    auto condition_value = this->_condition->compileRValue(CompileScopePtr);
    auto true_value = this->_left->compileRValue(CompileScopePtr);
    auto false_value = this->_right->compileRValue(CompileScopePtr);
    return CompileScopePtr->_Builder.CreateSelect(condition_value, true_value, false_value);
}

llvm::Value* TernaryExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

llvm::Value* AssignExpression::compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) {
    llvm::Value* to_be_stored_in = this->_left->compileLValue(CompileScopePtr);
    llvm::Value* value_to_be_stored = this->_right->compileRValue(CompileScopePtr);
    CompileScopePtr->_Builder.CreateStore(value_to_be_stored, to_be_stored_in);
    return value_to_be_stored;
}

llvm::Value* AssignExpression::compileLValue(std::shared_ptr<CompileScope>) {
    errorloc(this->loc,"cannot compute l-value of this expression");
}

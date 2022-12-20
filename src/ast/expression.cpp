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
        auto type = scope->getTypeVar(this->_ident);
        if (!type.has_value()) {
            errorloc(this->loc, "Variable ", *this->_ident, " is not defined");
        }
        return type.value();
    }

TypePtr IntConstantExpression::typecheck(ScopePtr&) { return INT_TYPE; }

TypePtr CharConstantExpression::typecheck(ScopePtr&) { return CHAR_TYPE; }

TypePtr StringLiteralExpression::typecheck(ScopePtr&) { return STRING_TYPE; }

TypePtr IndexExpression::typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Indexed expression must have pointer type");
        }
        if (this->_index->typecheck(scope)->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "Index must have integer type");
        }
        auto expr_pointer_type = std::static_pointer_cast<PointerType>(expr_type);
        return std::make_shared<Type>(*expr_pointer_type->inner);
    }

TypePtr CallExpression::typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->_expression->loc, "Call epression needs to be called on a function pointer");
        }
        auto pointer_type = std::static_pointer_cast<PointerType>(expr_type);
        auto& callee_type = pointer_type->inner;
        if (callee_type->kind != TypeKind::TY_FUNCTION) {
            errorloc(this->_expression->loc, "Cannot call a non-function");
        }
        auto function_type = std::static_pointer_cast<FunctionType>(callee_type);
        if (this->_arguments.size() != function_type->args.size()) {
            // a function f() can accept any number of args
            if (function_type->args.size() != 0) {
                errorloc(this->loc, "Invalid number of arguments");
            }
        }
        for (size_t i = 0; i < function_type->args.size(); i++) {
            auto arg_type = this->_arguments[i]->typecheck(scope);
            if (!arg_type->equals(function_type->args[i])) {
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
        auto struct_type = std::static_pointer_cast<StructType>(expr_type);

        auto ident = this->_ident->_ident;
        if (struct_type->fields.find(ident) == struct_type->fields.end()) {
            errorloc(this->loc, "Field does not exist on this struct");
        }

        return struct_type->fields.at(ident);
    }

TypePtr ArrowExpression::typecheck(ScopePtr& scope) {
        auto expr_type = _expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Cannot access non-pointers using the arrow operator");
        }
        auto pointer_type = std::static_pointer_cast<PointerType>(expr_type);

        if (pointer_type->inner->kind != TypeKind::TY_STRUCT) {
            errorloc(this->loc, "Cannot index a non-struct expression");
        }
        auto struct_type = std::static_pointer_cast<StructType>(pointer_type->inner);

        auto ident = this->_ident->_ident;
        if (struct_type->fields.find(ident) == struct_type->fields.end()) {
            errorloc(this->loc, "Field does not exist on this struct");
        }

        return struct_type->fields.at(ident);
    }

TypePtr SizeofExpression::typecheck(ScopePtr& scope) {
        // TODO: Additional checks
        return INT_TYPE;
    }

TypePtr SizeofTypeExpression::typecheck(ScopePtr& scope) {
        // TODO: Additional checks
        return INT_TYPE;
    }

TypePtr ReferenceExpression::typecheck(ScopePtr& scope) {
        // TODO: Additional checks

        auto inner_type = this->_inner->typecheck(scope);
        return std::make_shared<PointerType>(inner_type);
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
        if (inner_type->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "type to be negated has to be int");
        }
        return INT_TYPE;
    }

TypePtr LogicalNegationExpression::typecheck(ScopePtr& scope) {
        auto inner_type = _inner->typecheck(scope);
        if (inner_type->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "type to be logcially negated has to be int");
        }
        return INT_TYPE;
    }

TypePtr BinaryExpression::typecheck(ScopePtr& scope) {
        auto left_type = _left->typecheck(scope);
        auto right_type = _right->typecheck(scope);
        // TODO: Check for arithmetic types, not just int
        if (left_type->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "left side of a binary expression must be of type int");
        } 
        if (right_type->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "right side of a binary expression must be of type int");
        }
        return INT_TYPE;
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
        auto left_type = this->_left->typecheck(scope);
        if (!this->_left->isLvalue()) {
            errorloc(this->loc, "Cannot assign to rvalue");
        }
        return left_type;
    }
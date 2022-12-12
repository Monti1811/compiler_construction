#pragma once

#include <memory>
#include <vector>
#include <string.h>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"
#include "../lexer/token.h"

#include "declarator.h"
#include "types.h"

struct Expression {
    Expression(Locatable loc)
        : loc(loc) {};
    virtual ~Expression() = default;
    Locatable loc;

    virtual TypePtr typecheck(ScopePtr& scope) = 0;
    virtual bool isLvalue(void) = 0;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr);
};

typedef std::unique_ptr<Expression> ExpressionPtr;

struct IdentExpression: public Expression {
    public:
    IdentExpression(Locatable loc, Symbol ident)
        : Expression(loc)
        , _ident(ident) {};

    TypePtr typecheck(ScopePtr& scope) {
        return scope->getTypeVar(this->_ident);
    }
    bool isLvalue() { return true; }

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<IdentExpression>& expr);

    public:
    Symbol _ident;
};

struct IntConstantExpression: public Expression {
    public:
    IntConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(std::stoull(*value)) {};

    TypePtr typecheck(ScopePtr&) { return Type::makeInt(); }
    bool isLvalue() { return false; }

    void print(std::ostream& stream);

    unsigned long long _value;
};

struct CharConstantExpression: public Expression {
    public:
    CharConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value( (*value) ) {};

    TypePtr typecheck(ScopePtr&) { return Type::makeChar(); }
    bool isLvalue() { return false; }

    void print(std::ostream& stream);

    std::string _value;
};

struct StringLiteralExpression: public Expression {
    public:
    StringLiteralExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(*value) {};

    TypePtr typecheck(ScopePtr&) {
        return std::make_unique<PointerType>(Type::makeChar());
    }
    bool isLvalue() { return true; }

    void print(std::ostream& stream);

    private:
    std::string _value;
};

struct IndexExpression: public Expression {
    public:
    IndexExpression(Locatable loc, ExpressionPtr expression, ExpressionPtr index)
        : Expression(loc)
        , _expression(std::move(expression))
        , _index(std::move(index)) {};

    TypePtr typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Indexed expression must have pointer type");
        }
        if (this->_index->typecheck(scope)->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "Index must have integer type");
        }
        auto expr_pointer_type = static_cast<PointerType>(std::move(expr_type));
        return std::move(expr_pointer_type.inner);
    }
    bool isLvalue() {
        return true; // TODO
    }

    void print(std::ostream& stream);

    // expression[index]
    private:
    ExpressionPtr _expression;
    ExpressionPtr _index;
};

struct CallExpression: public Expression {
    public:
    CallExpression(Locatable loc, ExpressionPtr expression, std::vector<ExpressionPtr> arguments)
        : Expression(loc)
        , _expression(std::move(expression))
        , _arguments(std::move(arguments)) {};

    TypePtr typecheck(ScopePtr& scope) {
        auto expr_type = this->_expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->_expression->loc, "Call epression needs to be called on a function pointer");
        }
        auto called_type = static_cast<PointerType>(std::move(expr_type)).inner;
        if (called_type->kind != TypeKind::TY_FUNCTION) {
            errorloc(this->_expression->loc, "Cannot call a non-function");
        }
        auto function_type = dynamic_cast<FunctionType*>(called_type.get());
        if (this->_arguments.size() != function_type->args.size()) {
            // TODO: Technically not quite correct, a function f() can accept any number of args
            errorloc(this->loc, "Invalid number of arguments");
        }
        for (size_t i = 0; i < this->_arguments.size(); i++) {
            auto arg_type = this->_arguments[i]->typecheck(scope);
            if (!arg_type->equals(function_type->args[i])) {
                errorloc(this->_arguments[i]->loc, "Incorrect argument type");
            }
        }
        return std::move(function_type->return_type);
    }
    bool isLvalue() { return false; } // TODO

    void print(std::ostream& stream);

    // expression(args)
    ExpressionPtr _expression;
    std::vector<ExpressionPtr> _arguments;
};

struct DotExpression: public Expression {
    public:
    DotExpression(Locatable loc, ExpressionPtr expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope) {
        auto expr_type = _expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_STRUCT) {
            errorloc(this->loc, "Cannot access a field of a non-struct expression");
        }
        auto struct_type = static_cast<StructType*>(expr_type.get());

        auto ident = this->_ident->_ident;
        if (!struct_type->fields.at(ident)) {
            errorloc(this->loc, "Field does not exist on this struct");
        }

        return std::move(struct_type->fields.at(ident));
    }
    bool isLvalue() { return false; } // TODO

    // expression.ident
    private:
    ExpressionPtr _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct ArrowExpression: public Expression {
    public:
    ArrowExpression(Locatable loc, ExpressionPtr expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope) {
        auto expr_type = _expression->typecheck(scope);
        if (expr_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Cannot access non-pointers using the arrow operator");
        }
        auto pointer_type = static_cast<PointerType*>(expr_type.get());

        if (pointer_type->inner->kind != TypeKind::TY_STRUCT) {
            errorloc(this->loc, "Cannot index a non-struct expression");
        }
        auto struct_type = static_cast<StructType*>(pointer_type->inner.get());

        auto ident = this->_ident->_ident;
        if (!struct_type->fields.at(ident)) {
            errorloc(this->loc, "Field does not exist on this struct");
        }

        return std::move(struct_type->fields.at(ident));
    }
    bool isLvalue() { return false; } // TODO

    // expression->ident
    private:
    ExpressionPtr _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct UnaryExpression: public Expression {
    public:
    UnaryExpression(Locatable loc, ExpressionPtr inner, const char* const op_str)
        : Expression(loc)
        , _inner(std::move(inner))
        , _op_str(op_str) {};
    
    void print(std::ostream& stream);

    ExpressionPtr _inner;

    private:
    const char* const _op_str;
};

struct SizeofExpression: public UnaryExpression {
    // sizeof inner

    public:
    SizeofExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "sizeof ") {};
        
    TypePtr typecheck(ScopePtr&) {
        // TODO: Additional checks
        return Type::makeInt();
    }
    bool isLvalue() { return false; }
};

struct SizeofTypeExpression: public Expression {
    public:
    SizeofTypeExpression(Locatable loc, TypeSpecifierPtr type)
        : Expression(loc)
        , _type(std::move(type)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr&) {
        // TODO: Additional checks
        return Type::makeInt();
    }
    bool isLvalue() { return false; }

    // sizeof (inner)
    private:
    TypeSpecifierPtr _type;
};

struct ReferenceExpression: public UnaryExpression {
    // &inner

    public:
    ReferenceExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "&") {};
    
    TypePtr typecheck(ScopePtr& scope) {
        // TODO: Additional checks

        auto inner_type = this->_inner->typecheck(scope);
        return std::make_unique<PointerType>(std::move(inner_type));
    }
    bool isLvalue() { return false; }
};

struct PointerExpression: public UnaryExpression {
    // *inner

    public:
    PointerExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "*") {};

    TypePtr typecheck(ScopePtr& scope) {
        auto inner_type = this->_inner->typecheck(scope);
        if (inner_type->kind != TypeKind::TY_POINTER) {
            errorloc(this->loc, "Cannot dereference a non-pointer");
        }
        auto pointer_type = static_cast<PointerType>(std::move(inner_type));
        return std::move(pointer_type.inner);
    }
    bool isLvalue() { return true; }
};

struct NegationExpression: public UnaryExpression {
    // -inner

    public:
    NegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "-") {};
        
    TypePtr typecheck(ScopePtr& scope) {
        TypePtr innerType = _inner->typecheck(scope);
        if (innerType->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "type to be negated has to be int");
        }
        return Type::makeInt();
    }

    bool isLvalue() { return false; }
};

struct LogicalNegationExpression: public UnaryExpression {
    // !inner

    public:
    LogicalNegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "!") {};

    TypePtr typecheck(ScopePtr& scope) {
        TypePtr innerType = _inner->typecheck(scope);
        if (innerType->kind != TypeKind::TY_INT) {
            errorloc(this->loc, "type to be logcially negated has to be int");
        }
        return Type::makeInt();
    }
    bool isLvalue() { return false; }
};

struct BinaryExpression: public Expression {
    public:
    BinaryExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right, const char* const op_str)
        : Expression(loc)
        , _left(std::move(left))
        , _right(std::move(right))
        , _op_str(op_str) {};

    void print(std::ostream& stream);
    TypePtr typecheck(ScopePtr& scope) {
        TypePtr leftType = _left->typecheck(scope);
        TypePtr rightType = _right->typecheck(scope);
        // TODO: Check for arithmetic types, not just int
        if (!leftType->kind == TypeKind::TY_INT) {
            errorloc(this->loc, "left side of a binary expression must be of type int");
        } 
        if (!rightType->kind == TypeKind::TY_INT) {
            errorloc(this->loc, "right side of a binary expression must be of type int");
        }
        return Type::makeInt();
    }
    bool isLvalue() { return false; }

    ExpressionPtr _left;
    ExpressionPtr _right;

    private:
    const char* const _op_str;
};

struct MultiplyExpression: public BinaryExpression {
    // left * right

    public:
    MultiplyExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "*") {};
};

struct AddExpression: public BinaryExpression {
    // left + right

    public:
    AddExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "+") {};
};

struct SubstractExpression: public BinaryExpression {
    // left - right

    public:
    SubstractExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "-") {};
};

struct LessThanExpression: public BinaryExpression {
    // left < right

    public:
    LessThanExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "<") {};

    TypePtr typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheck(scope);
        auto right_type = this->_right->typecheck(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Both sides of a less than expression must have the same type");
        }
        return Type::makeInt();
    }
    bool isLvalue() { return false; }
};

struct EqualExpression: public BinaryExpression {
    // left == right

    public:
    EqualExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "==") {};
    
    TypePtr typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheck(scope);
        auto right_type = this->_right->typecheck(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Both sides of an equal expression must have the same type");
        }
        return Type::makeInt();
    }
    bool isLvalue() { return false; }
};

struct UnequalExpression: public BinaryExpression {
    // left != right

    public:
    UnequalExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "!=") {};

    TypePtr typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheck(scope);
        auto right_type = this->_right->typecheck(scope);

        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Both sides of unequal expression must have the same type");
        }
        return Type::makeInt();
    }
    bool isLvalue() { return false; }
};

struct AndExpression: public BinaryExpression {
    // left && right

    public:
    AndExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "&&") {};

    TypePtr typecheck(ScopePtr& scope) {
        if (!this->_left->typecheck(scope)->isScalar() || !this->_right->typecheck(scope)->isScalar()) {
            errorloc(this->loc, "Both sides of a logical and expression must be scalar types");
        }
        return Type::makeInt();
    }
    bool isLvalue() { return false; }
};

struct OrExpression: public BinaryExpression {
    // left || right

    public:
    OrExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "||") {};

    TypePtr typecheck(ScopePtr& scope) {
        if (!this->_left->typecheck(scope)->isScalar() || !this->_right->typecheck(scope)->isScalar()) {
            errorloc(this->loc, "Both sides of a logical or expression must be scalar types");
        }
        return Type::makeInt();
    }
    bool isLvalue() { return false; }
};

struct TernaryExpression: public Expression {
    // condition ? left : right

    public:
    TernaryExpression(Locatable loc, ExpressionPtr condition, ExpressionPtr left, ExpressionPtr right)
        : Expression(loc)
        , _condition(std::move(condition))
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope) {
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
    bool isLvalue() { return false; }

    private:
    ExpressionPtr _condition;
    ExpressionPtr _left;
    ExpressionPtr _right;
};

struct AssignExpression: public BinaryExpression {
    // left = right

    public:
    AssignExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "=") {};

    TypePtr typecheck(ScopePtr& scope) {
        auto left_type = this->_left->typecheck(scope);
        if (this->_left->isLvalue()) {
            errorloc(this->loc, "Cannot assign to rvalue");
        }
        return left_type;
    }
    bool isLvalue() { return false; }
};

#pragma once

#include <memory>
#include <vector>
#include <string.h>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"
#include "../lexer/token.h"

#include "declarator.h"

struct Expression {
    Expression(Locatable loc)
        : loc(loc) {};
    virtual ~Expression() = default;
    Locatable loc;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr);
};

typedef std::unique_ptr<Expression> ExpressionPtr;

struct IdentExpression: public Expression {
    public:
    IdentExpression(Locatable loc, Symbol ident)
        : Expression(loc)
        , _ident(ident) {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<IdentExpression>& expr);

    private:
    Symbol _ident;
};

struct IntConstantExpression: public Expression {
    public:
    IntConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(std::stoull(*value)) {};

    void print(std::ostream& stream);

    unsigned long long _value;
};

struct CharConstantExpression: public Expression {
    public:
    CharConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value( (*value)[0] ) {};

    void print(std::ostream& stream);

    unsigned char _value;
};

struct StringLiteralExpression: public Expression {
    public:
    StringLiteralExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(*value) {};

    void print(std::ostream& stream);

    private:
    std::string _value;
};

struct IndexExpression: public Expression {
    public:
    IndexExpression(Locatable loc, ExpressionPtr expression, std::unique_ptr<Expression> index)
        : Expression(loc)
        , _expression(std::move(expression))
        , _index(std::move(index)) {};

    void print(std::ostream& stream);

    // expression[index]
    private:
    ExpressionPtr _expression;
    ExpressionPtr _index;
};

struct CallExpression: public Expression {
    public:
    CallExpression(Locatable loc, ExpressionPtr expression, std::vector<std::unique_ptr<Expression>> arguments)
        : Expression(loc)
        , _expression(std::move(expression))
        , _arguments(std::move(arguments)) {};

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

    private:
    ExpressionPtr _inner;
    const char* const _op_str;
};

struct SizeofExpression: public UnaryExpression {
    // sizeof inner

    public:
    SizeofExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "sizeof ") {};
};

struct SizeofTypeExpression: public Expression {
    public:
    SizeofTypeExpression(Locatable loc, std::unique_ptr<TypeSpecifier> type)
        : Expression(loc)
        , _type(std::move(type)) {};

    void print(std::ostream& stream);

    // sizeof (inner)
    private:
    std::unique_ptr<TypeSpecifier> _type;
};

struct ReferenceExpression: public UnaryExpression {
    // &inner

    public:
    ReferenceExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "&") {};
};

struct PointerExpression: public UnaryExpression {
    // *inner

    public:
    PointerExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "*") {};
};

struct NegationExpression: public UnaryExpression {
    // -inner

    public:
    NegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "-") {};
};

struct LogicalNegationExpression: public UnaryExpression {
    // !inner

    public:
    LogicalNegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "!") {};
};

struct BinaryExpression: public Expression {
    public:
    BinaryExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right, const char* const op_str)
        : Expression(loc)
        , _left(std::move(left))
        , _right(std::move(right))
        , _op_str(op_str) {};

    void print(std::ostream& stream);

    private:
    ExpressionPtr _left;
    ExpressionPtr _right;

    const char* const _op_str;
};

struct MultiplyExpression: public BinaryExpression {
    // left * right

    public:
    MultiplyExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "*") {};
};

struct AddExpression: public BinaryExpression {
    // left + right

    public:
    AddExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "+") {};
};

struct SubstractExpression: public BinaryExpression {
    // left - right

    public:
    SubstractExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "-") {};
};

struct LessThanExpression: public BinaryExpression {
    // left < right

    public:
    LessThanExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "<") {};
};

struct EqualExpression: public BinaryExpression {
    // left == right

    public:
    EqualExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "==") {};
};

struct UnequalExpression: public BinaryExpression {
    // left != right

    public:
    UnequalExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "!=") {};
};

struct AndExpression: public BinaryExpression {
    // left && right

    public:
    AndExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "&&") {};
};

struct OrExpression: public BinaryExpression {
    // left || right

    public:
    OrExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "||") {};
};

struct TernaryExpression: public Expression {
    // condition ? left : right

    public:
    TernaryExpression(Locatable loc, ExpressionPtr condition, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : Expression(loc)
        , _condition(std::move(condition))
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    private:
    ExpressionPtr _condition;
    ExpressionPtr _left;
    ExpressionPtr _right;
};

struct AssignExpression: public BinaryExpression {
    // left = right

    public:
    AssignExpression(Locatable loc, ExpressionPtr left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc, std::move(left), std::move(right), "=") {};
};

#pragma once

#include <memory>
#include <vector>
#include <string.h>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"
#include "../lexer/token.h"
#include "declarator.h"

#include "declarator.h"

struct Expression {
    Expression(Locatable loc)
        : loc(loc) {};

    Locatable loc;

    virtual void print(std::ostream& stream) = 0;

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr);
};

struct IdentExpression: public Expression {
    public:
    IdentExpression(Locatable loc, Symbol ident)
        : Expression(loc)
        , _ident(ident) {};

    void print(std::ostream& stream);

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
    IndexExpression(Locatable loc, std::unique_ptr<Expression> expression, std::unique_ptr<Expression> index)
        : Expression(loc)
        , _expression(std::move(expression))
        , _index(std::move(index)) {};

    void print(std::ostream& stream);

    // expression[index]
    private:
    std::unique_ptr<Expression> _expression;
    std::unique_ptr<Expression> _index;
};

struct CallExpression: public Expression {
    public:
    CallExpression(Locatable loc, std::unique_ptr<Expression> expression, std::vector<std::unique_ptr<Expression>> arguments)
        : Expression(loc)
        , _expression(std::move(expression))
        , _arguments(std::move(arguments)) {};

    void print(std::ostream& stream);

    // expression(args)
    std::unique_ptr<Expression> _expression;
    std::vector<std::unique_ptr<Expression>> _arguments;
};

struct DotExpression: public Expression {
    public:
    DotExpression(Locatable loc, std::unique_ptr<Expression> expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)) {};

    void print(std::ostream& stream);

    // expression.ident
    private:
    std::unique_ptr<Expression> _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct ArrowExpression: public Expression {
    public:
    ArrowExpression(Locatable loc, std::unique_ptr<Expression> expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)) {};

    void print(std::ostream& stream);

    // expression->ident
    private:
    std::unique_ptr<Expression> _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct UnaryExpression: public Expression {
    public:
    UnaryExpression(Locatable loc)
        : Expression(loc) {};
    
    virtual void print(std::ostream& stream) = 0;
};

struct SizeofExpression: public UnaryExpression {
    public:
    SizeofExpression(Locatable loc, std::unique_ptr<Expression> inner)
        : UnaryExpression(loc)
        , _inner(std::move(inner)) {};

    void print(std::ostream& stream);

    // sizeof inner
    private:
    std::unique_ptr<Expression> _inner;
};

struct SizeofTypeExpression: public UnaryExpression {
    public:
    SizeofTypeExpression(Locatable loc, std::unique_ptr<TypeSpecifier> type)
        : UnaryExpression(loc)
        , _type(std::move(type)) {};

    void print(std::ostream& stream);

    // sizeof (inner)
    private:
    std::unique_ptr<TypeSpecifier> _type;
};

struct ReferenceExpression: public UnaryExpression {
    public:
    ReferenceExpression(Locatable loc, std::unique_ptr<Expression> inner)
        : UnaryExpression(loc)
        , _inner(std::move(inner)) {};

    void print(std::ostream& stream);

    // &inner
    private:
    std::unique_ptr<Expression> _inner;
};

struct PointerExpression: public UnaryExpression {
    public:
    PointerExpression(Locatable loc, std::unique_ptr<Expression> inner)
        : UnaryExpression(loc)
        , _inner(std::move(inner)) {};

    void print(std::ostream& stream);

    // *inner
    private:
    std::unique_ptr<Expression> _inner;
};

struct NegationExpression: public UnaryExpression {
    public:
    NegationExpression(Locatable loc, std::unique_ptr<Expression> inner)
        : UnaryExpression(loc)
        , _inner(std::move(inner)) {};

    void print(std::ostream& stream);

    // -inner
    private:
    std::unique_ptr<Expression> _inner;
};

struct LogicalNegationExpression: public UnaryExpression {
    public:
    LogicalNegationExpression(Locatable loc, std::unique_ptr<Expression> inner)
        : UnaryExpression(loc)
        , _inner(std::move(inner)) {};

    void print(std::ostream& stream);

    // !inner
    private:
    std::unique_ptr<Expression> _inner;
};

struct BinaryExpression: public Expression {
    public:
    BinaryExpression(Locatable loc)
        : Expression(loc) {};

    virtual void print(std::ostream& stream) = 0;
};

struct MultiplyExpression: public BinaryExpression {
    public:
    MultiplyExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left * right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct AddExpression: public BinaryExpression {
    public:
    AddExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left + right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct SubstractExpression: public BinaryExpression {
    public:
    SubstractExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left - right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct LessThanExpression: public BinaryExpression {
    public:
    LessThanExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left < right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct EqualExpression: public BinaryExpression {
    public:
    EqualExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left == right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct UnequalExpression: public BinaryExpression {
    public:
    UnequalExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left != right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct AndExpression: public BinaryExpression {
    public:
    AndExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left && right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct OrExpression: public BinaryExpression {
    public:
    OrExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left && right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct TernaryExpression: public Expression {
    public:
    TernaryExpression(Locatable loc, std::unique_ptr<Expression> condition, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : Expression(loc)
        , _condition(std::move(condition))
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // condition ? left : right
    private:
    std::unique_ptr<Expression> _condition;
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};

struct AssignExpression: public BinaryExpression {
    public:
    AssignExpression(Locatable loc, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : BinaryExpression(loc)
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    // left = right
    private:
    std::unique_ptr<Expression> _left;
    std::unique_ptr<Expression> _right;
};


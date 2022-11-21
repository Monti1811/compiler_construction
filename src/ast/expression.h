#pragma once

#include <vector>
#include <string.h>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"
#include "../lexer/token.h"
#include "declarator.h"

struct Expression {
    public:
    // TODO: Implement Locatable
};

struct IdentExpression: public Expression {
    public:
    IdentExpression(Symbol ident) : _ident(*ident) {};
    private:
    std::string _ident;
};

struct IntConstantExpression: public Expression {
    public:
    IntConstantExpression(Symbol value) : _value( std::stoi(*value) ) {};
    private:
    int _value;
};

struct CharConstantExpression: public Expression {
    public:
    CharConstantExpression(Symbol value) : _value( (*value)[0] ) {};
    private:
    char _value;
};

struct StringLiteralExpression: public Expression {
    public:
    StringLiteralExpression(Symbol value) : _value(*value) {};
    private:
    std::string _value;
};

struct IndexExpression: public Expression {
    // expression[index]
    public:
    IndexExpression(Expression expression, Expression index) : _expression(expression), _index(index) {};
    private:
    Expression _expression;
    Expression _index;
};

struct CallExpression: public Expression {
    // expression(args)
    public:
    CallExpression(Expression expression, std::vector<Expression> arguments) : _expression(expression), _arguments(arguments) {};
    private:
    Expression _expression;
    std::vector<Expression> _arguments;
};

struct DotExpression: public Expression {
    // expression.ident
    public:
    DotExpression(Expression expression, IdentExpression ident) : _expression(expression), _ident(ident) {};
    private:
    Expression _expression;
    IdentExpression _ident;
};

struct ArrowExpression: public Expression {
    // expression->ident
    public:
    ArrowExpression(Expression expression, IdentExpression ident) : _expression(expression), _ident(ident) {};
    private:
    Expression _expression;
    IdentExpression _ident;
};

struct UnaryExpression: public Expression {};

struct SizeofExpression: public UnaryExpression {
    // sizeof inner
    public:
    SizeofExpression(Expression inner) : _inner(inner) {};
    private:
    Expression _inner;
};

struct SizeofTypeExpression: public UnaryExpression {
    // sizeof (inner)
    public:
    SizeofTypeExpression(TypeSpecifier type) : _type(type) {};
    private:
    TypeSpecifier _type;
};

struct ReferenceExpression: public UnaryExpression {
    // &inner
    public:
    ReferenceExpression(Expression inner) : _inner(inner) {};
    private:
    Expression _inner;
};

struct PointerExpression: public UnaryExpression {
    // *inner
    public:
    PointerExpression(Expression inner) : _inner(inner) {};
    private:
    Expression _inner;
};

struct NegationExpression: public UnaryExpression {
    // -inner
    public:
    NegationExpression(Expression inner) : _inner(inner) {};
    private:
    Expression _inner;
};

struct LogicalNegationExpression: public UnaryExpression {
    // !inner
    public:
    LogicalNegationExpression(Expression inner) : _inner(inner) {};
    private:
    Expression _inner;
};

struct BinaryExpression: public Expression {};

struct MultiplyExpression: public BinaryExpression {
    // left * right
    public:
    MultiplyExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct AddExpression: public BinaryExpression {
    // left + right
    public:
    AddExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct SubstractExpression: public BinaryExpression {
    // left - right
    public:
    SubstractExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct LessThanExpression: public BinaryExpression {
    // left < right
    public:
    LessThanExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct EqualExpression: public BinaryExpression {
    // left == right
    public:
    EqualExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct UnequalExpression: public BinaryExpression {
    // left != right
    public:
    UnequalExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct AndExpression: public BinaryExpression {
    // left && right
    public:
    AndExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct OrExpression: public BinaryExpression {
    // left && right
    public:
    OrExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};

struct TernaryExpression: public Expression {
    // condition ? left : right
    public:
    TernaryExpression(Expression condition, Expression left, Expression right) : _condition(condition), _left(left), _right(right) {};
    private:
    Expression _condition;
    Expression _left;
    Expression _right;
};

struct AssignExpression: public BinaryExpression {
    // left = right
    public:
    AssignExpression(Expression left, Expression right) : _left(left), _right(right) {};
    private:
    Expression _left;
    Expression _right;
};


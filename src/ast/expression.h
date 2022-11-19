#pragma once

#include <vector>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

struct Expression: public Locatable {
    // TODO: Implement Locatable
};

struct IdentExpression: public Expression {
    Symbol ident;
};

struct IntConstantExpression: public Expression {
    int value;
};

struct CharConstantExpression: public Expression {
    char value;
};

struct StringLiteralExpression: public Expression {
    Symbol value;
};

struct IndexExpression: public Expression {
    // expression[index]
    Expression expression;
    Expression index;
};

struct CallExpression: public Expression {
    // expression(args)
    Expression expression;
    std::vector<Expression> arguments;
};

struct DotExpression: public Expression {
    // expression.ident
    Expression expression;
    IdentExpression ident;
};

struct ArrowExpression: public Expression {
    // expression->ident
    Expression expression;
    IdentExpression ident;
};

struct UnaryExpression {};

struct SizeofExpression: public UnaryExpression {
    // sizeof inner
    Expression inner;
};

struct SizeofTypeExpression: public UnaryExpression {
    // sizeof (inner)
    TypeSpecifier type;
};

struct ReferenceExpression: public UnaryExpression {
    // &inner
    Expression inner;
};

struct PointerExpression: public UnaryExpression {
    // *inner
    Expression inner;
};

struct NegationExpression: public UnaryExpression {
    // -inner
    Expression inner;
};

struct LogicalNegationExpression: public UnaryExpression {
    // !inner
    Expression inner;
};

struct BinaryExpression {};

struct MultiplyExpression: public BinaryExpression {
    // left * right
    Expression left;
    UnaryExpression right;
};

struct BinaryExpression {};

struct AddExpression: public BinaryExpression {
    // left + right
    Expression left;
    Expression right;
};

struct SubstractExpression: public BinaryExpression {
    // left - right
    Expression left;
    Expression right;
};

struct LessThanExpression: public BinaryExpression {
    // left < right
    Expression left;
    Expression right;
};

struct EqualExpression: public BinaryExpression {
    // left == right
    Expression left;
    Expression right;
};

struct UnequalExpression: public BinaryExpression {
    // left != right
    Expression left;
    Expression right;
};

struct AndExpression: public BinaryExpression {
    // left && right
    Expression left;
    Expression right;
};

struct OrExpression: public BinaryExpression {
    // left && right
    Expression left;
    Expression right;
};

struct TernaryExpression: public Expression {
    // condition ? left : right
    Expression condition;
    Expression left;
    Expression right;
};

struct AssignExpression: public BinaryExpression {
    // left = right
    Expression left;
    Expression right;
};


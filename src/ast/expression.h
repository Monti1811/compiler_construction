#pragma once

#include <vector>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

struct Expression: public Locatable {
    // TODO: Implement Locatable
};

struct PrimaryExpression {};

struct IdentExpression: public PrimaryExpression {
    Symbol ident;
};

struct ConstantExpression: public PrimaryExpression {};

struct IntConstantExpression: public ConstantExpression {
    int value;
};

struct CharConstantExpression: public ConstantExpression {
    char value;
};

struct StringLiteralExpression: public PrimaryExpression {
    Symbol value;
};

struct PostfixExpression {};

struct BasePostfixExpression: public PostfixExpression {
    // TODO: this is not necessary
    PrimaryExpression inner;
};

struct IndexExpression: public PostfixExpression {
    // expression[index]
    PostfixExpression expression;
    Expression index;
};

struct CallExpression: public PostfixExpression {
    // expression(args)
    PostfixExpression expression;
    std::vector<AssignmentExpression> arguments;
};

struct DotExpression: public PostfixExpression {
    // expression.ident
    PostfixExpression expression;
    IdentExpression ident;
};

struct ArrowExpression: public PostfixExpression {
    // expression->ident
    PostfixExpression expression;
    IdentExpression ident;
};

struct UnaryExpression {};

struct BaseUnaryExpression: public UnaryExpression {
    // TODO: this is not necessary
    PostfixExpression inner;
};

struct SizeofExpression: public UnaryExpression {
    // sizeof inner
    UnaryExpression inner;
};

struct ReferenceExpression: public UnaryExpression {
    // &inner
    UnaryExpression inner;
};

struct PointerExpression: public UnaryExpression {
    // *inner
    UnaryExpression inner;
};

struct NegationExpression: public UnaryExpression {
    // -inner
    UnaryExpression inner;
};

struct LogicalNegationExpression: public UnaryExpression {
    // !inner
    UnaryExpression inner;
};

struct MultiplicativeExpression {};

struct BaseMultiplicativeExpression: public MultiplicativeExpression {
    // TODO: This is not necessary
    UnaryExpression inner;
};

struct MultiplyExpression: public MultiplicativeExpression {
    // left * right
    MultiplicativeExpression left;
    UnaryExpression right;
};

struct AdditiveExpression {};

struct BaseAdditiveExpression: public AdditiveExpression {
    // TODO: This is not necessary
    MultiplicativeExpression inner;
};

struct AddExpression: public AdditiveExpression {
    // left + right
    AdditiveExpression left;
    MultiplicativeExpression right;
};

struct SubstractExpression: public AdditiveExpression {
    // left - right
    AdditiveExpression left;
    MultiplicativeExpression right;
};

struct RelationalExpression {};

struct BaseRelationalExpression: public RelationalExpression {
    // TODO
    AdditiveExpression inner;
};

struct LessThanExpression: public RelationalExpression {
    // left < right
    RelationalExpression left;
    AdditiveExpression right;
};

struct EqualityExpression {};

struct BaseEqualityExpression: public EqualityExpression {
    // TODO
    RelationalExpression inner;
};

struct EqualExpression: public EqualityExpression {
    // left == right
    EqualityExpression left;
    RelationalExpression right;
};

struct UnequalExpression: public EqualityExpression {
    // left != right
    EqualityExpression left;
    RelationalExpression right;
};

struct LogicalAndExpression {};

struct BaseLogicalAndExpression: public LogicalAndExpression {
    // TODO
    EqualityExpression inner;
};

struct BinaryLogicalAndExpression: public LogicalAndExpression {
    // left && right
    LogicalAndExpression left;
    EqualityExpression right;
};

struct LogicalOrExpression {};

struct BaseLogicalOrExpression: public LogicalOrExpression {
    // TODO
    LogicalAndExpression inner;
};

struct BinaryLogicalOrExpression: public LogicalOrExpression {
    // left && right
    LogicalOrExpression left;
    LogicalAndExpression right;
};

struct ConditionalExpression {};

struct BaseConditionalExpression: public ConditionalExpression {
    // TODO
    LogicalOrExpression inner;
};

struct TernaryExpression: public ConditionalExpression {
    // condition ? left : right
    LogicalOrExpression condition;
    Expression left;
    ConditionalExpression right;
};

struct AssignmentExpression: public Expression {};

struct BaseAssignmentExpression: public AssignmentExpression {
    // TODO
    ConditionalExpression inner;
};

struct EqualAssignExpression: public AssignmentExpression {
    // left = right
    UnaryExpression left;
    AssignmentExpression right;
};


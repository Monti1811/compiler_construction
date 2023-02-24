#pragma once

#include "expression.h"

struct BinaryExpression : public Expression {
  public:
    BinaryExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right, const char* const op_str)
        : Expression(loc)
        , _left(std::move(left))
        , _right(std::move(right))
        , _op_str(op_str){};

    void print(std::ostream& stream) override;

  protected:
    ExpressionPtr _left;
    ExpressionPtr _right;

  private:
    const char* const _op_str;
};

struct MultiplyExpression : public BinaryExpression {
    // left * right

  public:
    MultiplyExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "*"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct AddExpression : public BinaryExpression {
    // left + right

  public:
    AddExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "+"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct SubtractExpression : public BinaryExpression {
    // left - right

  public:
    SubtractExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "-"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct LessThanExpression : public BinaryExpression {
    // left < right

  public:
    LessThanExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "<"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct EqualExpression : public BinaryExpression {
    // left == right

  public:
    EqualExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "=="){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct UnequalExpression : public BinaryExpression {
    // left != right

  public:
    UnequalExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "!="){};

    TypePtr typecheck(ScopePtr& scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct AndExpression : public BinaryExpression {
    // left && right

  public:
    AndExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "&&"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct OrExpression : public BinaryExpression {
    // left || right

  public:
    OrExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "||"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct AssignExpression : public BinaryExpression {
    // left = right

  public:
    AssignExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "="){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

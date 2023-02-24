#pragma once

#include "expression.h"

#include "../declaration.h"

struct UnaryExpression : public Expression {
  public:
    UnaryExpression(Locatable loc, ExpressionPtr inner, const char* const op_str)
        : Expression(loc)
        , _inner(std::move(inner))
        , _op_str(op_str){};

    void print(std::ostream& stream) override;

  protected:
    ExpressionPtr _inner;

  private:
    const char* const _op_str;
};

struct SizeofExpression : public UnaryExpression {
    // sizeof inner

  public:
    SizeofExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "sizeof "){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct SizeofTypeExpression : public Expression {
    // sizeof (inner)

  public:
    SizeofTypeExpression(Locatable loc, Declaration decl)
        : Expression(loc)
        , _decl(std::move(decl)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    Declaration _decl;
    TypePtr _inner_type;
};

struct ReferenceExpression : public UnaryExpression {
    // &inner

  public:
    ReferenceExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "&"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

    std::optional<size_t> getStringLength(void) override;
};

struct DerefExpression : public UnaryExpression {
    // *inner

  public:
    DerefExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "*"){};

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr& scope) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

    std::optional<size_t> getStringLength(void) override;
};

struct NegationExpression : public UnaryExpression {
    // -inner

  public:
    NegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "-"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct LogicalNegationExpression : public UnaryExpression {
    // !inner

  public:
    LogicalNegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "!"){};

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

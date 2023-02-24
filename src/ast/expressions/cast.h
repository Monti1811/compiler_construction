#pragma once

#include "expression.h"

struct CastExpression : public Expression {
    // Just used for type conversion during typecheck phase

  public:
    CastExpression(Locatable loc, ExpressionPtr inner)
        : Expression(loc)
        , _inner(std::move(inner)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    std::optional<llvm::Value*> convertNullptrs(CompileScopePtr compile_scope);
    llvm::Value* castArithmetics(CompileScopePtr compile_scope, llvm::Value* value);

    ExpressionPtr _inner;
};

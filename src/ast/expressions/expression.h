#pragma once

#include <memory>

#include "../../util/diagnostic.h"

#include "../compile_scope.h"
#include "../scope.h"
#include "../types.h"

struct Expression {
    Expression(Locatable loc)
        : loc(loc){};
    virtual ~Expression() = default;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr);

    /// Typechecks the expression, and returns the type of the expression.
    virtual TypePtr typecheck(ScopePtr& scope) = 0;

    /// Typechecks the expression, and wraps functions inside into function pointers.
    ///
    /// 6.3.2.1.4:
    /// Except when it is the operand of the sizeof operator [...] or the unary & operator,
    /// a function designator with type "function returning type" is converted to an expression that
    /// has type "pointer to function returning type".
    TypePtr typecheckWrap(ScopePtr& scope);

    /// Returns true if the expression is an lvalue
    virtual bool isLvalue(ScopePtr& scope);

    virtual llvm::Value* compileLValue(CompileScopePtr compile_scope);
    virtual llvm::Value* compileRValue(CompileScopePtr compile_scope) = 0;

    /// If this is a string literal, return the length of the string
    virtual std::optional<size_t> getStringLength(void);

    Locatable loc;
    TypePtr type;
};

typedef std::unique_ptr<Expression> ExpressionPtr;

ExpressionPtr castExpression(ExpressionPtr expr, TypePtr type);

llvm::Value* toBoolTy(llvm::Value* to_convert, CompileScopePtr compile_scope);

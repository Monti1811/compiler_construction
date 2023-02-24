#pragma once

#include "../../util/symbol_internalizer.h"

#include "expression.h"

struct IdentExpression : public Expression {
  public:
    IdentExpression(Locatable loc, Symbol ident)
        : Expression(loc)
        , _ident(ident){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr& scope) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    Symbol _ident;
};

struct IntConstantExpression : public Expression {
  public:
    IntConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(std::stoull(*value)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr&) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    unsigned long long _value;
};

struct NullPtrExpression : public Expression {
  public:
    NullPtrExpression(Locatable loc)
        : Expression(loc){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr&) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct CharConstantExpression : public Expression {
  public:
    CharConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value((*value)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr&) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    char getChar();

    std::string _value;
};

struct StringLiteralExpression : public Expression {
  public:
    StringLiteralExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(*value){};

    TypePtr typecheck(ScopePtr&) override;
    bool isLvalue(ScopePtr& scope) override;

    void print(std::ostream& stream) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

    std::string getString();
    std::optional<size_t> getStringLength(void) override;

  private:
    std::string _value;
};

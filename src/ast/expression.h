#pragma once

#include <memory>
#include <string.h>
#include <vector>

#include "../lexer/token.h"
#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

#include "compile_scope.h"
#include "declaration.h"
#include "scope.h"
#include "specifiers/specifier.h"
#include "types.h"

struct Expression {
    Expression(Locatable loc)
        : loc(loc){};
    virtual ~Expression() = default;

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

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr);

    // If this is a string literal, return the length of the string
    virtual std::optional<size_t> getStringLength(void);

    Locatable loc;
    TypePtr type;
};

typedef std::unique_ptr<Expression> ExpressionPtr;

ExpressionPtr castExpression(ExpressionPtr expr, TypePtr type);

struct IdentExpression : public Expression {
  public:
    IdentExpression(Locatable loc, Symbol ident)
        : Expression(loc)
        , _ident(ident){};

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr& scope) override;

    void print(std::ostream& stream) override;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<IdentExpression>& expr);
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

    TypePtr typecheck(ScopePtr&) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
    void print(std::ostream& stream) override;

  private:
    unsigned long long _value;
};

struct NullPtrExpression : public Expression {
  public:
    NullPtrExpression(Locatable loc)
        : Expression(loc){};

    TypePtr typecheck(ScopePtr&) override;

    void print(std::ostream& stream) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

struct CharConstantExpression : public Expression {
  public:
    CharConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value((*value)){};

    TypePtr typecheck(ScopePtr&) override;

    void print(std::ostream& stream) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

    char getChar();

  private:
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

struct IndexExpression : public Expression {
  public:
    IndexExpression(Locatable loc, ExpressionPtr expression, ExpressionPtr index)
        : Expression(loc)
        , _expression(std::move(expression))
        , _index(std::move(index)){};

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr&) override;

    void print(std::ostream& stream) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression[index]
    ExpressionPtr _expression;
    ExpressionPtr _index;
    bool _swapped = false;
};

struct CallExpression : public Expression {
  public:
    CallExpression(Locatable loc, ExpressionPtr expression, std::vector<ExpressionPtr> arguments)
        : Expression(loc)
        , _expression(std::move(expression))
        , _arguments(std::move(arguments)){};

    TypePtr typecheck(ScopePtr& scope) override;

    void print(std::ostream& stream) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression(arguments)
    ExpressionPtr _expression;
    std::vector<ExpressionPtr> _arguments;
};

struct DotExpression : public Expression {
  public:
    DotExpression(Locatable loc, ExpressionPtr expression, Symbol ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr& scope) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression.ident
    ExpressionPtr _expression;
    Symbol _ident;
};

struct ArrowExpression : public Expression {
  public:
    ArrowExpression(Locatable loc, ExpressionPtr expression, Symbol ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr& scope) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression->ident
    ExpressionPtr _expression;
    Symbol _ident;
};

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
  public:
    SizeofTypeExpression(Locatable loc, Declaration decl)
        : Expression(loc)
        , _decl(std::move(decl)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // sizeof (inner)
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

struct BinaryExpression : public Expression {
  public:
    BinaryExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right, const char* const op_str)
        : Expression(loc)
        , _left(std::move(left))
        , _right(std::move(right))
        , _op_str(op_str){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;

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

struct TernaryExpression : public Expression {
    // condition ? left : right

  public:
    TernaryExpression(Locatable loc, ExpressionPtr condition, ExpressionPtr left, ExpressionPtr right)
        : Expression(loc)
        , _condition(std::move(condition))
        , _left(std::move(left))
        , _right(std::move(right)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    ExpressionPtr _condition;
    ExpressionPtr _left;
    ExpressionPtr _right;
};

struct AssignExpression : public BinaryExpression {
    // left = right

  public:
    AssignExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "="){};

    TypePtr typecheck(ScopePtr& scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;
};

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

    std::optional<llvm::Value*> convertNullptrs(CompileScopePtr compile_scope);
    llvm::Value* castArithmetics(CompileScopePtr compile_scope, llvm::Value* value);

  private:
    ExpressionPtr _inner;
};

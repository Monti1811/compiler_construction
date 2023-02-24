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

    virtual llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr) = 0;
    virtual llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr) = 0;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr);

    // If this is a string literal, return the length of the string
    virtual std::optional<size_t> getStringLength(void) {
        return std::nullopt;
    }

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

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<IdentExpression>& expr);
    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);

  public:
    Symbol _ident;
};

struct IntConstantExpression : public Expression {
  public:
    IntConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(std::stoull(*value)){};

    TypePtr typecheck(ScopePtr&);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
    void print(std::ostream& stream);

    unsigned long long _value;
};

struct NullPtrExpression : public Expression {
  public:
    NullPtrExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(std::stoull(*value)){};

    TypePtr typecheck(ScopePtr&);

    void print(std::ostream& stream);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
    unsigned long long _value;
};

struct CharConstantExpression : public Expression {
  public:
    CharConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value((*value)){};

    TypePtr typecheck(ScopePtr&);

    void print(std::ostream& stream);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);

    char getChar();

    std::string _value;
};

struct StringLiteralExpression : public Expression {
  public:
    StringLiteralExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(*value){};

    TypePtr typecheck(ScopePtr&);
    bool isLvalue(ScopePtr& scope);

    void print(std::ostream& stream);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);

    std::string getString();
    std::optional<size_t> getStringLength(void);

    std::string _value;
};

struct IndexExpression : public Expression {
  public:
    IndexExpression(Locatable loc, ExpressionPtr expression, ExpressionPtr index)
        : Expression(loc)
        , _expression(std::move(expression))
        , _index(std::move(index)){};

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr&);

    void print(std::ostream& stream);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
    // expression[index]
  private:
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

    TypePtr typecheck(ScopePtr& scope);

    void print(std::ostream& stream);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
    // expression(args)
    ExpressionPtr _expression;
    std::vector<ExpressionPtr> _arguments;
};

struct DotExpression : public Expression {
  public:
    DotExpression(Locatable loc, ExpressionPtr expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)){};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
    // expression.ident
  private:
    ExpressionPtr _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct ArrowExpression : public Expression {
  public:
    ArrowExpression(Locatable loc, ExpressionPtr expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)){};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
    // expression->ident
  private:
    ExpressionPtr _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct UnaryExpression : public Expression {
  public:
    UnaryExpression(Locatable loc, ExpressionPtr inner, const char* const op_str)
        : Expression(loc)
        , _inner(std::move(inner))
        , _op_str(op_str){};

    void print(std::ostream& stream);

    ExpressionPtr _inner;

  private:
    const char* const _op_str;
};

struct SizeofExpression : public UnaryExpression {
    // sizeof inner

  public:
    SizeofExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "sizeof "){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct SizeofTypeExpression : public Expression {
  public:
    SizeofTypeExpression(Locatable loc, Declaration decl)
        : Expression(loc)
        , _decl(std::move(decl)){};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
    // sizeof (inner)
  private:
    Declaration _decl;
    TypePtr _inner_type;
};

struct ReferenceExpression : public UnaryExpression {
    // &inner

  public:
    ReferenceExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "&"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);

    std::optional<size_t> getStringLength(void);
};

struct DerefExpression : public UnaryExpression {
    // *inner

  public:
    DerefExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "*"){};

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);
    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);

    std::optional<size_t> getStringLength(void);
};

struct NegationExpression : public UnaryExpression {
    // -inner

  public:
    NegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "-"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct LogicalNegationExpression : public UnaryExpression {
    // !inner

  public:
    LogicalNegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "!"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct BinaryExpression : public Expression {
  public:
    BinaryExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right, const char* const op_str)
        : Expression(loc)
        , _left(std::move(left))
        , _right(std::move(right))
        , _op_str(op_str){};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);

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
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct AddExpression : public BinaryExpression {
    // left + right

  public:
    AddExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "+"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct SubtractExpression : public BinaryExpression {
    // left - right

  public:
    SubtractExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "-"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct LessThanExpression : public BinaryExpression {
    // left < right

  public:
    LessThanExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "<"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct EqualExpression : public BinaryExpression {
    // left == right

  public:
    EqualExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "=="){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct UnequalExpression : public BinaryExpression {
    // left != right

  public:
    UnequalExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "!="){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct AndExpression : public BinaryExpression {
    // left && right

  public:
    AndExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "&&"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct OrExpression : public BinaryExpression {
    // left || right

  public:
    OrExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "||"){};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct TernaryExpression : public Expression {
    // condition ? left : right

  public:
    TernaryExpression(Locatable loc, ExpressionPtr condition, ExpressionPtr left, ExpressionPtr right)
        : Expression(loc)
        , _condition(std::move(condition))
        , _left(std::move(left))
        , _right(std::move(right)){};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);

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

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compileLValue(std::shared_ptr<CompileScope> CompileScopePtr);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> CompileScopePtr);
};

struct CastExpression : public Expression {
    // Just used for type conversion during typecheck phase

  public:
    CastExpression(Locatable loc, ExpressionPtr inner)
        : Expression(loc)
        , _inner(std::move(inner)){};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);

    llvm::Value* compileLValue(std::shared_ptr<CompileScope> compile_scope);
    llvm::Value* compileRValue(std::shared_ptr<CompileScope> compile_scope);

    std::optional<llvm::Value*> convertNullptrs(std::shared_ptr<CompileScope> compile_scope);
    llvm::Value* castArithmetics(std::shared_ptr<CompileScope> compile_scope, llvm::Value* value);

    ExpressionPtr _inner;
};

#pragma once

#include <memory>
#include <vector>
#include <string.h>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"
#include "../lexer/token.h"

#include "scope.h"
#include "specifiers/specifier.h"
#include "types.h"
#include "compile_scope.h"

struct Expression {
    Expression(Locatable loc)
        : loc(loc) {};
    virtual ~Expression() = default;
    Locatable loc;

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

    virtual llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent) = 0;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Expression>& expr);
};

typedef std::unique_ptr<Expression> ExpressionPtr;

struct IdentExpression: public Expression {
    public:
    IdentExpression(Locatable loc, Symbol ident)
        : Expression(loc)
        , _ident(ident) {};

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<IdentExpression>& expr);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    public:
    Symbol _ident;
};

struct IntConstantExpression: public Expression {
    public:
    IntConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(std::stoull(*value)) {};

    TypePtr typecheck(ScopePtr&);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    void print(std::ostream& stream);

    unsigned long long _value;
};

struct NullPtrExpression: public Expression {
    public:
    NullPtrExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(std::stoull(*value)) {};

    TypePtr typecheck(ScopePtr&);

    void print(std::ostream& stream);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    unsigned long long _value;
};

struct CharConstantExpression: public Expression {
    public:
    CharConstantExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value( (*value) ) {};

    TypePtr typecheck(ScopePtr&);

    void print(std::ostream& stream);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    std::string _value;
};

struct StringLiteralExpression: public Expression {
    public:
    StringLiteralExpression(Locatable loc, Symbol value)
        : Expression(loc)
        , _value(*value) {};

    TypePtr typecheck(ScopePtr&);
    bool isLvalue(ScopePtr& scope);

    void print(std::ostream& stream);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    private:
    std::string _value;
};

struct IndexExpression: public Expression {
    public:
    IndexExpression(Locatable loc, ExpressionPtr expression, ExpressionPtr index)
        : Expression(loc)
        , _expression(std::move(expression))
        , _index(std::move(index)) {};

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr&);

    void print(std::ostream& stream);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    // expression[index]
    private:
    ExpressionPtr _expression;
    ExpressionPtr _index;
};

struct CallExpression: public Expression {
    public:
    CallExpression(Locatable loc, ExpressionPtr expression, std::vector<ExpressionPtr> arguments)
        : Expression(loc)
        , _expression(std::move(expression))
        , _arguments(std::move(arguments)) {};

    TypePtr typecheck(ScopePtr& scope);

    void print(std::ostream& stream);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    // expression(args)
    ExpressionPtr _expression;
    std::vector<ExpressionPtr> _arguments;
};

struct DotExpression: public Expression {
    public:
    DotExpression(Locatable loc, ExpressionPtr expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    // expression.ident
    private:
    ExpressionPtr _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct ArrowExpression: public Expression {
    public:
    ArrowExpression(Locatable loc, ExpressionPtr expression, std::unique_ptr<IdentExpression> ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    // expression->ident
    private:
    ExpressionPtr _expression;
    std::unique_ptr<IdentExpression> _ident;
};

struct UnaryExpression: public Expression {
    public:
    UnaryExpression(Locatable loc, ExpressionPtr inner, const char* const op_str)
        : Expression(loc)
        , _inner(std::move(inner))
        , _op_str(op_str) {};
    
    void print(std::ostream& stream);

    ExpressionPtr _inner;

    private:
    const char* const _op_str;
};

struct SizeofExpression: public UnaryExpression {
    // sizeof inner

    public:
    SizeofExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "sizeof ") {};
        
    TypePtr typecheck(ScopePtr& scope);    
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct SizeofTypeExpression: public Expression {
    public:
    SizeofTypeExpression(Locatable loc, TypeSpecifierPtr type)
        : Expression(loc)
        , _type(std::move(type)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    // sizeof (inner)
    private:
    TypeSpecifierPtr _type;
};

struct ReferenceExpression: public UnaryExpression {
    // &inner

    public:
    ReferenceExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "&") {};
    
    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct DerefExpression: public UnaryExpression {
    // *inner

    public:
    DerefExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "*") {};

    TypePtr typecheck(ScopePtr& scope);
    bool isLvalue(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct NegationExpression: public UnaryExpression {
    // -inner

    public:
    NegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "-") {};
        
    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct LogicalNegationExpression: public UnaryExpression {
    // !inner

    public:
    LogicalNegationExpression(Locatable loc, ExpressionPtr inner)
        : UnaryExpression(loc, std::move(inner), "!") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct BinaryExpression: public Expression {
    public:
    BinaryExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right, const char* const op_str)
        : Expression(loc)
        , _left(std::move(left))
        , _right(std::move(right))
        , _op_str(op_str) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);

    ExpressionPtr _left;
    ExpressionPtr _right;

    private:
    const char* const _op_str;
};

struct MultiplyExpression: public BinaryExpression {
    // left * right

    public:
    MultiplyExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "*") {};
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct AddExpression: public BinaryExpression {
    // left + right

    public:
    AddExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "+") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct SubstractExpression: public BinaryExpression {
    // left - right

    public:
    SubstractExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "-") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct LessThanExpression: public BinaryExpression {
    // left < right

    public:
    LessThanExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "<") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct EqualExpression: public BinaryExpression {
    // left == right

    public:
    EqualExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "==") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct UnequalExpression: public BinaryExpression {
    // left != right

    public:
    UnequalExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "!=") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct AndExpression: public BinaryExpression {
    // left && right

    public:
    AndExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "&&") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct OrExpression: public BinaryExpression {
    // left || right

    public:
    OrExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "||") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

struct TernaryExpression: public Expression {
    // condition ? left : right

    public:
    TernaryExpression(Locatable loc, ExpressionPtr condition, ExpressionPtr left, ExpressionPtr right)
        : Expression(loc)
        , _condition(std::move(condition))
        , _left(std::move(left))
        , _right(std::move(right)) {};

    void print(std::ostream& stream);

    TypePtr typecheck(ScopePtr& scope);

    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
    private:
    ExpressionPtr _condition;
    ExpressionPtr _left;
    ExpressionPtr _right;
};

struct AssignExpression: public BinaryExpression {
    // left = right

    public:
    AssignExpression(Locatable loc, ExpressionPtr left, ExpressionPtr right)
        : BinaryExpression(loc, std::move(left), std::move(right), "=") {};

    TypePtr typecheck(ScopePtr& scope);
    llvm::Value* compile(std::shared_ptr<CompileScope> CompileScopePtr, llvm::Function* Parent);
};

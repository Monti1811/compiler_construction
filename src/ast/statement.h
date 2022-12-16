#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

#include "expression.h"
#include "declarator.h"
#include "types.h"

enum class StatementKind {
    ST_LABELED,
    ST_BLOCK,
    ST_EMPTY,
    ST_DECLARATION,
    ST_EXPRESSION,
    ST_IF,
    ST_WHILE,
    ST_JUMP
};

struct Statement {
    Statement(Locatable loc, const StatementKind kind)
        : loc(loc)
        , kind(kind) {};
    virtual ~Statement() = default;

    Locatable loc;
    const StatementKind kind;

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Statement>& stat);
    virtual void print(std::ostream& stream) = 0;

    virtual void typecheck(ScopePtr& scope) = 0;
};

typedef std::unique_ptr<Statement> StatementPtr;

// label:
// label: statement
struct LabeledStatement: public Statement {
    LabeledStatement(Locatable loc, Symbol name, StatementPtr inner)
        : Statement(loc, StatementKind::ST_LABELED)
        , _name(name)
        , _inner(std::move(inner)) {};

    private:
    Symbol _name;
    StatementPtr _inner;

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope) {
        // TODO: Check that labels are unique within a function
        this->_inner->typecheck(scope);
    }
};

/// Block statement, e.g.:
/// ```
/// {
///    stmt1;
///    stmt2;
/// }
/// ```
struct BlockStatement: public Statement {
    BlockStatement(Locatable loc, std::vector<StatementPtr> items)
        : Statement(loc, StatementKind::ST_BLOCK)
        , _items(std::move(items)) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope) {
        // TODO: Make new scope
        for (auto& item : this->_items) {
            item->typecheck(scope);
        }
    }

    private:
    std::vector<StatementPtr> _items;
};

struct EmptyStatement: public Statement {
    EmptyStatement(Locatable loc)
        : Statement(loc, StatementKind::ST_EMPTY) {};
    
    private:
    void print(std::ostream& stream);

    void typecheck(ScopePtr&) {}
};

// int y;
struct DeclarationStatement: public Statement {
    DeclarationStatement(Locatable loc, Declaration declaration)
        : Statement(loc, StatementKind::ST_DECLARATION)
        , _declaration(std::move(declaration)) {};

    private:
    Declaration _declaration;

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope) {
        this->_declaration.typecheck(scope);
    }
};

// Just an expression disguised as a Statement
struct ExpressionStatement: public Statement {
    ExpressionStatement(Locatable loc, ExpressionPtr expr)
        : Statement(loc, StatementKind::ST_EXPRESSION)
        , _expr(std::move(expr)) {};

    private:
    ExpressionPtr _expr;

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope) {
        this->_expr->typecheck(scope);
    }
};

// if (cond) then stat
// if (cond) then stat else stat
struct IfStatement: public Statement {
    public:
    IfStatement(Locatable loc, ExpressionPtr condition, StatementPtr then_statement, std::optional<StatementPtr> else_statement)
        : Statement(loc, StatementKind::ST_IF)
        , _condition(std::move(condition))
        , _then_statement(std::move(then_statement))
        , _else_statement(std::move(else_statement)) {};
    IfStatement(Locatable loc, ExpressionPtr condition, StatementPtr then_statement)
        : IfStatement(loc, std::move(condition), std::move(then_statement), std::nullopt) {};

    private:
    ExpressionPtr _condition;
    StatementPtr _then_statement;
    std::optional<StatementPtr> _else_statement;

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope) {
        auto condition_type = this->_condition->typecheck(scope);
        if (!condition_type->isScalar()) {
            errorloc(this->_condition->loc, "Condition of an if statement must be scalar");
        }
        this->_then_statement->typecheck(scope);
        if (this->_else_statement.has_value()) {
            this->_else_statement->get()->typecheck(scope);
        }
    }
};

// while (condition) statement
struct WhileStatement: public Statement {
    public:
    WhileStatement(Locatable loc, ExpressionPtr condition, StatementPtr statement)
        : Statement(loc, StatementKind::ST_WHILE)
        , _condition(std::move(condition))
        , _body(std::move(statement)) {};

    private:
    ExpressionPtr _condition;
    StatementPtr _body;

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope) {
        auto condition_type = this->_condition->typecheck(scope);
        if (!condition_type->isScalar()) {
            errorloc(this->loc, "Condition of a while statement must be scalar");
        }
        scope->loop_counter++;
        this->_body->typecheck(scope);
        scope->loop_counter--;
    }
};

// Parent class of jump instructions
struct JumpStatement: public Statement {
    JumpStatement(Locatable loc, Symbol name)
        : Statement(loc, StatementKind::ST_JUMP)
        , _jump_str(*name) {};

    private:        
    const std::string _jump_str;

    void print(std::ostream& stream);

    void typecheck(ScopePtr&) {}
};

// goto identifier
struct GotoStatement: public JumpStatement {
    GotoStatement(Locatable loc, Symbol name, Symbol ident)
        : JumpStatement(loc, name)
        , _ident(ident) {};

    private:
    Symbol _ident;

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope) {
        if (this->_ident->length() == 0) {
            errorloc(this->loc, "Labels cannot be empty");
        }
        if (!scope->isLabelDefined(this->_ident)) {
            errorloc(this->loc, "Missing label");
        }
    }
};

// continue; (in loops)
struct ContinueStatement: public JumpStatement {
    ContinueStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name) {};
    
    void typecheck(ScopePtr& scope) {
        if (scope->loop_counter == 0) {
            errorloc(this->loc, "Invalid 'continue' outside of a loop");
        }
    }
};

// break; (in loops)
struct BreakStatement: public JumpStatement {
    BreakStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name) {};

    void typecheck(ScopePtr& scope) {
        if (scope->loop_counter == 0) {
            errorloc(this->loc, "Invalid 'break' outside of a loop");
        }
    }
};

// return;
// return 1;
struct ReturnStatement: public JumpStatement {
    ReturnStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name)
        , _expr(std::nullopt) {};

    ReturnStatement(Locatable loc, Symbol name, ExpressionPtr expr)
        : JumpStatement(loc, name)
        , _expr(std::make_optional(std::move(expr))) {};

    private:
    std::optional<ExpressionPtr> _expr;

    void print(std::ostream& stream);

    void typecheck(ScopePtr&) {
        // TODO: Check that expression matches return type of current function
        // Also if current function has return type void, expr must not be set
    }
};

// Class implemented as singleton pattern to get the current identation to print the AST correctly
class IdentManager
{
    public:
        static IdentManager& getInstance() {
            static IdentManager instance; 
            return instance;
        }
        int getCurrIdentation();
        void setCurrIdentation(int value);
        void increaseCurrIdentation(int value);
        void decreaseCurrIdentation(int value);
        void printCurrIdentation(std::ostream& stream);
        int currIdent = 0;

        friend std::ostream& operator<<(std::ostream& stream, const IdentManager& ident);
    private:
        IdentManager() {}
        IdentManager(IdentManager const&); 
        IdentManager& operator = (const IdentManager&);
};

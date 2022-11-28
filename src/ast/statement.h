#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

#include "expression.h"
#include "declarator.h"


struct Statement {
    Statement(Locatable loc)
        : loc(loc) {};
    virtual ~Statement() = default;
    Locatable loc;

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Statement>& stat);
    virtual void print(std::ostream& stream) = 0;
};

typedef std::unique_ptr<Statement> StatementPtr;

// label:
// label: statement
struct LabeledStatement: public Statement {
    LabeledStatement(Locatable loc, Symbol name, StatementPtr inner)
        : Statement(loc)
        , _name(name)
        , _inner(std::move(inner)) {};
    Symbol _name;

    StatementPtr _inner;
    void print(std::ostream& stream);
};

/// Block statement, e.g.:
/// ```
/// {
///    stmt1;
///    stmt2;
/// }
/// ```
/// Also represents a null statement (`;`) if `items` is empty.
struct BlockStatement: public Statement {
    BlockStatement(Locatable loc, std::vector<StatementPtr> items)
        : Statement(loc)
        , _items(std::move(items)) {};

    std::vector<StatementPtr> _items;
    void print(std::ostream& stream);
};

// int y;
struct DeclarationStatement: public Statement {
    DeclarationStatement(Locatable loc, Declaration declaration)
        : Statement(loc)
        , _declaration(std::move(declaration)) {};
    Declaration _declaration;
    void print(std::ostream& stream);
};

// Just an expression disguised as a Statement
struct ExpressionStatement: public Statement {
    ExpressionStatement(Locatable loc, ExpressionPtr expr)
        : Statement(loc)
        , _expr(std::move(expr)) {};

    ExpressionPtr _expr;
    void print(std::ostream& stream);
};

// if (cond) then stat
// if (cond) then stat else stat
struct IfStatement: public Statement {
    public:
    IfStatement(Locatable loc, ExpressionPtr condition, StatementPtr then_statement, std::optional<StatementPtr> else_statement)
        : Statement(loc)
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
};

// while (condition) statement
struct WhileStatement: public Statement {
    public:
    WhileStatement(Locatable loc, ExpressionPtr condition, StatementPtr statement)
        : Statement(loc)
        , _condition(std::move(condition))
        , _statement(std::move(statement)) {};

    private:
    ExpressionPtr _condition;
    StatementPtr _statement;
    void print(std::ostream& stream);
};

// Parent class of jump instructions
struct JumpStatement: public Statement {
    JumpStatement(Locatable loc, Symbol name)
        : Statement(loc)
        ,_jump_str(*name) {};
        
    void print(std::ostream& stream);

    const std::string _jump_str;
};

// goto identifier
struct GotoStatement: public JumpStatement {
    GotoStatement(Locatable loc, Symbol name, Symbol ident)
        : JumpStatement(loc, name)
        ,_ident(ident) {};

    Symbol _ident;

    void print(std::ostream& stream);
};

// continue; (in loops)
struct ContinueStatement: public JumpStatement {
    ContinueStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name) {};

};

// break; (in loops)
struct BreakStatement: public JumpStatement {
    BreakStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name) {};

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

    std::optional<ExpressionPtr> _expr;
    void print(std::ostream& stream);
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
    private:
        IdentManager() {}
        IdentManager(IdentManager const&); 
        IdentManager& operator = (const IdentManager&);
};

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

    Locatable loc;

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Statement>& stat);
    virtual void print(std::ostream& stream) = 0;
};

// label:
// label: statement
struct LabeledStatement: public Statement {
    LabeledStatement(Locatable loc, Symbol name)
        : Statement(loc)
        ,_name(name)
        , _inner(std::nullopt) {};
    LabeledStatement(Locatable loc, Symbol name, std::unique_ptr<Statement> inner)
        : Statement(loc)
        ,_name(name)
        , _inner(std::make_optional(std::move(inner))) {};
    Symbol _name;
    std::optional<std::unique_ptr<Statement>> _inner;
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
    BlockStatement(Locatable loc, std::vector<std::unique_ptr<Statement>> items)
        : Statement(loc)
        ,_items(std::move(items)) {};
    ~BlockStatement() = default;
    std::vector<std::unique_ptr<Statement>> _items;
    void print(std::ostream& stream);
};

// int y;
struct DeclarationStatement: public Statement {
    DeclarationStatement(Locatable loc, std::unique_ptr<TypeSpecifier> type, Symbol name)
        : Statement(loc)
        ,_type(std::move(type))
        ,_name(name) {};
    std::unique_ptr<TypeSpecifier> _type;
    Symbol _name;
    void print(std::ostream& stream);
};

// Just an expression disguised as a Statement
struct ExpressionStatement: public Statement {
    ExpressionStatement(Locatable loc, std::unique_ptr<Expression> expr)
        : Statement(loc)
        ,_expr(std::move(expr)) {};
    std::unique_ptr<Expression> _expr;
    void print(std::ostream& stream);
};

// if (cond) then stat
// if (cond) then stat else stat
struct IfStatement: public Statement {
    IfStatement(Locatable loc, std::unique_ptr<Expression> condition, 
        std::unique_ptr<Statement> then_statement)
        : Statement(loc)
        ,_condition(std::move(condition))
        ,_then_statement(std::move(then_statement))
        , _else_statement(std::nullopt) {};
    IfStatement(Locatable loc, std::unique_ptr<Expression> condition, 
        std::unique_ptr<Statement> then_statement, std::unique_ptr<Statement> else_statement)
        : Statement(loc)
        ,_condition(std::move(condition))
        ,_then_statement(std::move(then_statement))
        , _else_statement(std::make_optional(std::move(else_statement))) {};
    std::unique_ptr<Expression> _condition;
    std::unique_ptr<Statement> _then_statement;
    std::optional<std::unique_ptr<Statement>> _else_statement;
    void print(std::ostream& stream);
};

// while (condition) statement
struct WhileStatement: public Statement {
    WhileStatement(Locatable loc, std::unique_ptr<Expression> condition, 
        std::unique_ptr<Statement> statement)
        : Statement(loc)
        ,_condition(std::move(condition))
        ,_statement(std::move(statement)) {};
    std::unique_ptr<Expression> _condition;
    std::unique_ptr<Statement> _statement;
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
    GotoStatement(Locatable loc, Symbol name, const char** ident)
        : JumpStatement(loc, name)
        ,_ident(ident) {};

    const char** _ident;

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

    ReturnStatement(Locatable loc, Symbol name, std::unique_ptr<Expression> expr)
        : JumpStatement(loc, name)
        , _expr(std::make_optional(std::move(expr))) {};

    std::optional<std::unique_ptr<Expression>> _expr;
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
#pragma once

#include <optional>
#include <string>

#include "expression.h"
#include "declarator.h"

struct Statement {};


struct LabeledStatement: public Statement {
    std::string name;
};

struct CompoundStatement: public Statement {
    // { block-item-list_opt }
    BlockItemList items;
};

struct BlockItemList {};

struct SingleBlockItemList: public BlockItemList {
    // block-item
    BlockItem item;
};

struct MultiBlockItemList: public BlockItemList {
    // block-item-list block-item
    BlockItem item;
    BlockItemList next;
};

struct BlockItem {};

struct DeclarationBlockItem: public BlockItem {
    // declaration
    Declarator dec;
};

struct StatementBlockItem: public BlockItem {
    // statement
    Statement stat;
};

struct NullStatement: public Statement {
    // ;
};

struct ExpressionStatement: public Statement {
    // expression;
    Expression expression;
};

struct SelectionStatement: public Statement {};

struct IfStatement: public SelectionStatement {
    // if (condition) statement
    Expression condition;
    Statement statement;
};

struct IfElseStatement: public SelectionStatement {
    // if (condition) then_statement else else_statement
    Expression condition;
    Statement then_statement;
    Statement else_statement;
};

struct WhileStatement: public Statement {
    // while (condition) statement
    Expression condition;
    Statement statement;
};

struct JumpStatement: public Statement {};

struct GotoStatement: public JumpStatement {
    // goto ident
    const char** ident;
};

struct ContinueStatement: public JumpStatement {};

struct BreakStatement: public JumpStatement {};

struct ReturnStatement: public JumpStatement {
    // return expr
    std::optional<Expression> expr;
};

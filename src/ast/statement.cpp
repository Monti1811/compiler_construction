#include "statement.h"

#include "indentation.h"

std::ostream& operator<<(std::ostream& stream, const StatementPtr& stat) {
    stat->print(stream);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const IndentManager& identmanager) {
    for (int i = 0; i < identmanager.currIndent; i++) {
        stream << '\t';
    }
    return stream;
}

void LabeledStatement::print(std::ostream& stream) {
    IndentManager& indent = IndentManager::getInstance();
    stream << *this->_name << ':';
    if (this->_inner.get()->kind == StatementKind::ST_LABELED) {
        stream << '\n' << this->_inner;
    } else {
        stream << '\n' << indent << this->_inner;
    }
}

void DeclarationStatement::print(std::ostream& stream) {
    stream << this->_declaration << ';';
}

void ExpressionStatement::print(std::ostream& stream) {
    stream << this->_expr << ';';
}

void BlockStatement::print(std::ostream& stream) {
    stream << "{";
    IndentManager& indent = IndentManager::getInstance();
    indent.increaseCurrIndentation(1);
    for (auto &item : this->_items) {
        if (item.get()->kind == StatementKind::ST_LABELED) {
            stream << '\n' << item;
        } else {
            stream << '\n' << indent << item;
        }
    }
    indent.decreaseCurrIndentation(1);
    stream << '\n' << indent << '}';
}

void BlockStatement::typecheck(ScopePtr& scope) {
    auto block_scope = std::make_shared<Scope>(scope);
    this->typecheckInner(block_scope);
}

void BlockStatement::typecheckInner(ScopePtr& inner_scope) {
    for (auto& item : this->_items) {
        item->typecheck(inner_scope);
    }
}

void EmptyStatement::print(std::ostream& stream) {
    stream << ';';
}

void IfStatement::print(std::ostream& stream) {
    stream << "if (" << this->_condition << ')';
    IndentManager& indent = IndentManager::getInstance();
    bool hasElseStatement = this->_else_statement.has_value();

    if (this->_then_statement->kind == StatementKind::ST_BLOCK) {
        stream << ' ' << this->_then_statement;
        if (hasElseStatement) {
            stream << " ";
        }
    } else {
        indent.increaseCurrIndentation(1);
        stream << "\n" << indent << this->_then_statement;
        indent.decreaseCurrIndentation(1);
        if (hasElseStatement) {
            stream << "\n" << indent;
        }
    }
        
    if (hasElseStatement) {
        auto& else_statement = this->_else_statement.value();
        auto else_type = else_statement->kind;
        
        if (else_type == StatementKind::ST_BLOCK || else_type == StatementKind::ST_IF) {
            stream << "else " << else_statement;
        } else {
            stream << "else";
            indent.increaseCurrIndentation(1);
            stream << "\n" << indent << else_statement;
            indent.decreaseCurrIndentation(1);
        }
    }
}

void WhileStatement::print(std::ostream& stream) {
    stream << "while (" << this->_condition << ')';
    IndentManager& indent = IndentManager::getInstance();
    if (this->_body->kind == StatementKind::ST_BLOCK) {
        stream << ' ' << this->_body;
    } else if (this->_body->kind == StatementKind::ST_LABELED) {
        indent.increaseCurrIndentation(1);
        stream << '\n' << this->_body;
        indent.decreaseCurrIndentation(1);
    } else {
        indent.increaseCurrIndentation(1);
        stream << '\n' << indent << this->_body;
        indent.decreaseCurrIndentation(1);
    }
    
}

void JumpStatement::print(std::ostream& stream) {
    stream << this->_jump_str << ';';
}

void ReturnStatement::print(std::ostream& stream) {
    stream << "return";
    if (this->_expr) {
        stream << " " << this->_expr.value();
    }
    stream << ';';
}

void GotoStatement::print(std::ostream& stream) {
    stream << "goto " << *this->_ident << ';';
}

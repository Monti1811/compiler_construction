#include "statement.h"

std::ostream& operator<<(std::ostream& stream, const StatementPtr& stat) {
    stat->print(stream);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const IdentManager& identmanager) {
    for (int i = 0; i < identmanager.currIdent; i++) {
        stream << '\t';
    }
    return stream;
}

void LabeledStatement::print(std::ostream& stream) {
    IdentManager& ident = IdentManager::getInstance();
    stream << *this->_name << ':';
    if (this->_inner.get()->kind == StatementKind::ST_LABELED) {
        stream << '\n' << this->_inner;
    } else {
        stream << '\n' << ident << this->_inner;
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
    IdentManager& ident = IdentManager::getInstance();
    ident.increaseCurrIdentation(1);
    for (auto &item : this->_items) {
        if (item.get()->kind == StatementKind::ST_LABELED) {
            stream << '\n' << item;
        } else {
            stream << '\n' << ident << item;
        }
    }
    ident.decreaseCurrIdentation(1);
    stream << '\n' << ident << '}';
}

void EmptyStatement::print(std::ostream& stream) {
    stream << ';';
}

void IfStatement::print(std::ostream& stream) {
    stream << "if (" << this->_condition << ')';
    IdentManager& ident = IdentManager::getInstance();
    bool hasElseStatement = this->_else_statement.has_value();

    if (this->_then_statement->kind == StatementKind::ST_BLOCK) {
        stream << ' ' << this->_then_statement;
        if (hasElseStatement) {
            stream << " ";
        }
    } else {
        ident.increaseCurrIdentation(1);
        stream << "\n" << ident << this->_then_statement;
        ident.decreaseCurrIdentation(1);
        if (hasElseStatement) {
            stream << "\n" << ident;
        }
    }
        
    if (hasElseStatement) {
        auto& else_statement = this->_else_statement.value();
        auto else_type = else_statement->kind;
        
        if (else_type == StatementKind::ST_BLOCK || else_type == StatementKind::ST_IF) {
            stream << "else " << else_statement;
        } else {
            stream << "else";
            ident.increaseCurrIdentation(1);
            stream << "\n" << ident << else_statement;
            ident.decreaseCurrIdentation(1);
        }
    }
}

void WhileStatement::print(std::ostream& stream) {
    stream << "while (" << this->_condition << ')';
    IdentManager& ident = IdentManager::getInstance();
    if (this->_statement->kind == StatementKind::ST_BLOCK) {
        stream << ' ' << this->_statement;
    } else if (this->_statement->kind == StatementKind::ST_LABELED) {
        ident.increaseCurrIdentation(1);
        stream << '\n' << this->_statement;
        ident.decreaseCurrIdentation(1);
    } else {
        ident.increaseCurrIdentation(1);
        stream << '\n' << ident << this->_statement;
        ident.decreaseCurrIdentation(1);
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

// Identation manager to keep track of idendation

int IdentManager::getCurrIdentation() {
    return this->currIdent;
}

void IdentManager::setCurrIdentation(int value) {
    this->currIdent = value;
}

void IdentManager::increaseCurrIdentation(int value) {
    this->currIdent += value;
}

void IdentManager::decreaseCurrIdentation(int value) {
    this->currIdent = std::max(this->currIdent-value,0);
}

void IdentManager::printCurrIdentation(std::ostream& stream) {
    for (int i = 0; i < this->currIdent; i++)
        stream << '\t';
}

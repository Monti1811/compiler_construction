#include "statement.h"

// TODO: Add a global way of checking the current identation

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
    stream << '\n' << *this->_name << ":\n" << ident << this->_inner;
}

void DeclarationStatement::print(std::ostream& stream) {
    stream << this->_declaration << ';';
}

void ExpressionStatement::print(std::ostream& stream) {
    stream << this->_expr << ';';
}

void BlockStatement::print(std::ostream& stream) {
    if (!this->_items.empty()) {
        stream << "{";
        IdentManager& ident = IdentManager::getInstance();
        ident.increaseCurrIdentation(1);
        for (auto &item : this->_items) {
            stream << '\n' << ident << item;
        }
        ident.decreaseCurrIdentation(1);
        stream << '\n' << ident << '}';
    } else {
        stream << "{}";
    }
}

void IfStatement::print(std::ostream& stream) {
    stream << "if (" << this->_condition << ") " << this->_then_statement;
    if (this->_else_statement) {
        stream << " else " << this->_else_statement.value();
    }
}

void WhileStatement::print(std::ostream& stream) {
    stream << "while (" << this->_condition << ") ";
    IdentManager& ident = IdentManager::getInstance();
    ident.increaseCurrIdentation(1);
    stream << this->_statement;
    ident.decreaseCurrIdentation(1);
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

#include "statement.h"

// TODO: Add a global way of checking the current identation

std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Statement>& stat) {
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
    if (this->_inner) {
        stream << this->_name << ':';
    } else {
        IdentManager& ident = IdentManager::getInstance();
        ident.increaseCurrIdentation(1);
        stream << this->_name << ":\n" << ident << this->_inner.value();
        ident.decreaseCurrIdentation(1);
    }
    
}

void DeclarationStatement::print(std::ostream& stream) {
    this->_declaration.get()->print(stream);
    return;
}

void ExpressionStatement::print(std::ostream& stream) {
    this->_expr.get()->print(stream);
    return;
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
        stream << "\n}";
    } else {
        stream << "{}";
    }
}

void IfStatement::print(std::ostream& stream) {
    stream << "if (" << this->_condition << ") " << this->_then_statement;
    if (this->_else_statement) {
        stream << "else " << this->_else_statement.value();
    }
}

void WhileStatement::print(std::ostream& stream) {
    stream << "while (" << this->_condition << ") " << this->_statement;
}

void JumpStatement::print(std::ostream& stream) {
    stream << this->_jump_str;
}

void ReturnStatement::print(std::ostream& stream) {
    stream << "return";
    if (this->_expr) {
        stream << " " << this->_expr.value();
    }
}

void GotoStatement::print(std::ostream& stream) {
    stream << "goto" << this->_ident;
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

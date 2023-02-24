#include "block.h"

#include "../indentation.h"

// BlockStatement

void BlockStatement::print(std::ostream& stream) {
    stream << "{";
    IndentManager& indent = IndentManager::getInstance();
    indent.increaseCurrIndentation(1);
    for (auto& item : this->_items) {
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

void BlockStatement::compile(CompileScopePtr compile_scope) {
    auto inner_compile_scope = std::make_shared<CompileScope>(compile_scope);

    // First compile all declarations, then compile other statements
    for (auto& item : this->_items) {
        if (item->kind == StatementKind::ST_DECLARATION) {
            item->compile(inner_compile_scope);
        }
    }
    for (auto& item : this->_items) {
        if (!(item->kind == StatementKind::ST_DECLARATION)) {
            item->compile(inner_compile_scope);
        }
    }
}

// EmptyStatement

void EmptyStatement::print(std::ostream& stream) {
    stream << ';';
}

void EmptyStatement::compile(CompileScopePtr) {}

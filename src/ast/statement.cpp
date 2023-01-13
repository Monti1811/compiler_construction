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

void LabeledStatement::typecheck(ScopePtr& scope) {
    this->_inner->typecheck(scope);
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

void DeclarationStatement::print(std::ostream& stream) {
    stream << this->_declaration << ';';
}

void DeclarationStatement::typecheck(ScopePtr& scope) {
    this->_declaration.typecheck(scope);
}

void ExpressionStatement::print(std::ostream& stream) {
    stream << this->_expr << ';';
}

void ExpressionStatement::typecheck(ScopePtr& scope) {
    this->_expr->typecheck(scope);
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

void IfStatement::typecheck(ScopePtr& scope) {
    auto condition_type = this->_condition->typecheckWrap(scope);
    if (!condition_type->isScalar()) {
        errorloc(this->_condition->loc, "Condition of an if statement must be scalar");
    }
    this->_then_statement->typecheck(scope);
    if (this->_else_statement.has_value()) {
        this->_else_statement->get()->typecheck(scope);
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

void WhileStatement::typecheck(ScopePtr& scope) {
    auto condition_type = this->_condition->typecheckWrap(scope);
    if (!condition_type->isScalar()) {
        errorloc(this->loc, "Condition of a while statement must be scalar");
    }
    scope->loop_counter++;
    this->_body->typecheck(scope);
    scope->loop_counter--;
}

void JumpStatement::print(std::ostream& stream) {
    stream << this->_jump_str << ';';
}

void GotoStatement::print(std::ostream& stream) {
    stream << "goto " << *this->_ident << ';';
}

void GotoStatement::typecheck(ScopePtr& scope) {
    if (this->_ident->length() == 0) {
        errorloc(this->loc, "Labels cannot be empty");
    }
    if (!scope->isLabelDefined(this->_ident)) {
        errorloc(this->loc, "Missing label");
    }
}

void ContinueStatement::typecheck(ScopePtr& scope) {
    if (scope->loop_counter == 0) {
        errorloc(this->loc, "Invalid 'continue' outside of a loop");
    }
}

void BreakStatement::typecheck(ScopePtr& scope) {
    if (scope->loop_counter == 0) {
        errorloc(this->loc, "Invalid 'break' outside of a loop");
    }
}

void ReturnStatement::print(std::ostream& stream) {
    stream << "return";
    if (this->_expr) {
        stream << " " << this->_expr.value();
    }
    stream << ';';
}

void ReturnStatement::typecheck(ScopePtr& scope) {
    auto return_type_opt = scope->function_return_type;
    if (!return_type_opt.has_value()) {
        errorloc(this->loc, "Return Statement in a non-function block");
    }

    auto return_type = return_type_opt.value();

    if (return_type->kind == TypeKind::TY_VOID) {
        if (_expr.has_value()) {
            errorloc(this->loc, "return statement must be empty if return type is void");
        }
        return;
    } 

    if (!_expr.has_value()) {
        errorloc(this->loc, "expected a return expression but got none");
    }

    auto expr_type = _expr.value()->typecheckWrap(scope);
    if (!expr_type->equals(return_type)) {
        errorloc(this->loc, "return type and type of return expr did not match");
    }
}

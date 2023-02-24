#include "primary.h"

const std::unordered_map<char, char> ESCAPE_CHARS = {
    {'\'', '\''},
    {'"', '"'},
    {'?', '?'},
    {'\\', '\\'},
    {'a', '\a'},
    {'b', '\b'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'v', '\v'}};

// IdentExpression

void IdentExpression::print(std::ostream& stream) {
    stream << (*this->_ident);
}

TypePtr IdentExpression::typecheck(ScopePtr& scope) {
    auto type = scope->getVarType(this->_ident);
    if (!type.has_value()) {
        errorloc(this->loc, "Variable ", *this->_ident, " is not defined");
    }
    this->type = type.value();
    return this->type;
}

bool IdentExpression::isLvalue(ScopePtr& scope) {
    // 6.5.1.0.2:
    // An identifier is a primary expression,
    // provided it has been declared as designating an object (in which case it is an lvalue)
    // or a function (in which case it is a function designator).
    return !scope->isFunctionDesignator(this->_ident);
}

llvm::Value* IdentExpression::compileLValue(CompileScopePtr compile_scope) {
    llvm::Value* saved_alloca = compile_scope->getAlloca(this->_ident).value();
    return saved_alloca;
}

llvm::Value* IdentExpression::compileRValue(CompileScopePtr compile_scope) {
    // identifier should always exist since we typechecked the program already
    llvm::Value* saved_alloca = this->compileLValue(compile_scope);
    llvm::Type* var_type = compile_scope->getType(this->_ident).value();

    // If the identifier denotes a function, return the function directly
    if (var_type->isFunctionTy()) {
        return saved_alloca;
    }

    return compile_scope->builder.CreateLoad(var_type, saved_alloca);
}

// IntConstantExpression

void IntConstantExpression::print(std::ostream& stream) {
    stream << this->_value;
}

TypePtr IntConstantExpression::typecheck(ScopePtr&) {
    this->type = INT_TYPE;
    return this->type;
}

llvm::Value* IntConstantExpression::compileRValue(CompileScopePtr compile_scope) {
    return compile_scope->builder.getInt32(this->_value);
}

// NullPtrExpression

void NullPtrExpression::print(std::ostream& stream) {
    stream << "0";
}

TypePtr NullPtrExpression::typecheck(ScopePtr&) {
    this->type = NULLPTR_TYPE;
    return this->type;
}

llvm::Value* NullPtrExpression::compileRValue(CompileScopePtr compile_scope) {
    // We assume that all null pointer constants that are of type int or char have already been cast
    auto type = compile_scope->builder.getPtrTy();
    return llvm::ConstantPointerNull::get(type);
}

// CharConstantExpression

void CharConstantExpression::print(std::ostream& stream) {
    stream << this->_value;
}

TypePtr CharConstantExpression::typecheck(ScopePtr&) {
    // 6.4.4.4.10:
    // An integer character constant has type int.
    this->type = INT_TYPE;
    return this->type;
}

char CharConstantExpression::getChar() {
    // Skip the first char since it is always a single quote
    auto result = this->_value[1];

    // If the first char is a backslash, we are dealing with an escape sequence
    if (result == '\\') {
        auto ch = this->_value[2];
        if (ESCAPE_CHARS.find(ch) == ESCAPE_CHARS.end()) {
            errorloc(this->loc, "Unknown escape code \\", ch);
        }
        result = ESCAPE_CHARS.at(ch);
    }

    return result;
}

llvm::Value* CharConstantExpression::compileRValue(CompileScopePtr compile_scope) {
    return compile_scope->builder.getInt32(this->getChar());
}

// StringLiteralExpression

void StringLiteralExpression::print(std::ostream& stream) {
    stream << this->_value;
}

TypePtr StringLiteralExpression::typecheck(ScopePtr&) {
    this->type = STRING_TYPE;
    return this->type;
}

bool StringLiteralExpression::isLvalue(ScopePtr&) {
    // 6.5.1.0.4:
    // A string literal is a primary expression. It is an lvalue with type as detailed in 6.4.5.
    return true;
}

std::string StringLiteralExpression::getString() {
    auto result = std::string("");

    // Loop through all chars, except for the first and last one (those are always double quotes)
    for (size_t i = 1; i < this->_value.length() - 1; i++) {
        auto ch = this->_value[i];
        if (ch != '\\') {
            result += ch;
            continue;
        }
        // Escape codes
        i++;
        ch = this->_value[i];

        if (ESCAPE_CHARS.find(ch) == ESCAPE_CHARS.end()) {
            errorloc(this->loc, "Unknown escape code \\", ch);
        }

        result += ESCAPE_CHARS.at(ch);
    }

    return result;
}

llvm::Value* StringLiteralExpression::compileRValue(CompileScopePtr compile_scope) {
    return compile_scope->builder.CreateGlobalStringPtr(this->getString());
}

std::optional<size_t> StringLiteralExpression::getStringLength(void) {
    return this->getString().length() + 1;
}

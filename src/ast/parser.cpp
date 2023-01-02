#include "parser.h"

Declaration Parser::parseDeclaration(DeclKind declKind) {
    TypeSpecifierPtr spec(nullptr);
    Locatable loc(getLoc());

    if (accept(TK_VOID)) {
        spec = std::make_unique<VoidSpecifier>(loc);
    } else if (accept(TK_CHAR)) {
        spec = std::make_unique<CharSpecifier>(loc);
    } else if (accept(TK_INT)) {
        spec = std::make_unique<IntSpecifier>(loc);
    } else if (accept(TK_STRUCT)) {
        std::optional<Symbol> tag = std::nullopt;
        if (check(TK_IDENTIFIER)) {
            tag = peekToken().Text;
            expect(TK_IDENTIFIER, "identifier");
        } else if (!check(TK_LBRACE)) {
            errorloc(getLoc(), "Expected struct declaration list but got `",
                     *peekToken().Text, "'");
        }

        auto structSpec = std::make_unique<StructSpecifier>(loc, tag);

        if (accept(TK_LBRACE)) {
            do {  // parse struct member declarations
                auto declaration = parseDeclaration(DeclKind::CONCRETE);
                structSpec->addComponent(std::move(declaration));
                expect(TK_SEMICOLON, ";");
            } while (!accept(TK_RBRACE));
        }
        spec = std::move(structSpec);

    } else {
        errorloc(loc, "Expected type specifier but got `", *peekToken().Text,
                 "'");
    }

    DeclaratorPtr decl(parseDeclarator(declKind));

    if (!decl->isAbstract() && declKind == DeclKind::ABSTRACT) {
        // At all places where non-abstractness is required, declarators are
        // also optional in the grammar, therefore it's okay to accept a simple
        // empty primitive declarator here anyway.

        errorloc(decl->loc, "This declarator must be abstract");
    }

    return Declaration(loc, std::move(spec), std::move(decl));
}

DeclaratorPtr Parser::parseNonFunDeclarator(DeclKind declKind) {
    switch (peekToken().Kind) {
        case TK_LPAREN: {
            if (checkLookAhead(TK_RPAREN)) {
                // type () not allowed
                errorloc(getLoc(), "nameless function");
            }
            if (checkLookAhead(TK_VOID) ||
                checkLookAhead(TK_CHAR) || checkLookAhead(TK_INT) ||
                checkLookAhead(TK_STRUCT)) {
                // this has to be an abstract function declarator, not a
                // parenthesized declarator, so we add an empty primitive
                // declarator
                return std::make_unique<PrimitiveDeclarator>(getLoc());
            }
            expect(TK_LPAREN, "(");
            auto res = parseDeclarator(declKind);
            expect(TK_RPAREN, ")");
            return res;
        }

        case TK_ASTERISK: {
            Locatable loc(getLoc());
            expect(TK_ASTERISK, "*");
            auto inner = parseDeclarator(declKind);
            return std::make_unique<PointerDeclarator>(loc, std::move(inner));
        }

        case TK_IDENTIFIER: {
            auto res = std::make_unique<PrimitiveDeclarator>(getLoc(), peekToken().Text);
            expect(TK_IDENTIFIER, "identifier");
            return res;
        }
        default:
            return std::make_unique<PrimitiveDeclarator>(getLoc());
    }
}

DeclaratorPtr Parser::parseDeclarator(DeclKind declKind) {
    DeclaratorPtr res = parseNonFunDeclarator(declKind);
    while (check(TK_LPAREN)) {
        if (res->isAbstract() && declKind == DeclKind::CONCRETE) {
            errorloc(getLoc(), "Functions must have a name");
        }

        auto funDecl = std::make_unique<FunctionDeclarator>(getLoc(), std::move(res));
        expect(TK_LPAREN, "(");

        if (accept(TK_RPAREN)) {
            res = std::move(funDecl);
            continue;
        }

        bool next_decl = true;
        while (next_decl && !check(TK_RPAREN)) {
            auto param = parseDeclaration(DeclKind::ANY);
            next_decl = accept(TK_COMMA);
            if (next_decl && check(TK_RPAREN)) {
                errorloc(getLoc(), "Expected another function argument");
            }
            funDecl->addParameter(std::move(param));
        }

        expect(TK_RPAREN, ")");
        res = std::move(funDecl);
    }
    return res;
}

ExpressionPtr Parser::parseExpression() {
    return parseAssignmentExpression();
}

ExpressionPtr Parser::parsePrimaryExpression() {
    Token token = peekToken();
    Symbol sym = token.Text;
    switch(token.Kind) {
        case TokenKind::TK_IDENTIFIER: 
        {
            expect(TokenKind::TK_IDENTIFIER, "identifier");
            auto expr = std::make_unique<IdentExpression>(token, sym);
            return expr;
        }
        case TokenKind::TK_ZERO_CONSTANT: 
        {
            expect(TokenKind::TK_ZERO_CONSTANT, "zero constant");
            auto expr = std::make_unique<NullPtrExpression>(token, sym);
            return expr;
        }
        case TokenKind::TK_DECIMAL_CONSTANT: 
        {
            expect(TokenKind::TK_DECIMAL_CONSTANT, "decimal constant");
            auto expr = std::make_unique<IntConstantExpression>(token, sym);
            return expr;
        }
        case TokenKind::TK_CHARACTER_CONSTANT:
        {
            expect(TokenKind::TK_CHARACTER_CONSTANT, "char constant");
            auto expr = std::make_unique<CharConstantExpression>(token, sym);
            return expr;
        }
        case TokenKind::TK_STRING_LITERAL: 
        {
            expect(TokenKind::TK_STRING_LITERAL, "string literal");
            auto expr = std::make_unique<StringLiteralExpression>(token, sym);
            return expr;
        }
        case TokenKind::TK_LPAREN: 
        {
            expect(TokenKind::TK_LPAREN, "(");
            auto expr = parseExpression();
            expect(TokenKind::TK_RPAREN, ")");
            return expr;
        }
        default: {
            errorloc(getLoc(), "wanted to parse PrimaryExpression but found no fitting token");
        }
    }
}

ExpressionPtr Parser::parsePostfixExpression(std::optional<ExpressionPtr> postfixExpression = std::nullopt) {
    // if there is no postfixExpression preceding the current token
    // parse current token, as this has to be a primaryExpression either way
    if (!postfixExpression) {
        postfixExpression = parsePrimaryExpression();
    }
    Token token = peekToken();
    switch (token.Kind) {
        // [expr]
        case TokenKind::TK_LBRACKET: 
        {
            expect(TK_LBRACKET, "[");
            auto index = parseExpression(); // accept index
            expect(TK_RBRACKET, "]");
            auto newPostfixExpr = std::make_unique<IndexExpression>(getLoc(), std::move(postfixExpression.value()), std::move(index));
            return parsePostfixExpression(std::move(newPostfixExpr));
        }
        // function call (a_1, a_2, ...)
        case TokenKind::TK_LPAREN:
        {
            expect(TK_LPAREN, "(");
            auto args = std::vector<ExpressionPtr>();
            bool next_argument = true;
            while (next_argument && !check(TK_RPAREN)) { // argumente lesen bis )
                auto arg = parseExpression(); // parse a_i
                next_argument = accept(TK_COMMA);
                if (next_argument && check(TK_RPAREN)) { 
                    errorloc(getLoc(), "Expected another function argument");
                }
                args.push_back(std::move(arg));
            }
            expect(TK_RPAREN, ")");
            // wrong error location for wrong_num_args test
            // change loc to name
            auto newPostfixExpr = std::make_unique<CallExpression>(token, std::move(postfixExpression.value()), std::move(args));
            return parsePostfixExpression(std::move(newPostfixExpr));
        }
        // .id
        case TokenKind::TK_DOT:
        {
            expect(TK_DOT, ".");
            Token token = peekToken();
            expect(TK_IDENTIFIER, "Identifier"); // expect id
            auto ident = std::make_unique<IdentExpression>(getLoc(), token.Text);
            auto newPostfixExpr = std::make_unique<DotExpression>(getLoc(), std::move(postfixExpression.value()), std::move(ident));
            return parsePostfixExpression(std::move(newPostfixExpr));
        }
        // -> id
        case TokenKind::TK_ARROW:
        {
            expect(TK_ARROW, "->");
            Token token = peekToken();
            expect(TK_IDENTIFIER, "Identifier"); // expect id
            auto ident = std::make_unique<IdentExpression>(getLoc(), token.Text);
            auto newPostfixExpr = std::make_unique<ArrowExpression>(getLoc(), std::move(postfixExpression.value()), std::move(ident));
            return parsePostfixExpression(std::move(newPostfixExpr));
        }
        // no postfix found
        // postFixExpressionFinished
        default:
        return std::move(postfixExpression.value());
    }
}

ExpressionPtr Parser::parseUnaryExpression() {
    Token token = peekToken();
    switch (token.Kind) {
        // &unaryexpr
        case TokenKind::TK_AND:
        {
            expect(TokenKind::TK_AND, "&");
            auto refExpr = std::make_unique<ReferenceExpression>(getLoc(), parseUnaryExpression());
            return refExpr;
        }
        // *unaryexpr
        case TokenKind::TK_ASTERISK:
        {
            expect(TokenKind::TK_ASTERISK, "*");
            auto pointerExpr = std::make_unique<PointerExpression>(getLoc(), parseUnaryExpression());
            return pointerExpr;
        }
        // +unaryexpr
        case TokenKind::TK_PLUS:
        {
            expect(TokenKind::TK_PLUS, "+");
            return parseUnaryExpression();
        }
        // -unaryexpr
        case TokenKind::TK_MINUS:
        {
            expect(TokenKind::TK_MINUS, "-");
            auto negExpr = std::make_unique<NegationExpression>(getLoc(), parseUnaryExpression());
            return negExpr;
        }
        // !unaryexpr
        case TokenKind::TK_BANG:
        {
            expect(TokenKind::TK_BANG, "!");
            auto logNegExpr = std::make_unique<LogicalNegationExpression>(getLoc(), parseUnaryExpression());
            return logNegExpr;
        }
        // sizeof unaryexpr or sizeof (type-name)
        case TokenKind::TK_SIZEOF:
        {
            expect(TokenKind::TK_SIZEOF, "sizeof"); // accept sizeof
            // check wether ( and type-name follows
            if (check(TokenKind::TK_LPAREN)) {
                // sizeof (type-name)
                if (checkLookAhead(TokenKind::TK_INT)) {
                    auto intSpec = std::make_unique<IntSpecifier>(peekToken());
                    expect(TokenKind::TK_LPAREN, "(");
                    expect(TokenKind::TK_INT, "int");
                    auto expr = std::make_unique<SizeofTypeExpression>(getLoc(), std::move(intSpec));
                    expect(TokenKind::TK_RPAREN, ")");
                    return expr;
                } else if (checkLookAhead(TokenKind::TK_VOID)) {
                    auto voidSpec = std::make_unique<VoidSpecifier>(peekToken());
                    expect(TokenKind::TK_LPAREN, "(");
                    expect(TokenKind::TK_VOID, "void");
                    auto expr = std::make_unique<SizeofTypeExpression>(getLoc(), std::move(voidSpec));
                    expect(TokenKind::TK_RPAREN, ")");
                    return expr;
                } else if (checkLookAhead(TokenKind::TK_CHAR)) {
                    auto charSpec = std::make_unique<CharSpecifier>(peekToken());
                    expect(TokenKind::TK_LPAREN, "(");
                    expect(TokenKind::TK_CHAR, "char");
                    auto expr = std::make_unique<SizeofTypeExpression>(getLoc(), std::move(charSpec));
                    expect(TokenKind::TK_RPAREN, ")");
                    return expr;
                }
            }
            auto expr = std::make_unique<SizeofExpression>(getLoc(), parseUnaryExpression());
            return expr;
        }
        // no unary-operator or sizeof found
        default: {
            return parsePostfixExpression();
        }
    }
}

constexpr int getPrecedenceLevel(TokenKind tk) {
    /*
        precedence table
        operator --- precedence level
        ||              0
        &&              1
        !=              2
        ==              2
        <               3
        +               4
        -               4
        *               5
    */
    switch (tk) {
        // ||
        case TK_PIPE_PIPE:
        {
            return 0;
        }
        // &&
        case TK_AND_AND:
        {
            return 1;
        }
        // !=
        case TK_NOT_EQUAL:
        {
            return 2;
        }
        // ==
        case TK_EQUAL_EQUAL:
        {
            return 2;
        }
        // <
        case TK_LESS:
        {
            return 3;
        }
        // +
        case TK_PLUS:
        {
            return 4;
        }
        // -
        case TK_MINUS:
        {
            return 4;
        }
        // *
        case TK_ASTERISK:
        {
            return 5;
        }
        default: {
            return -1;
        }
    }
}

ExpressionPtr Parser::parseBinaryExpression(int minPrec = 0, std::optional<ExpressionPtr> left = std::nullopt) {
    // compute unary expr
    // inbetween operators there has to be a unary expr
    if (!left) {
        left = parseUnaryExpression();
    }
    
    while (true) {
        Token token = peekToken();
        int precLevel = getPrecedenceLevel(token.Kind);

        if (minPrec > precLevel) {
            return std::move(left.value());
        }

        // since every binary Operator (that we handle) is left-associative
        precLevel++;

        // Accept binary operator.
        // We know this is a binary operator since the precLevel of every token that's not a binaryOp is < 0
        // and thus would already have returned the left part of the expression above.
        popToken();

        auto right = parseBinaryExpression(precLevel);
        switch (token.Kind) {
            // binaryOp = *
            case TK_ASTERISK: {
                auto multExpr = std::make_unique<MultiplyExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(multExpr));
            }
            // binaryOp = -
            case TK_MINUS: {
                auto subExpr = std::make_unique<SubstractExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(subExpr));
            }
            // binaryOp = +
            case TK_PLUS: {
                auto addExpr = std::make_unique<AddExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(addExpr));
            }
            // binaryOp = <
            case TK_LESS: {
                auto lessThanExpr = std::make_unique<LessThanExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(lessThanExpr));
            }
            // binaryOp !=
            case TK_NOT_EQUAL: {
                auto unequalExpr = std::make_unique<UnequalExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(unequalExpr));
            }
            // binaryOp ==
            case TK_EQUAL_EQUAL: {
                auto equalExpr = std::make_unique<EqualExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(equalExpr));
            }
            // binaryOp &&
            case TK_AND_AND: {
                auto logAndExpr = std::make_unique<AndExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(logAndExpr));
            }
            // binaryOp ||
            case TK_PIPE_PIPE: {
                auto logOrExpr = std::make_unique<OrExpression>(token, std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(logOrExpr));
            }
            default: 
            // no binary Op found
                return std::move(left.value());
        }
    }

}

ExpressionPtr Parser::parseConditionalExpression(std::optional<ExpressionPtr> left = std::nullopt) {
    // parse cond
    auto cond = (left.has_value()) ? parseBinaryExpression(0, std::move(left.value())) : parseBinaryExpression();
    // check for ?
    if (accept(TokenKind::TK_QUESTION_MARK)) {
        // accept ?
        auto operandOne = parseExpression();
        expect(TokenKind::TK_COLON, ":"); // expect :
        auto operandTwo = parseConditionalExpression();
        auto condExpr = std::make_unique<TernaryExpression>(getLoc(), std::move(cond), std::move(operandOne), std::move(operandTwo));
        return condExpr;
    }
    return cond;
}

ExpressionPtr Parser::parseAssignmentExpression() {
    ExpressionPtr unaryExpr = parseUnaryExpression();
    auto loc = getLoc();
    // check for =
    if (accept(TokenKind::TK_EQUAL)) {
        // accept = and start parsing AssignmentExpression
        ExpressionPtr right = parseAssignmentExpression();
        return std::make_unique<AssignExpression>(loc, std::move(unaryExpr), std::move(right));
    }
    // we know it's not an assignment and therefore we keep parsing a cond expression
    // but with the knowledge that we have already parsed a unary expression
    // this works since every binaryexpression starts with a unary expression 
    // and every conditionalexpression starts with a binary expression
    return parseConditionalExpression(std::move(unaryExpr));
}

BlockStatement Parser::parseBlockStatement() {
    auto loc = getLoc();

    expect(TK_LBRACE, "{");

    std::vector<StatementPtr> statements;
    while (!check(TK_RBRACE)) {
        statements.push_back(parseStatement());
    }

    expect(TK_RBRACE, "}");

    return BlockStatement(loc, std::move(statements));
}

StatementPtr Parser::parseStatement() {
    Token token = peekToken();
    switch (token.Kind) {
        case TK_IF: {
            expect(TK_IF, "if");
            expect(TK_LPAREN, "(");
            ExpressionPtr condition = parseExpression();
            expect(TK_RPAREN, ")");
            StatementPtr then_statement = parseNonDeclStatement();
            if (accept(TK_ELSE)) {
                StatementPtr else_statement = parseNonDeclStatement();
                return std::make_unique<IfStatement>(token, std::move(condition), std::move(then_statement), std::move(else_statement));
            } else {
                return std::make_unique<IfStatement>(token, std::move(condition), std::move(then_statement));
            }
        }

        case TK_LBRACE: {
            return std::make_unique<BlockStatement>(parseBlockStatement());
        }

        case TK_VOID:
        case TK_CHAR:
        case TK_INT:
        case TK_STRUCT: {
            auto declaration = parseDeclaration(DeclKind::CONCRETE);
            auto statement = std::make_unique<DeclarationStatement>(token, std::move(declaration));
            expect(TokenKind::TK_SEMICOLON, ";");
            return statement;
        }

        case TK_WHILE: {
            expect(TK_WHILE, "while");
            expect(TK_LPAREN, "(");
            ExpressionPtr condition = parseExpression();
            expect(TK_RPAREN, ")");
            StatementPtr stat = parseNonDeclStatement();
            return std::make_unique<WhileStatement>(token, std::move(condition), std::move(stat));
        }

        case TK_GOTO: {
            expect(TK_GOTO, "goto");
            Token curr = peekToken();
            expect(TK_IDENTIFIER, "identifier");
            expect(TokenKind::TK_SEMICOLON, ";");
            return std::make_unique<GotoStatement>(token, token.Text, curr.Text);
        }

        case TK_CONTINUE: {
            expect(TK_CONTINUE, "continue");
            auto statement = std::make_unique<ContinueStatement>(token, token.Text);
            expect(TokenKind::TK_SEMICOLON, ";");
            return statement;
        }
        
        case TK_BREAK: {
            expect(TK_BREAK, "break");
            auto statement = std::make_unique<BreakStatement>(token, token.Text);
            expect(TokenKind::TK_SEMICOLON, ";");
            return statement;
        }

        case TK_RETURN: {
            expect(TK_RETURN, "return");
            if (!check(TK_SEMICOLON)) {
                ExpressionPtr returnvalue = parseExpression();
                expect(TokenKind::TK_SEMICOLON, ";");
                return std::make_unique<ReturnStatement>(token, token.Text, std::move(returnvalue));
            } else {
                expect(TokenKind::TK_SEMICOLON, ";");
                return std::make_unique<ReturnStatement>(token, token.Text);
            }
        }

        // Empty statement
        case TK_SEMICOLON: {
            expect(TokenKind::TK_SEMICOLON, ";");
            return std::make_unique<EmptyStatement>(token);
        }

        case TK_IDENTIFIER: {
            if (checkLookAhead(TK_COLON)) {
                expect(TK_IDENTIFIER, "identifier");
                expect(TK_COLON, ":");

                auto new_label = this->_labels.insert(token.Text).second;
                if (!new_label) {
                    errorloc(token, "Duplicate label");
                }

                StatementPtr stat = parseNonDeclStatement();
                return std::make_unique<LabeledStatement>(token, token.Text, std::move(stat));
            }
            [[fallthrough]];
        }
        
        default: {
            auto statement = std::make_unique<ExpressionStatement>(token, parseExpression());
            expect(TokenKind::TK_SEMICOLON, ";");
            return statement;
        }
    }
}

StatementPtr Parser::parseNonDeclStatement() {
    auto statement = parseStatement();
    if (statement->kind == StatementKind::ST_DECLARATION) {
        errorloc(statement->loc, "Expected statement, got declaration");
    }
    return statement;
}

Program Parser::parseProgram() {
    auto program = Program();

    while (!check(TokenKind::TK_EOI)) {
        auto declaration = parseDeclaration(DeclKind::CONCRETE);

        switch (peekToken().Kind) {
            case TokenKind::TK_SEMICOLON: {
                expect(TK_SEMICOLON, ";");
                // abstract function
                if (declaration._declarator->kind == FUNCTION) {
                    auto function = FunctionDefinition(std::move(declaration), std::nullopt, this->_labels);
                    program.addFunctionDefinition(std::move(function));
                    break;
                }
                program.addDeclaration(std::move(declaration));
                break;
            }
            case TokenKind::TK_LBRACE: {
                this->_labels.clear();
                auto block = parseBlockStatement();
                auto function = FunctionDefinition(std::move(declaration), std::move(block), this->_labels);
                program.addFunctionDefinition(std::move(function));
                break;
            }
            default:
                errorloc(getLoc(), "Expected either semicolon or block");
        }
    }

    return program;
}

void Parser::popToken() {
    if (_currentToken.Kind == TokenKind::TK_EOI) {
        errorloc(getLoc(), "Parsing State cannot be advance - unexpected end of input");
    }
    _currentToken = _nextToken;
    if (_currentToken.Kind != TokenKind::TK_EOI) {
        _nextToken = _lexer.next();
    }
}

const Token& Parser::peekToken() {
    return _currentToken;
}

const Locatable& Parser::getLoc() {
    // TODO: test if this works
    return _currentToken;
}

void Parser::expect(TokenKind tk, const char* txt) {
    if (_currentToken.Kind == tk) {
        popToken();
    } else {
        errorloc(getLoc(), "TokenKind '" + std::string(txt) 
            + "' was expected, but it was '" + *_currentToken.Text 
            + "', next token is '" + *_nextToken.Text + "'");
    }
}

bool Parser::accept(TokenKind tk) {
    if (_currentToken.Kind == tk) {
        popToken();
        return true;
    }
    return false;
}

bool Parser::check(TokenKind tk) {
    return _currentToken.Kind == tk;
}

bool Parser::checkLookAhead(TokenKind tk) {
    return _nextToken.Kind == tk;
}


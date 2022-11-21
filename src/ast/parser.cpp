#include "../util/diagnostic.h"
#include "../lexer/token.h"

#include "declarator.h"
#include "parser.h"

Parser::Parser(Lexer& lexer, Token currentToken, Token nextToken) : _lexer(lexer), _currentToken(currentToken), _nextToken(nextToken) {};

Expression Parser::parseNext() {
    return parseExpression();
}

Declaration Parser::parseSpecDecl(DeclKind dKind) {
    std::unique_ptr<TypeSpecifier> spec(nullptr);
    Locatable loc(getLoc());

    if (accept(TK_VOID)) {
        spec = std::make_unique<VoidSpecifier>(loc);
    } else if (accept(TK_CHAR)) {
        spec = std::make_unique<CharSpecifier>(loc);
    } else if (accept(TK_INT)) {
        spec = std::make_unique<IntSpecifier>(loc);
    } else if (accept(TK_STRUCT)) {
        Symbol tag = nullptr;
        if (check(TK_IDENTIFIER)) {
            tag = peekToken().Text;
            popToken();
        } else if (!check(TK_LBRACE)) {
            errorloc(getLoc(), "Expected struct declaration list but got `",
                     *peekToken().Text, "'");
        }

        auto structSpec = std::make_unique<StructSpecifier>(loc, tag);

        if (accept(TK_LBRACE)) {
            do {  // parse struct member declarations
                auto specDecl = parseSpecDecl(DeclKind::CONCRETE);
                structSpec->addComponent(std::move(specDecl));
                expect(TK_SEMICOLON, ";");
            } while (!accept(TK_RBRACE));
        }
        spec = std::move(structSpec);

    } else {
        errorloc(loc, "Expected type specifier but got `", *peekToken().Text,
                 "'");
    }

    std::unique_ptr<Declarator> decl(parseDeclarator());

    if (!decl->isEmptyDeclarator()) {
        // At all places where non-abstractness is required, declarators are
        // also optional in the grammar, therefore it's okay to accept a simple
        // empty primitive declarator here anyway.

        // TODO check if decl is a valid declarator or abstract declarator and
        //      that it fits with what is required by dKind
    }

    return Declaration(std::move(spec), std::move(decl));
}

Declarator* Parser::parseNonFunDeclarator(void) {
    switch (peekToken().Kind) {
        case TK_LPAREN: {
            if (checkLookAhead(TK_RPAREN) || checkLookAhead(TK_VOID) ||
                checkLookAhead(TK_CHAR) || checkLookAhead(TK_INT) ||
                checkLookAhead(TK_STRUCT)) {
                // this has to be an abstract function declarator, not a
                // parenthesized declarator, so we add an empty primitive
                // declarator
                return new PrimitiveDeclarator(getLoc(), nullptr);
            }
            popToken();
            auto* res = parseDeclarator();
            expect(TK_RPAREN, ")");
            return res;
        }

        case TK_ASTERISK: {
            Locatable loc(getLoc());
            popToken();
            auto* inner = parseDeclarator();
            return new PointerDeclarator(loc, inner);
        }

        case TK_IDENTIFIER: {
            auto* res = new PrimitiveDeclarator(getLoc(), peekToken().Text);
            popToken();
            return res;
        }
        default:
            return new PrimitiveDeclarator(getLoc(), nullptr);
    }
}

Declarator* Parser::parseDeclarator(void) {
    Declarator* res = parseNonFunDeclarator();
    while (check(TK_LPAREN)) {
        auto* funDecl = new FunctionDeclarator(getLoc(), res);
        res = funDecl;
        popToken();

        if (accept(TK_RPAREN)) {
            continue;
        }

        do {
            auto param = parseSpecDecl(DeclKind::ANY);
            funDecl->addParameter(std::move(param));
        } while (accept(TK_COMMA));

        expect(TK_RPAREN, ")");
    }
    return res;
}

Expression Parser::parseExpression() {
    return parseAssignmentExpression();
}

Expression Parser::parsePrimaryExpression() {
    Token token = peekToken();
    Symbol sym = token.Text;
    switch(token.Kind) {
        case TokenKind::TK_IDENTIFIER: 
        {
            IdentExpression expr(getLoc(), sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_ZERO_CONSTANT: 
        {
            IntConstantExpression expr = IntConstantExpression(getLoc(), sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_DECIMAL_CONSTANT: 
        {
            IntConstantExpression expr(getLoc(), sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_CHARACTER_CONSTANT:
        {
            CharConstantExpression expr(getLoc(), sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_STRING_LITERAL: 
        {
            StringLiteralExpression expr(getLoc(), sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_LPAREN: 
        {
            popToken();
            Expression expr = parseExpression();
            expect(TokenKind::TK_RPAREN, ")");
            return expr;
        }
        default: {
            errorloc(getLoc(), "wanted to parse PrimaryExpression but found no fitting token");
        }
    }
}

Expression Parser::parsePostfixExpression(std::optional<Expression> postfixExpression = std::nullopt) {
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
            popToken(); // accept [
            Expression index = parseExpression(); // accept index
            expect(TK_RBRACKET, "]"); // expect ]
            IndexExpression newPostfixExpression(getLoc(), std::make_unique<Expression>(postfixExpression.value()), std::make_unique<Expression>(index));
            return parsePostfixExpression(newPostfixExpression);
        }
        // function call (a_1, a_2, ...)
        case TokenKind::TK_LPAREN:
        {
            popToken(); // accept (
            auto args = std::vector<std::unique_ptr<Expression>>();
            while (peekToken().Kind != TokenKind::TK_RPAREN) { // argumente lesen bis )
                Expression arg = parseExpression(); // parse a_i
                accept(TK_COMMA); // accept ,
                args.push_back(std::make_unique<Expression>(arg));
            }
            popToken(); // accept )
            CallExpression newPostfixExpression(getLoc(), std::make_unique<Expression>(postfixExpression.value()), args);
            return parsePostfixExpression(newPostfixExpression);
        }
        // .id
        case TokenKind::TK_DOT:
        {
            popToken(); // accept .
            expect(TK_IDENTIFIER, "Identifier"); // expect id
            IdentExpression ident(getLoc(), token.Text);
            DotExpression newPostfixExpression(getLoc(), std::make_unique<Expression>(postfixExpression.value()), std::make_unique<IdentExpression>(ident));
            return parsePostfixExpression(newPostfixExpression);
        }
        // -> id
        case TokenKind::TK_ARROW:
        {
            popToken(); // accept .
            expect(TK_IDENTIFIER, "Identifier"); // expect id
            IdentExpression ident(getLoc(), token.Text);
            ArrowExpression newPostfixExpression(getLoc(), std::make_unique<Expression>(postfixExpression.value()), std::make_unique<IdentExpression>(ident));
            return parsePostfixExpression(newPostfixExpression);
        }
        // no postfix found
        // postFixExpressionFinished
        default:
        return postfixExpression.value();
    }
}

Expression Parser::parseUnaryExpression() {
    Token token = peekToken();
    switch (token.Kind) {
        // &unaryexpr
        case TokenKind::TK_AND:
        {
            popToken();
            ReferenceExpression refExpr(getLoc(), std::make_unique<Expression>(parseUnaryExpression()));
            return refExpr;
        }
        // *unaryexpr
        case TokenKind::TK_ASTERISK:
        {
            popToken();
            PointerExpression pointerExpr(getLoc(), std::make_unique<Expression>(parseUnaryExpression()));
            return pointerExpr;
        }
        // -unaryexpr
        case TokenKind::TK_MINUS:
        {
            popToken();
            NegationExpression negExpr(getLoc(), std::make_unique<Expression>(parseUnaryExpression()));
            return negExpr;
        }
        // !unaryexpr
        case TokenKind::TK_BANG:
        {
            popToken();
            LogicalNegationExpression logNegExpr(getLoc(), std::make_unique<Expression>(parseUnaryExpression()));
            return logNegExpr;
        }
        // sizeof unaryexpr or sizeof (type-name)
        case TokenKind::TK_SIZEOF:
        {
            popToken(); // accept sizeof
            // check wether ( and type-name follows
            if (check(TokenKind::TK_LPAREN))
            // sizeof (type-name)
            {
                if (checkLookAhead(TokenKind::TK_INT)) {
                    popToken(); // accept (
                    IntSpecifier intSpec(peekToken());
                    SizeofTypeExpression expr(getLoc(), intSpec);
                    expect(TokenKind::TK_RPAREN, ")"); // expect )
                    return expr;
                } else if (checkLookAhead(TokenKind::TK_VOID)) {
                    popToken(); // accept (
                    VoidSpecifier voidSpec(peekToken());
                    SizeofTypeExpression expr(getLoc(), voidSpec);
                    expect(TokenKind::TK_RPAREN, ")");
                    return expr;
                } else if (checkLookAhead(TokenKind::TK_CHAR)) {
                    popToken(); // accept (
                    CharSpecifier charSpec(peekToken());
                    SizeofTypeExpression expr(getLoc(), charSpec);
                    expect(TokenKind::TK_RPAREN, ")");
                    return expr;
                }
            }
            SizeofExpression expr(getLoc(), std::make_unique<Expression>(parseUnaryExpression()));
            return expr;
        }
        // no unary-operator or sizeof found
        default: {
            return parsePostfixExpression();
        }
    }
}

const int getPrecedenceLevel(TokenKind tk) {
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

Expression Parser::parseBinaryExpression(int minPrec = 0, std::optional<Expression> left = std::nullopt) {
    // compute unary expr
    // inbetween operators there has to be a unary expr
    if (!left) {
        left = parseUnaryExpression();
    }
    
    while (true) {
        Token token = peekToken();
        int precLevel = getPrecedenceLevel(token.Kind);

        if (minPrec > precLevel) {
            return left.value();
        }

        // since every binary Operator (that we handle) is left-associative
        precLevel++;

        popToken(); // accept binaryOp
        Expression right = parseBinaryExpression(precLevel);
        switch (token.Kind) {
            // binaryOp = *
            case TK_ASTERISK: {
                MultiplyExpression multExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, multExpr);
            }
            // binaryOp = -
            case TK_MINUS: {
                SubstractExpression subExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, subExpr);
            }
            // binaryOp = +
            case TK_PLUS: {
                AddExpression addExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, addExpr);
            }
            // binaryOp = <
            case TK_LESS: {
                LessThanExpression lessThanExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, lessThanExpr);
            }
            // binaryOp !=
            case TK_NOT_EQUAL: {
                UnequalExpression unequalExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, unequalExpr);
            }
            // binaryOp ==
            case TK_EQUAL_EQUAL: {
                EqualExpression equalExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, equalExpr);
            }
            // binaryOp &&
            case TK_AND_AND: {
                AndExpression logAndExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, logAndExpr);
            }
            // binaryOp ||
            case TK_PIPE_PIPE: {
                OrExpression logOrExpr(getLoc(), std::make_unique<Expression>(left.value()), std::make_unique<Expression>(right));
                return parseBinaryExpression(0, logOrExpr);
            }
            default: 
            // no binary Op found
                return left.value();
        }
    }

}

Expression Parser::parseConditionalExpression(std::optional<Expression> left = std::nullopt) {
    // parse cond
    Expression cond = (left.has_value()) ? parseBinaryExpression(0, left.value()) : parseBinaryExpression();
    // check for ?
    if (accept(TokenKind::TK_QUESTION_MARK)) {
        // accept ?
        Expression operandOne = parseExpression();
        expect(TokenKind::TK_COLON, ":"); // expect :
        Expression operandTwo = parseConditionalExpression();
        TernaryExpression condExpr(getLoc(), std::make_unique<Expression>(cond), std::make_unique<Expression>(operandOne), std::make_unique<Expression>(operandTwo));
        return condExpr;
    }
    return cond;
}

Expression Parser::parseAssignmentExpression() {
    Expression unaryExpr = parseUnaryExpression();
    // check for =
    if (accept(TokenKind::TK_EQUAL)) {
        // accept = and start parsing AssignmentExpression
        Expression right = parseAssignmentExpression();
        return AssignExpression(getLoc(), std::make_unique<Expression>(unaryExpr), std::make_unique<Expression>(right));
    }
    // we know it's not an assignment and therefore we keep parsing a cond expression
    // but with the knowledge that we have already parsed a unary expression
    // this works since every binaryexpression starts with a unary expression 
    // and every conditionalexpression starts with a binary expression
    return parseConditionalExpression(unaryExpr);
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
        errorloc(getLoc(), "TokenKind " + std::string(txt) + " was expected, but something else occured");
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

#include "../util/diagnostic.h"
#include "../lexer/token.h"

#include "declarator.h"
#include "parser.h"

Parser::Parser(Lexer& lexer, Token currentToken, Token nextToken) : _lexer(lexer), _currentToken(currentToken), _nextToken(nextToken) {};

std::unique_ptr<Expression> Parser::parseNext() {
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

std::unique_ptr<Expression> Parser::parseExpression() {
    return std::move(parseAssignmentExpression());
}

std::unique_ptr<Expression> Parser::parsePrimaryExpression() {
    Token token = peekToken();
    Symbol sym = token.Text;
    switch(token.Kind) {
        case TokenKind::TK_IDENTIFIER: 
        {
            auto expr = std::make_unique<IdentExpression>(getLoc(), sym);
            popToken();
            return std::move(expr);
        }
        case TokenKind::TK_ZERO_CONSTANT: 
        {
            auto expr = std::make_unique<IntConstantExpression>(getLoc(), sym);
            popToken();
            return std::move(expr);
        }
        case TokenKind::TK_DECIMAL_CONSTANT: 
        {
            auto expr = std::make_unique<IntConstantExpression>(getLoc(), sym);
            popToken();
            return std::move(expr);
        }
        case TokenKind::TK_CHARACTER_CONSTANT:
        {
            auto expr = std::make_unique<CharConstantExpression>(getLoc(), sym);
            popToken();
            return std::move(expr);
        }
        case TokenKind::TK_STRING_LITERAL: 
        {
            auto expr = std::make_unique<StringLiteralExpression>(getLoc(), sym);
            popToken();
            return std::move(expr);
        }
        case TokenKind::TK_LPAREN: 
        {
            popToken();
            auto expr = parseExpression();
            expect(TokenKind::TK_RPAREN, ")");
            return std::move(expr);
        }
        default: {
            errorloc(getLoc(), "wanted to parse PrimaryExpression but found no fitting token");
        }
    }
}

std::unique_ptr<Expression> Parser::parsePostfixExpression(std::optional<std::unique_ptr<Expression>> postfixExpression = std::nullopt) {
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
            auto index = parseExpression(); // accept index
            expect(TK_RBRACKET, "]"); // expect ]
            auto newPostfixExpr = std::make_unique<IndexExpression>(getLoc(), std::move(postfixExpression.value()), std::move(index));
            return parsePostfixExpression(std::move(newPostfixExpr));
        }
        // function call (a_1, a_2, ...)
        case TokenKind::TK_LPAREN:
        {
            popToken(); // accept (
            auto args = std::vector<std::unique_ptr<Expression>>();
            while (peekToken().Kind != TokenKind::TK_RPAREN) { // argumente lesen bis )
                auto arg = parseExpression(); // parse a_i
                accept(TK_COMMA); // accept ,
                args.push_back(std::move(arg));
            }
            popToken(); // accept )
            auto newPostfixExpr = std::make_unique<CallExpression>(getLoc(), std::move(postfixExpression.value()), std::move(args));
            return parsePostfixExpression(std::move(newPostfixExpr));
        }
        // .id
        case TokenKind::TK_DOT:
        {
            popToken(); // accept .
            Token token = peekToken();
            expect(TK_IDENTIFIER, "Identifier"); // expect id
            auto ident = std::make_unique<IdentExpression>(getLoc(), token.Text);
            auto newPostfixExpr = std::make_unique<DotExpression>(getLoc(), std::move(postfixExpression.value()), std::move(ident));
            return parsePostfixExpression(std::move(newPostfixExpr));
        }
        // -> id
        case TokenKind::TK_ARROW:
        {
            popToken(); // accept .
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

std::unique_ptr<Expression> Parser::parseUnaryExpression() {
    Token token = peekToken();
    switch (token.Kind) {
        // &unaryexpr
        case TokenKind::TK_AND:
        {
            popToken();
            auto refExpr = std::make_unique<ReferenceExpression>(getLoc(), parseUnaryExpression());
            return refExpr;
        }
        // *unaryexpr
        case TokenKind::TK_ASTERISK:
        {
            popToken();
            auto pointerExpr = std::make_unique<PointerExpression>(getLoc(), parseUnaryExpression());
            return pointerExpr;
        }
        // -unaryexpr
        case TokenKind::TK_MINUS:
        {
            popToken();
            auto negExpr = std::make_unique<NegationExpression>(getLoc(), parseUnaryExpression());
            return negExpr;
        }
        // !unaryexpr
        case TokenKind::TK_BANG:
        {
            popToken();
            auto logNegExpr = std::make_unique<LogicalNegationExpression>(getLoc(), parseUnaryExpression());
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
                popToken(); // accept (

                if (check(TokenKind::TK_INT)) {
                    auto intSpec = std::make_unique<IntSpecifier>(peekToken());
                    popToken();
                    auto expr = std::make_unique<SizeofTypeExpression>(getLoc(), std::move(intSpec));
                    expect(TokenKind::TK_RPAREN, ")"); // expect )
                    return expr;
                } else if (check(TokenKind::TK_VOID)) {
                    auto voidSpec = std::make_unique<VoidSpecifier>(peekToken());
                    popToken();
                    auto expr = std::make_unique<SizeofTypeExpression>(getLoc(), std::move(voidSpec));
                    expect(TokenKind::TK_RPAREN, ")");
                    return expr;
                } else if (check(TokenKind::TK_CHAR)) {
                    auto charSpec = std::make_unique<CharSpecifier>(peekToken());
                    popToken();
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

std::unique_ptr<Expression> Parser::parseBinaryExpression(int minPrec = 0, std::optional<std::unique_ptr<Expression>> left = std::nullopt) {
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

        popToken(); // accept binaryOp
        auto right = parseBinaryExpression(precLevel);
        switch (token.Kind) {
            // binaryOp = *
            case TK_ASTERISK: {
                auto multExpr = std::make_unique<MultiplyExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(multExpr));
            }
            // binaryOp = -
            case TK_MINUS: {
                auto subExpr = std::make_unique<SubstractExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(subExpr));
            }
            // binaryOp = +
            case TK_PLUS: {
                auto addExpr = std::make_unique<AddExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(addExpr));
            }
            // binaryOp = <
            case TK_LESS: {
                auto lessThanExpr = std::make_unique<LessThanExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(lessThanExpr));
            }
            // binaryOp !=
            case TK_NOT_EQUAL: {
                auto unequalExpr = std::make_unique<UnequalExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(unequalExpr));
            }
            // binaryOp ==
            case TK_EQUAL_EQUAL: {
                auto equalExpr = std::make_unique<EqualExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(equalExpr));
            }
            // binaryOp &&
            case TK_AND_AND: {
                auto logAndExpr = std::make_unique<AndExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(logAndExpr));
            }
            // binaryOp ||
            case TK_PIPE_PIPE: {
                auto logOrExpr = std::make_unique<OrExpression>(getLoc(), std::move(left.value()), std::move(right));
                return parseBinaryExpression(minPrec, std::move(logOrExpr));
            }
            default: 
            // no binary Op found
                return std::move(left.value());
        }
    }

}

std::unique_ptr<Expression> Parser::parseConditionalExpression(std::optional<std::unique_ptr<Expression>> left = std::nullopt) {
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

std::unique_ptr<Expression> Parser::parseAssignmentExpression() {
    std::unique_ptr<Expression> unaryExpr = parseUnaryExpression();
    // check for =
    if (accept(TokenKind::TK_EQUAL)) {
        // accept = and start parsing AssignmentExpression
        std::unique_ptr<Expression> right = parseAssignmentExpression();
        return std::make_unique<AssignExpression>(getLoc(), std::move(unaryExpr), std::move(right));
    }
    // we know it's not an assignment and therefore we keep parsing a cond expression
    // but with the knowledge that we have already parsed a unary expression
    // this works since every binaryexpression starts with a unary expression 
    // and every conditionalexpression starts with a binary expression
    return parseConditionalExpression(std::move(unaryExpr));
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


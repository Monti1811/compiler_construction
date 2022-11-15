#include "parser.h"
#include "declarator.h"
#include "../util/diagnostic.h"
#include "../lexer/token.h"
#include "specifier.h"
#include <optional>

SpecDecl* Parser::parseSpecDecl(DeclKind dKind) {
    Specifier* spec = nullptr;
    Locatable loc(getLoc());
    if (accept(TK_VOID)) {
        spec = new VoidSpecifier(loc);
    } else if (accept(TK_CHAR)) {
        spec = new CharSpecifier(loc);
    } else if (accept(TK_INT)) {
        spec = new IntSpecifier(loc);
    } else if (accept(TK_STRUCT)) {
        Symbol tag = nullptr;
        if (check(TK_IDENTIFIER)) {
            tag = peekToken().Text;
            popToken();
        } else if (!check(TK_LBRACE)) {
            errorloc(getLoc(), "Expected struct declaration list but got `",
                     *peekToken().Text, "'");
        }
        auto* structSpec = new StructSpecifier(loc, tag);

        if (accept(TK_LBRACE)) {
            do {  // parse struct member declarations
                auto* specDecl = parseSpecDecl(DeclKind::CONCRETE);
                structSpec->addComponent(specDecl);
                expect(TK_SEMICOLON, ";");
            } while (!accept(TK_RBRACE));
        }
        spec = structSpec;

    } else {
        errorloc(loc, "Expected type specifier but got `", *peekToken().Text,
                 "'");
    }

    Declarator* decl = parseDeclarator();

    if (!decl->isEmptyDeclarator()) {
        // At all places where non-abstractness is required, declarators are
        // also optional in the grammar, therefore it's okay to accept a simple
        // empty primitive declarator here anyway.

        // TODO check if decl is a valid declarator or abstract declarator and
        //      that it fits with what is required by dKind
    }

    return new SpecDecl(spec, decl);
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
            auto* param = parseSpecDecl(DeclKind::ANY);
            funDecl->addParameter(param);
        } while (accept(TK_COMMA));

        expect(TK_RPAREN, ")");
    }
    return res;
}

/*
precedence table
operator --- precedence level
||              0
&&              1
==              2
<               3
+               4
-               4
*               5
*/

Expression Parser::parseExpression() {
}

PrimaryExpression parsePrimaryExpression() {
    Token token = peekToken();
    Symbol sym = token.Text;
    switch(token.Kind) {
        case TokenKind::TK_IDENTIFIER: 
        {
            IdentExpression expr(sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_ZERO_CONSTANT: 
        {
            Expression expr = ZeroConstantExpression();
            popToken();
            return expr;
        }
        case TokenKind::TK_DECIMAL_CONSTANT: 
        {
            IntConstantExpression expr(sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_CHARACTER_CONSTANT:
        {
            CharConstantExpression expr(sym);
            popToken();
            return expr;
        }
        case TokenKind::TK_STRING_LITERAL: 
        {
            StringLiteralExpression expr(sym);
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
    }
}

Expression parsePostfixExpression(std::optional<PostfixExpression> postfixExpression = std::nullopt) {
    // if there is no postfixExpression preceding the current token
    // parse current token, as this has to be a primaryExpression either way
    if (!postfixExpression) {
        postfixExpression = BasePostfixExpression(parsePrimaryExpression());
    }
    Token token = peekToken();
    switch (token.Kind) {
        // [expr]
        case TokenKind::TK_LBRACKET: 
        {
            popToken(); // accept [
            Expression index = parseExpression(); // accept index
            expectToken(TK_RBRACKET); // expect ]
            IndexExpression newPostfixExpression(postfixExpression.value(), index);
            return parsePostfixExpression(newPostfixExpression);
        }
        // function call (a_1, a_2, ...)
        case TokenKind::TK_LPAREN:
        {
            popToken(); // accept (
            std::vector<Expression> args = std::vector<Expression>();
            while (peekToken().Kind != TokenKind::TK_RPAREN) { // argumente lesen bis )
                Expression arg = parseExpression(); // parse a_i
                accept(TK_COMMA); // accept ,
                args.push_back(arg);
            }
            popToken(); // accept )
            CallExpression newPostfixExpression(postfixExpression.value(), args);
            return parsePostfixExpression(newPostfixExpression);
        }
        // .id
        case TokenKind::TK_DOT:
        {
            popToken(); // accept .
            expect(TK_IDENTIFIER); // expect id
            IdentExpression ident(token.Text);
            DotExpression newPostfixExpression(postfixExpression.value(), ident);
            return parsePostfixExpression(newPostfixExpression);
        }
        // -> id
        case TokenKind::TK_ARROW:
        {
            popToken(); // accept .
            expect(TK_IDENTIFIER); // expect id
            IdentExpression ident(token.Text);
            ArrowExpression newPostfixExpression(postfixExpression.value(), ident);
            return parsePostfixExpression(newPostfixExpression);
        }
        // no postfix found
        default:
        return postfixExpression.value();
    }
}




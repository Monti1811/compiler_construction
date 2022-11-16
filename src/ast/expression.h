#include <vector>
#include <string.h>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"
#include "../lexer/token.h"

struct Expression: public Locatable {
    public:
    Expression() {};
    // TODO: Implement Locatable
};

struct PrimaryExpression: public Expression {};

struct IdentExpression: public PrimaryExpression {
    public:
    IdentExpression(Symbol id) : ident(*id) {};
    private:
    std::string ident;
};

struct ConstantExpression: public PrimaryExpression {
};

struct ZeroConstantExpression: public ConstantExpression {
    public:
    ZeroConstantExpression() {};
    private:
    int value = 0;
};

struct IntConstantExpression: public ConstantExpression {
    public:
    IntConstantExpression(Symbol sym) : value( std::stoi(*sym)) {};
    private:
    int value;
};

struct CharConstantExpression: public ConstantExpression {
    public:
    CharConstantExpression(Symbol sym) : value( std::stoi(*sym)) {};
    private:
    char value;
};

struct StringLiteralExpression: public PrimaryExpression {
    public:
    StringLiteralExpression(Symbol sym) : value( (*sym).c_str() ) {};
    private:
    const char* value;
};

struct PostfixExpression: public PrimaryExpression {};

struct BasePostfixExpression: public PostfixExpression {
    // TODO: this is not necessary
    public:
    BasePostfixExpression(PrimaryExpression inner) : _inner(inner) {};
    private:
    PrimaryExpression _inner;
};

struct IndexExpression: public PostfixExpression {
    // expression[index]
    public:
    IndexExpression(PostfixExpression expression, Expression index) : _expression(expression), _index(index) {};
    private:
    PostfixExpression _expression;
    Expression _index;
};

struct CallExpression: public PostfixExpression {
    // expression(args)
    public:
    CallExpression(PostfixExpression expression, std::vector<Expression> arguments) : _expression(expression), _arguments(arguments) {};
    private:
    PostfixExpression _expression;
    std::vector<Expression> _arguments;
};

struct DotExpression: public PostfixExpression {
    // expression.ident
    public:
    DotExpression(PostfixExpression expression, IdentExpression ident) : _expression(expression), _ident(ident) {};
    private:
    PostfixExpression _expression;
    IdentExpression _ident;
};

struct ArrowExpression: public PostfixExpression {
    // expression->ident
    public:
    ArrowExpression(PostfixExpression expression, IdentExpression ident) : _expression(expression), _ident(ident) {};
    private:
    PostfixExpression _expression;
    IdentExpression _ident;
};

struct UnaryExpression: public PostfixExpression {};

struct BaseUnaryExpression: public UnaryExpression {
    // TODO: this is not necessary
    public:
    BaseUnaryExpression(PostfixExpression inner) : _inner(inner) {};
    private:
    PostfixExpression _inner;
};

struct SizeofExpression: public UnaryExpression {
    // sizeof inner
    public:
    SizeofExpression(UnaryExpression inner) : _inner(inner) {};
    private:
    UnaryExpression _inner;
};

struct SizeofTypeNameExpression: public UnaryExpression {
    // sizeof inner
    public:
    SizeofTypeNameExpression(TokenKind typeName) : _typeName(typeName) {};
    private:
    TokenKind _typeName;
};

struct ReferenceExpression: public UnaryExpression {
    // &inner
    public:
    ReferenceExpression(UnaryExpression inner) : _inner(inner) {};
    private:
    UnaryExpression _inner;
};

struct PointerExpression: public UnaryExpression {
    // *inner
    public:
    PointerExpression(UnaryExpression inner) : _inner(inner) {};
    private:
    UnaryExpression _inner;
};

struct NegationExpression: public UnaryExpression {
    // -inner
    public:
    NegationExpression(UnaryExpression inner) : _inner(inner) {};
    private:
    UnaryExpression _inner;
};

struct LogicalNegationExpression: public UnaryExpression {
    // !inner
    public:
    LogicalNegationExpression(UnaryExpression inner) : _inner(inner) {};
    private:
    UnaryExpression _inner;
};

struct BinaryExpression: public Expression {};

struct BaseBinaryExpression: public BinaryExpression {
    // TODO: This is not necessary
    public:
    BaseBinaryExpression(UnaryExpression inner) : _inner(inner) {};
    private:
    UnaryExpression _inner;
};

struct MultiplyExpression: public BinaryExpression {
    // left * right
    public:
    MultiplyExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct AdditiveExpression: public BinaryExpression {};

struct BaseAdditiveExpression: public AdditiveExpression {
    // TODO: This is not necessary
    BinaryExpression inner;
};

struct AddExpression: public AdditiveExpression {
    // left + right
    public:
    AddExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct SubstractExpression: public AdditiveExpression {
    // left - right
    public:
    SubstractExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct RelationalExpression: public BinaryExpression {};

struct BaseRelationalExpression: public RelationalExpression {
    // TODO
    BinaryExpression inner;
};

struct LessThanExpression: public RelationalExpression {
    // left < right
    public:
    LessThanExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct EqualityExpression: public BinaryExpression {};

struct BaseEqualityExpression: public EqualityExpression {
    // TODO
    RelationalExpression inner;
};

struct EqualExpression: public EqualityExpression {
    // left == right
    public:
    EqualExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct UnequalExpression: public EqualityExpression {
    // left != right
    public:
    UnequalExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct LogicalAndExpression: public BinaryExpression {};

struct BaseLogicalAndExpression: public LogicalAndExpression {
    // TODO
    EqualityExpression inner;
};

struct BinaryLogicalAndExpression: public LogicalAndExpression {
    // left && right
    public:
    BinaryLogicalAndExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct LogicalOrExpression: public BinaryExpression {};

struct BaseLogicalOrExpression: public LogicalOrExpression {
    // TODO
};

struct BinaryLogicalOrExpression: public LogicalOrExpression {
    // left && right
    public:
    BinaryLogicalOrExpression(BinaryExpression left, BinaryExpression right) : _left(left), _right(right) {};
    private:
    BinaryExpression _left;
    BinaryExpression _right;
};

struct ConditionalExpression {};

struct BaseConditionalExpression: public ConditionalExpression {
    // TODO
    LogicalOrExpression inner;
};

struct TernaryExpression: public ConditionalExpression {
    // condition ? left : right
    LogicalOrExpression condition;
    Expression left;
    ConditionalExpression right;
};

struct AssignmentExpression: public Expression {};

struct BaseAssignmentExpression: public AssignmentExpression {
    // TODO
    ConditionalExpression inner;
};

struct EqualAssignExpression: public AssignmentExpression {
    // left = right
    UnaryExpression left;
    AssignmentExpression right;
};


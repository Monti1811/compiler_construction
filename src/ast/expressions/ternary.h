#include "expression.h"

struct TernaryExpression : public Expression {
    // condition ? left : right

  public:
    TernaryExpression(Locatable loc, ExpressionPtr condition, ExpressionPtr left, ExpressionPtr right)
        : Expression(loc)
        , _condition(std::move(condition))
        , _left(std::move(left))
        , _right(std::move(right)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    ExpressionPtr _condition;
    ExpressionPtr _left;
    ExpressionPtr _right;
};

#pragma once

#include <memory>
#include <optional>

#include "../../util/diagnostic.h"

#include "../scope.h"

enum DeclaratorKind { PRIMITIVE, FUNCTION, POINTER };

struct Declarator {
  public:
    Declarator(const Locatable loc, const bool abstract, const DeclaratorKind kind)
        : loc(loc)
        , kind(kind)
        , _abstract(abstract){};

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Declarator>& declarator);

    bool isAbstract();

    virtual std::optional<Symbol> getName() = 0;
    virtual TypePtr wrapType(TypePtr const&, ScopePtr&) = 0;
    virtual bool isFunction() = 0;

    const Locatable loc;
    const DeclaratorKind kind;

  private:
    bool _abstract = false;
};

typedef std::unique_ptr<Declarator> DeclaratorPtr;

struct PrimitiveDeclarator : public Declarator {
  public:
    PrimitiveDeclarator(Locatable loc, Symbol ident)
        : Declarator(loc, false, DeclaratorKind::PRIMITIVE)
        , _ident(ident){};

    PrimitiveDeclarator(Locatable loc)
        : Declarator(loc, true, DeclaratorKind::PRIMITIVE)
        , _ident(std::nullopt){};

    void print(std::ostream& stream) override;

    std::optional<Symbol> getName();
    TypePtr wrapType(TypePtr const& type, ScopePtr&);

    bool isAbstract();
    bool isFunction();

  private:
    std::optional<Symbol> _ident;
};

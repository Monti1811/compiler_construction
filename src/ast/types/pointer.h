#pragma once

#include "type.h"

struct PointerType: public Type {
    public:
    PointerType(TypePtr inner)
        : Type(TypeKind::TY_POINTER)
        , inner(inner) {};

    bool equals(TypePtr const& other) override;
    bool strong_equals(TypePtr const& other) override;
    llvm::PointerType* toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx);

    TypePtr inner; 
};

static TypePtr STRING_TYPE = std::make_shared<PointerType>(CHAR_TYPE);

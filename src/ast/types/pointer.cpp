#include "pointer.h"

bool PointerType::equals(TypePtr const& other) {
    // return true when other or this pointer is a nullptr
    if (other->kind == TypeKind::TY_NULLPTR || inner->kind == TypeKind::TY_VOID) {
        return true;
    }
    if (other->kind == TypeKind::TY_POINTER) {
        auto other_pointer = std::static_pointer_cast<PointerType>(other);
        if (other_pointer->inner->kind == TY_VOID) {
            return true;
        }
        return this->inner->equals(other_pointer->inner);
    } else {
        return false;
    }
}

bool PointerType::strong_equals(TypePtr const& other) {
    if (other->kind != TypeKind::TY_POINTER) {
        return false;
    }
    auto other_pointer = std::static_pointer_cast<PointerType>(other);
    return this->inner->strong_equals(other_pointer->inner);
}

llvm::Type* PointerType::toLLVMType(CompileScopePtr compile_scope) {
    if (this->inner->kind == TY_STRUCT) {
        auto inner_type = this->inner->toLLVMType(compile_scope);
        return llvm::PointerType::getUnqual(inner_type);
    }
    return compile_scope->_Builder.getPtrTy();
}

#pragma once

#include "type.h"

struct StructType : public Type {
  public:
    StructType(std::optional<Symbol> tag, int scope_counter)
        : Type(TypeKind::TY_STRUCT)
        , tag(tag)
        , scope_counter(scope_counter){};

    // Note: We do not consider fields in the equality check.
    // (Hence they are not overridden in `CompleteStructType`)
    bool equals(TypePtr const& other) override;
    bool strong_equals(TypePtr const& other) override;

    llvm::Type* toLLVMType(CompileScopePtr compile_scope);

    std::optional<Symbol> tag;
    int scope_counter;

  protected:
    std::string _alt_tag;
};

struct CompleteStructType : public StructType {
  public:
    CompleteStructType(std::optional<Symbol> tag, int scope_counter)
        : StructType(tag, scope_counter){};

    bool isComplete() override;

    // Returns true if field was already defined, false otherwise.
    bool addField(StructField field);

    // Adds all fields of another struct to this struct.
    // Returns true if any field was already defined, false otherwise.
    bool combineWith(CompleteStructType const& other);

    // Returns false if the constraints for named fields are not satisfied, true otherwise.
    bool validateFields();

    llvm::Type* toLLVMType(CompileScopePtr compile_scope);
    llvm::Type* toLLVMTypeAnonymous(CompileScopePtr compile_scope);

    std::optional<TypePtr> typeOfField(Symbol& ident);
    size_t getIndexOfField(Symbol field);

    std::vector<StructField> fields;

  private:
    std::unordered_map<Symbol, size_t> _field_names;
    std::optional<std::string> _llvm_name;
};

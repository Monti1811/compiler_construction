#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

struct Specifier {
    public:
    Specifier(const Locatable loc) : _loc(loc) {};
    private:
    const Locatable _loc;
};

struct VoidSpecifier: public Specifier {
    public: 
    VoidSpecifier(const Locatable loc) : Specifier(loc) {};
};

struct CharSpecifier: public Specifier {
    public: 
    CharSpecifier(const Locatable loc) : Specifier(loc) {};
};

struct IntSpecifier: public Specifier {
    public: 
    IntSpecifier(const Locatable loc) : Specifier(loc) {};
};

struct StructSpecifier: public Specifier {
    public:
    StructSpecifier(const Locatable loc, Symbol tag) : Specifier(loc), _tag(tag) {};
    private:
    Symbol _tag;
};

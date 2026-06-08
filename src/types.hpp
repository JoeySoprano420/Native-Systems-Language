#pragma once
// NSL Type System
// Resolved types after semantic analysis.

#include "ast.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Type kinds
// ──────────────────────────────────────────────────────────────────────────
enum class TypeKind {
    Void,
    Int,
    Float,
    Double,
    Bool,
    Char,
    String,
    Array,      // array<T>
    Ptr,        // ptr<T>
    Ref,        // ref<T>
    Struct,     // user-defined type
    Enum,
    Function,
    Unknown,
    Null,
};

struct Type;
using TypePtr = std::shared_ptr<Type>;

// ──────────────────────────────────────────────────────────────────────────
// Resolved Type
// ──────────────────────────────────────────────────────────────────────────
struct Type {
    TypeKind    kind   = TypeKind::Unknown;
    std::string name;                   // canonical name
    TypePtr     inner;                  // for ptr<T>, ref<T>, array<T>
    std::vector<TypePtr> params;        // function param types
    TypePtr     returnType;             // function return type

    // Struct members
    struct Field {
        std::string name;
        TypePtr     type;
        bool        isPublic  = true;
    };
    std::vector<Field> fields;

    bool isVoid()   const { return kind == TypeKind::Void; }
    bool isInt()    const { return kind == TypeKind::Int; }
    bool isFloat()  const { return kind == TypeKind::Float || kind == TypeKind::Double; }
    bool isBool()   const { return kind == TypeKind::Bool; }
    bool isString() const { return kind == TypeKind::String; }
    bool isPtr()    const { return kind == TypeKind::Ptr; }
    bool isRef()    const { return kind == TypeKind::Ref; }
    bool isArray()  const { return kind == TypeKind::Array; }
    bool isNumeric()const { return isInt() || isFloat(); }
    bool isNull()   const { return kind == TypeKind::Null; }

    std::string toString() const;
    bool isCompatibleWith(const Type& other) const;

    // Factory helpers
    static TypePtr makeVoid()   { return make(TypeKind::Void,   "void"); }
    static TypePtr makeInt()    { return make(TypeKind::Int,    "int"); }
    static TypePtr makeFloat()  { return make(TypeKind::Float,  "float"); }
    static TypePtr makeDouble() { return make(TypeKind::Double, "double"); }
    static TypePtr makeBool()   { return make(TypeKind::Bool,   "bool"); }
    static TypePtr makeChar()   { return make(TypeKind::Char,   "char"); }
    static TypePtr makeString() { return make(TypeKind::String, "string"); }
    static TypePtr makeNull()   { return make(TypeKind::Null,   "null"); }
    static TypePtr makeUnknown(){ return make(TypeKind::Unknown, "unknown"); }
    static TypePtr makePtr(TypePtr inner) {
        auto t = make(TypeKind::Ptr, "ptr"); t->inner = std::move(inner); return t;
    }
    static TypePtr makeRef(TypePtr inner) {
        auto t = make(TypeKind::Ref, "ref"); t->inner = std::move(inner); return t;
    }
    static TypePtr makeArray(TypePtr elem) {
        auto t = make(TypeKind::Array, "array"); t->inner = std::move(elem); return t;
    }

private:
    static TypePtr make(TypeKind k, const std::string& n) {
        auto t = std::make_shared<Type>(); t->kind = k; t->name = n; return t;
    }
};

// ──────────────────────────────────────────────────────────────────────────
// Type Registry – knows all named types
// ──────────────────────────────────────────────────────────────────────────
class TypeRegistry {
public:
    TypeRegistry();

    // Register a user-defined struct type
    void registerType(const std::string& name, TypePtr t);

    // Resolve a TypeExpr to a TypePtr (nullptr on failure)
    TypePtr resolve(const TypeExpr& te) const;

    // Look up by canonical name
    TypePtr lookup(const std::string& name) const;

    // Check if two types are compatible (assignment / argument passing)
    bool isCompatible(const TypePtr& from, const TypePtr& to) const;

private:
    std::unordered_map<std::string, TypePtr> m_types;

    void registerBuiltins();
};

} // namespace nsl

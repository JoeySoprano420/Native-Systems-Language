// NSL Type System Implementation

#include "types.hpp"
#include <sstream>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Type::toString
// ──────────────────────────────────────────────────────────────────────────
std::string Type::toString() const {
    switch (kind) {
        case TypeKind::Void:    return "void";
        case TypeKind::Int:     return "int";
        case TypeKind::Float:   return "float";
        case TypeKind::Double:  return "double";
        case TypeKind::Bool:    return "bool";
        case TypeKind::Char:    return "char";
        case TypeKind::String:  return "string";
        case TypeKind::Null:    return "null";
        case TypeKind::Array:   return "array<" + (inner ? inner->toString() : "?") + ">";
        case TypeKind::Ptr:     return "ptr<"   + (inner ? inner->toString() : "?") + ">";
        case TypeKind::Ref:     return "ref<"   + (inner ? inner->toString() : "?") + ">";
        case TypeKind::Struct:  return name;
        case TypeKind::Enum:    return name;
        case TypeKind::Function:return "fn";
        default:                return "unknown";
    }
}

bool Type::isCompatibleWith(const Type& other) const {
    if (kind == TypeKind::Unknown || other.kind == TypeKind::Unknown) return true;
    if (kind == TypeKind::Null &&
        (other.kind == TypeKind::Ptr || other.kind == TypeKind::Ref)) return true;
    if (kind == other.kind) {
        // For generic types, inner types must also match
        if (inner && other.inner) return inner->isCompatibleWith(*other.inner);
        return true;
    }
    // Numeric promotions
    if (isNumeric() && other.isNumeric()) return true;
    return false;
}

// ──────────────────────────────────────────────────────────────────────────
// TypeRegistry
// ──────────────────────────────────────────────────────────────────────────
TypeRegistry::TypeRegistry() {
    registerBuiltins();
}

void TypeRegistry::registerBuiltins() {
    m_types["void"]   = Type::makeVoid();
    m_types["int"]    = Type::makeInt();
    m_types["float"]  = Type::makeFloat();
    m_types["double"] = Type::makeDouble();
    m_types["bool"]   = Type::makeBool();
    m_types["char"]   = Type::makeChar();
    m_types["string"] = Type::makeString();
    m_types["null"]   = Type::makeNull();
    // Common aliases
    m_types["i8"]   = Type::makeInt();
    m_types["i16"]  = Type::makeInt();
    m_types["i32"]  = Type::makeInt();
    m_types["i64"]  = Type::makeInt();
    m_types["u8"]   = Type::makeInt();
    m_types["u16"]  = Type::makeInt();
    m_types["u32"]  = Type::makeInt();
    m_types["u64"]  = Type::makeInt();
    m_types["f32"]  = Type::makeFloat();
    m_types["f64"]  = Type::makeDouble();
    m_types["byte"] = Type::makeInt();
    m_types["size"] = Type::makeInt();
    // Common library types (treated as opaque structs)
    auto makeOpaque = [](const std::string& n) {
        auto t = std::make_shared<Type>(); t->kind = TypeKind::Struct; t->name = n; return t;
    };
    m_types["Buffer"]      = makeOpaque("Buffer");
    m_types["Player"]      = makeOpaque("Player");
    m_types["Entity"]      = makeOpaque("Entity");
    m_types["Packet"]      = makeOpaque("Packet");
    m_types["AudioBuffer"] = makeOpaque("AudioBuffer");
    m_types["Vector3"]     = makeOpaque("Vector3");
    m_types["EnemyPool"]   = makeOpaque("EnemyPool");
}

void TypeRegistry::registerType(const std::string& name, TypePtr t) {
    m_types[name] = std::move(t);
}

TypePtr TypeRegistry::lookup(const std::string& name) const {
    auto it = m_types.find(name);
    if (it != m_types.end()) return it->second;
    return nullptr;
}

TypePtr TypeRegistry::resolve(const TypeExpr& te) const {
    // ptr<T>
    if (te.name == "ptr" || te.isPtr) {
        TypePtr inner = te.params.empty() ? Type::makeUnknown() : resolve(te.params[0]);
        return Type::makePtr(std::move(inner));
    }
    // ref<T>
    if (te.name == "ref" || te.isRef) {
        TypePtr inner = te.params.empty() ? Type::makeUnknown() : resolve(te.params[0]);
        return Type::makeRef(std::move(inner));
    }
    // array<T>
    if (te.name == "array") {
        TypePtr elem = te.params.empty() ? Type::makeUnknown() : resolve(te.params[0]);
        return Type::makeArray(std::move(elem));
    }

    // Look up registered name
    auto it = m_types.find(te.name);
    if (it != m_types.end()) return it->second;

    // Unknown user type – create an opaque struct placeholder
    auto t = std::make_shared<Type>();
    t->kind = TypeKind::Struct;
    t->name = te.name;
    return t;
}

bool TypeRegistry::isCompatible(const TypePtr& from, const TypePtr& to) const {
    if (!from || !to) return true; // unknown → accept
    return from->isCompatibleWith(*to);
}

} // namespace nsl

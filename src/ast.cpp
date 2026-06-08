// NSL AST helper implementations

#include "ast.hpp"
#include <sstream>

namespace nsl {

std::string typeExprToString(const TypeExpr& t) {
    if (t.params.empty()) return t.name;
    std::string s = t.name + "<";
    for (std::size_t i = 0; i < t.params.size(); ++i) {
        if (i) s += ", ";
        s += typeExprToString(t.params[i]);
    }
    s += ">";
    return s;
}

} // namespace nsl

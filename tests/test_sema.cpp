// NSL Semantic Analysis Tests

#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "diagnostics.hpp"
#include <cassert>
#include <iostream>

using namespace nsl;

extern int g_pass;
extern int g_fail;

#define ASSERT(cond, msg)                                               \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cerr << "FAIL [" << __func__ << "]: " << (msg) << "\n"; \
            ++g_fail;                                                   \
        } else {                                                        \
            ++g_pass;                                                   \
        }                                                               \
    } while(0)

struct SemaResult {
    bool    ok;
    int     errors;
    int     warnings;
};

static SemaResult analyseSource(const std::string& src) {
    Lexer lex(src, "<test>");
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens), nullptr);
    auto prog = parser.parseProgram();
    if (!prog) return {false, 1, 0};

    DiagnosticEngine diag;
    SemanticAnalyzer sema(diag);
    bool ok = sema.analyse(*prog);
    return {ok, diag.errorCount(), diag.warningCount()};
}

static void test_basic_types() {
    auto r = analyseSource(R"(
program Types

fn main() -> int

    let x: int = 42

    let s: string = "hello"

    let b: bool = true

    return 0

end
)");
    ASSERT(r.ok, "Basic types should type-check cleanly");
    ASSERT(r.errors == 0, "Should have no errors");
}

static void test_type_inference() {
    auto r = analyseSource(R"(
program Infer

fn main() -> int

    let x = 10

    let name = "Joey"

    let alive = true

    return 0

end
)");
    ASSERT(r.ok, "Type inference should work");
    ASSERT(r.errors == 0, "Should have no errors");
}

static void test_break_outside_loop() {
    auto r = analyseSource(R"(
program BadBreak

fn main() -> int

    break

    return 0

end
)");
    ASSERT(!r.ok || r.errors > 0, "break outside loop should produce an error");
}

static void test_const_mutation() {
    auto r = analyseSource(R"(
program ConstMut

const MaxHealth: int = 100

fn main() -> int

    set MaxHealth = 200

    return 0

end
)");
    // Should warn or error about mutating a constant
    ASSERT(r.errors > 0, "Mutating a const should produce an error");
}

static void test_function_forward_refs() {
    auto r = analyseSource(R"(
program Refs

fn add(a: int, b: int) -> int

    return a + b

end

fn main() -> int

    let result: int = add(1, 2)

    return result

end
)");
    ASSERT(r.errors == 0, "Forward function references should work");
}

static void test_user_type_fields() {
    auto r = analyseSource(R"(
program UserType

type Point

    let x: float

    let y: float

end

fn main() -> int

    let p: Point = Point()

    return 0

end
)");
    ASSERT(r.errors == 0, "User-defined type should be resolvable");
}

void run_sema_tests() {
    std::cout << "=== Semantic Analysis Tests ===\n";
    test_basic_types();
    test_type_inference();
    test_break_outside_loop();
    test_const_mutation();
    test_function_forward_refs();
    test_user_type_fields();
}

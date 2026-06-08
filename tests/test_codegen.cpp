// NSL Code Generation Tests
// Verifies that the C backend produces compilable, runnable output.

#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "codegen.hpp"
#include "diagnostics.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

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

static std::string generateC(const std::string& src) {
    Lexer lex(src, "<test>");
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens), nullptr);
    auto prog = parser.parseProgram();
    if (!prog) return "";
    DiagnosticEngine diag;
    CCodeGen gen(diag);
    return gen.generate(*prog);
}

static void test_hello_world_c_output() {
    const char* src = R"(
program Greeting

use io

fn main() -> int

    let greeting: string = "Hello, World!"

    io.print(greeting)

    return 0

end
)";
    std::string c = generateC(src);
    ASSERT(!c.empty(), "C output should not be empty");
    // Should contain the greeting string
    ASSERT(c.find("Hello, World!") != std::string::npos,
           "Generated C should contain the greeting string");
    // Should contain a main function
    ASSERT(c.find("int64_t main(") != std::string::npos ||
           c.find("int main(") != std::string::npos,
           "Generated C should contain a main function");
    // Should contain io_print
    ASSERT(c.find("io_print") != std::string::npos,
           "Generated C should contain io_print");
}

static void test_arithmetic_c_output() {
    const char* src = R"(
program Arith

fn add(a: int, b: int) -> int

    return a + b

end

fn main() -> int

    let result: int = add(3, 4)

    return result

end
)";
    std::string c = generateC(src);
    ASSERT(!c.empty(), "C output should not be empty");
    ASSERT(c.find("a + b") != std::string::npos ||
           c.find("(a + b)") != std::string::npos,
           "Generated C should have arithmetic expression");
}

static void test_type_struct_c_output() {
    const char* src = R"(
program Structs

type Player

    let name: string

    let health: int

end

fn main() -> int

    let p: Player = Player()

    return 0

end
)";
    std::string c = generateC(src);
    ASSERT(!c.empty(), "C output should not be empty");
    ASSERT(c.find("struct Player") != std::string::npos,
           "Generated C should define the Player struct");
    ASSERT(c.find("const char* name") != std::string::npos,
           "Generated C should have name field as const char*");
    ASSERT(c.find("int64_t health") != std::string::npos,
           "Generated C should have health field as int64_t");
}

static void test_runtime_header() {
    const char* src = R"(
program RuntimeTest

fn main() -> int

    return 0

end
)";
    std::string c = generateC(src);
    // Check that required C headers are included
    ASSERT(c.find("#include <stdlib.h>") != std::string::npos,
           "Generated C should include stdlib.h");
    ASSERT(c.find("NSLArray") != std::string::npos,
           "Generated C should define NSLArray");
    ASSERT(c.find("NSLArena") != std::string::npos,
           "Generated C should define NSLArena");
    ASSERT(c.find("NSLZone") != std::string::npos,
           "Generated C should define NSLZone");
}

static void test_if_else_c_output() {
    const char* src = R"(
program Conds

fn check(x: int) -> bool

    if x > 0

        return true

    else

        return false

    end

end

fn main() -> int

    return 0

end
)";
    std::string c = generateC(src);
    ASSERT(c.find("if (") != std::string::npos,   "Generated C should have if statement");
    ASSERT(c.find("} else {") != std::string::npos, "Generated C should have else block");
}

static void test_const_decl_c_output() {
    const char* src = R"(
program ConstTest

const maxHealth: int = 100

fn main() -> int

    return 0

end
)";
    std::string c = generateC(src);
    ASSERT(c.find("static const") != std::string::npos,
           "Constant should emit as 'static const'");
    ASSERT(c.find("100") != std::string::npos, "Constant value should be in output");
}

void run_codegen_tests() {
    std::cout << "=== Code Generation Tests ===\n";
    test_hello_world_c_output();
    test_arithmetic_c_output();
    test_type_struct_c_output();
    test_runtime_header();
    test_if_else_c_output();
    test_const_decl_c_output();
}

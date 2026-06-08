// NSL Parser Tests

#include "lexer.hpp"
#include "parser.hpp"
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

static std::unique_ptr<Program> parseSource(const std::string& src) {
    Lexer lex(src, "<test>");
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens), nullptr);
    return parser.parseProgram();
}

static void test_hello_world_parse() {
    const char* src = R"(
program Greeting

use io

fn main() -> int

    let greeting: string = "Hello, World!"

    io.print(greeting)

    return 0

end
)";
    auto prog = parseSource(src);
    ASSERT(prog != nullptr, "Program should parse successfully");
    ASSERT(prog->programName == "Greeting", "Program name should be Greeting");

    int fnCount = 0, useCount = 0;
    for (auto& d : prog->decls) {
        if (dynamic_cast<FunctionDecl*>(d.get())) ++fnCount;
        if (dynamic_cast<UseDecl*>(d.get()))      ++useCount;
    }
    ASSERT(fnCount == 1, "Should have 1 function");
    ASSERT(useCount == 1, "Should have 1 use declaration");
}

static void test_type_decl() {
    const char* src = R"(
program Types

type Player

    let name: string

    let health: int

end
)";
    auto prog = parseSource(src);
    ASSERT(prog != nullptr, "Program should parse");

    TypeDecl* td = nullptr;
    for (auto& d : prog->decls)
        if (auto* t = dynamic_cast<TypeDecl*>(d.get())) { td = t; break; }

    ASSERT(td != nullptr, "Should have a type declaration");
    ASSERT(td->name == "Player", "Type name should be Player");
    ASSERT(td->fields.size() == 2u, "Player should have 2 fields");
    ASSERT(td->fields[0].name == "name", "First field should be 'name'");
    ASSERT(td->fields[1].name == "health", "Second field should be 'health'");
}

static void test_if_else() {
    const char* src = R"(
program Cond

fn check(health: int) -> bool

    if health <= 0

        return false

    else

        return true

    end

end
)";
    auto prog = parseSource(src);
    ASSERT(prog != nullptr, "Program should parse");
    FunctionDecl* fn = nullptr;
    for (auto& d : prog->decls)
        if (auto* f = dynamic_cast<FunctionDecl*>(d.get())) { fn = f; break; }
    ASSERT(fn != nullptr, "Should have a function");
    ASSERT(!fn->body.empty(), "Function should have body");
    IfStmt* ifStmt = nullptr;
    for (auto& s : fn->body)
        if (auto* is = dynamic_cast<IfStmt*>(s.get())) { ifStmt = is; break; }
    ASSERT(ifStmt != nullptr, "Should have an if statement");
    ASSERT(!ifStmt->thenBlock.empty(), "Then block should not be empty");
    ASSERT(!ifStmt->elseBlock.empty(), "Else block should not be empty");
}

static void test_for_loop() {
    const char* src = R"(
program Loop

fn update(enemies: array<Entity>) -> void

    for enemy in enemies

        enemy.update()

    end

end
)";
    auto prog = parseSource(src);
    ASSERT(prog != nullptr, "Program should parse");
    FunctionDecl* fn = nullptr;
    for (auto& d : prog->decls)
        if (auto* f = dynamic_cast<FunctionDecl*>(d.get())) { fn = f; break; }
    ASSERT(fn != nullptr, "Should have function");
    ForStmt* forStmt = nullptr;
    for (auto& s : fn->body)
        if (auto* fs = dynamic_cast<ForStmt*>(s.get())) { forStmt = fs; break; }
    ASSERT(forStmt != nullptr, "Should have a for loop");
    ASSERT(forStmt->iterVar == "enemy", "Iter var should be 'enemy'");
}

static void test_match_stmt() {
    const char* src = R"(
program Match

fn run(state: int) -> void

    match state

        case 1
            return

        case 2
            return

        default
            return

    end

end
)";
    auto prog = parseSource(src);
    ASSERT(prog != nullptr, "Program should parse");
    FunctionDecl* fn = nullptr;
    for (auto& d : prog->decls)
        if (auto* f = dynamic_cast<FunctionDecl*>(d.get())) { fn = f; break; }
    ASSERT(fn != nullptr, "Should have function");
    MatchStmt* ms = nullptr;
    for (auto& s : fn->body)
        if (auto* m = dynamic_cast<MatchStmt*>(s.get())) { ms = m; break; }
    ASSERT(ms != nullptr, "Should have a match statement");
    ASSERT(ms->cases.size() == 3u, "Match should have 3 cases (including default)");
    ASSERT(ms->cases[2].pattern == nullptr, "Last case should be default (null pattern)");
}

static void test_memory_regions() {
    const char* src = R"(
program Memory

fn demo() -> int

    let temp: Buffer in stack = Buffer(256)

    let player: ptr<Player> in heap = new Player()

    let enemy: Entity in arena LevelArena = Entity()

    let data: Packet in zone SecureZone = Packet()

    return 0

end
)";
    auto prog = parseSource(src);
    ASSERT(prog != nullptr, "Program should parse");
    FunctionDecl* fn = nullptr;
    for (auto& d : prog->decls)
        if (auto* f = dynamic_cast<FunctionDecl*>(d.get())) { fn = f; break; }
    ASSERT(fn != nullptr, "Should have function");

    int stackCount = 0, heapCount = 0, arenaCount = 0, zoneCount = 0;
    for (auto& s : fn->body) {
        if (auto* ld = dynamic_cast<LetDecl*>(s.get())) {
            switch (ld->memSpec.region) {
                case MemRegion::Stack: ++stackCount; break;
                case MemRegion::Heap:  ++heapCount;  break;
                case MemRegion::Arena: ++arenaCount; break;
                case MemRegion::Zone:  ++zoneCount;  break;
                default: break;
            }
        }
    }
    ASSERT(stackCount == 1, "Should have 1 stack allocation");
    ASSERT(heapCount  == 1, "Should have 1 heap allocation");
    ASSERT(arenaCount == 1, "Should have 1 arena allocation");
    ASSERT(zoneCount  == 1, "Should have 1 zone allocation");
}

static void test_try_catch() {
    const char* src = R"(
program Errors

fn open_file(path: string) -> bool

    try

        return true

    catch

        return false

    end

end
)";
    auto prog = parseSource(src);
    ASSERT(prog != nullptr, "Program should parse");
    FunctionDecl* fn = nullptr;
    for (auto& d : prog->decls)
        if (auto* f = dynamic_cast<FunctionDecl*>(d.get())) { fn = f; break; }
    ASSERT(fn != nullptr, "Should have function");
    TryCatchStmt* tc = nullptr;
    for (auto& s : fn->body)
        if (auto* t = dynamic_cast<TryCatchStmt*>(s.get())) { tc = t; break; }
    ASSERT(tc != nullptr, "Should have a try-catch");
    ASSERT(!tc->tryBlock.empty(),   "Try block should not be empty");
    ASSERT(!tc->catchBlock.empty(), "Catch block should not be empty");
}

void run_parser_tests() {
    std::cout << "=== Parser Tests ===\n";
    test_hello_world_parse();
    test_type_decl();
    test_if_else();
    test_for_loop();
    test_match_stmt();
    test_memory_regions();
    test_try_catch();
}

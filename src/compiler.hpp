#pragma once
// NSL Compiler Driver
// Orchestrates the full compilation pipeline:
//   source → lex → parse → sema → ir-gen → codegen → C file → cc → binary

#include "ast.hpp"
#include "diagnostics.hpp"
#include "ir.hpp"
#include <string>
#include <vector>
#include <memory>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Build mode (--debug / --release / etc.)
// ──────────────────────────────────────────────────────────────────────────
enum class BuildMode {
    Debug,
    Release,
    Fast,
    Safe,
    Secure,
    Realtime,
    Embedded,
    Size,
    Profile,
    Obfuscate,
};

// ──────────────────────────────────────────────────────────────────────────
// Emit target
// ──────────────────────────────────────────────────────────────────────────
enum class EmitTarget {
    Binary,   // default – native executable
    C,        // --emit-c
    IR,       // --emit-ir
    AST,      // --emit-ast  (textual dump)
    Tokens,   // --emit-tokens (lex dump)
};

// ──────────────────────────────────────────────────────────────────────────
// Compiler options
// ──────────────────────────────────────────────────────────────────────────
struct CompilerOptions {
    std::string   inputFile;
    std::string   outputFile;    // empty → derive from input
    BuildMode     mode         = BuildMode::Debug;
    EmitTarget    emitTarget   = EmitTarget::Binary;
    bool          verbose      = false;
    bool          noColor      = false;
    bool          printVersion = false;
    std::string   target;        // cross-compilation target triple
    std::vector<std::string> extraCFlags; // pass-through flags to the C compiler
};

// ──────────────────────────────────────────────────────────────────────────
// Compiler
// ──────────────────────────────────────────────────────────────────────────
class Compiler {
public:
    explicit Compiler(CompilerOptions opts);

    // Run the full pipeline; returns 0 on success, non-zero on failure.
    int run();

    // Individual pipeline stages (public for testing / --emit-* flags)
    std::string    readSource(const std::string& path);
    std::vector<Token> lex(const std::string& src, const std::string& filename);
    std::unique_ptr<Program> parse(std::vector<Token> tokens, const std::string& filename);
    bool sema(Program& prog);
    ir::Module genIR(Program& prog);
    std::string genC(Program& prog);

    // Invoke system C compiler on the generated C file → native binary
    int compileC(const std::string& cFile, const std::string& outFile);

    DiagnosticEngine& diag() { return m_diag; }

private:
    CompilerOptions  m_opts;
    DiagnosticEngine m_diag;

    std::string defaultOutputName() const;
    std::string buildModeFlagsForCC() const;
};

} // namespace nsl

// NSL Compiler Driver Implementation

#include "compiler.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "irgen.hpp"
#include "codegen.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>

namespace nsl {

// ──────────────────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────────────────
Compiler::Compiler(CompilerOptions opts)
    : m_opts(std::move(opts))
    , m_diag([this](const Diagnostic& d) {
          d; // printed at end via printAll()
      })
{}

// ──────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────
std::string Compiler::readSource(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        m_diag.error("Cannot open file '" + path + "'", {path, 0, 0});
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string Compiler::defaultOutputName() const {
    std::string base = m_opts.inputFile;
    // strip directory
    auto slash = base.find_last_of("/\\");
    if (slash != std::string::npos) base = base.substr(slash + 1);
    // strip .nsl extension
    auto dot = base.rfind(".nsl");
    if (dot != std::string::npos) base = base.substr(0, dot);
#ifdef _WIN32
    return base + ".exe";
#else
    return base;
#endif
}

std::string Compiler::buildModeFlagsForCC() const {
    switch (m_opts.mode) {
        case BuildMode::Release:  return "-O2";
        case BuildMode::Fast:     return "-O3 -ffast-math";
        case BuildMode::Safe:     return "-O1 -fsanitize=address,undefined";
        case BuildMode::Size:     return "-Os";
        case BuildMode::Profile:  return "-O2 -pg";
        case BuildMode::Realtime: return "-O2 -fno-exceptions";
        case BuildMode::Embedded: return "-Os -fno-exceptions -fno-rtti";
        default:                  return "-O0 -g";
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Pipeline stages
// ──────────────────────────────────────────────────────────────────────────
std::vector<Token> Compiler::lex(const std::string& src, const std::string& filename) {
    Lexer lexer(src, filename, [this](const std::string& msg, const SourceLocation& loc) {
        m_diag.error(msg, loc);
    });
    return lexer.tokenize();
}

std::unique_ptr<Program> Compiler::parse(std::vector<Token> tokens,
                                         const std::string& filename) {
    Parser parser(std::move(tokens), [this](const std::string& msg, const SourceLocation& loc) {
        m_diag.error(msg, loc);
    });
    return parser.parseProgram();
}

bool Compiler::sema(Program& prog) {
    SemanticAnalyzer analyzer(m_diag);
    return analyzer.analyse(prog);
}

ir::Module Compiler::genIR(Program& prog) {
    SemanticAnalyzer semaPass(m_diag);
    semaPass.analyse(prog);
    IRGen gen(m_diag, semaPass);
    return gen.generate(prog);
}

std::string Compiler::genC(Program& prog) {
    CCodeGen gen(m_diag);
    return gen.generate(prog);
}

int Compiler::compileC(const std::string& cFile, const std::string& outFile) {
    // Choose the C compiler: prefer clang, fall back to gcc, then cc
    const char* cc = std::getenv("CC");
    std::string compiler = cc ? cc : "cc";

    std::string flags = buildModeFlagsForCC();
    std::string cmd   = compiler + " " + flags + " -std=c99 -o " + outFile + " " + cFile;
    if (m_opts.verbose) std::cout << "[nslc] " << cmd << "\n";
    return std::system(cmd.c_str());
}

// ──────────────────────────────────────────────────────────────────────────
// Main pipeline
// ──────────────────────────────────────────────────────────────────────────
int Compiler::run() {
    // ── Print version ──────────────────────────────────────────────────────
    if (m_opts.printVersion) {
        std::cout << "nslc 1.0.0 - Native Systems Language Compiler\n";
        std::cout << "50 Words. Clean Syntax. Huge Compiler. Native Power. Machine Truth.\n";
        return 0;
    }

    if (m_opts.inputFile.empty()) {
        std::cerr << "nslc: error: no input file\n";
        return 1;
    }

    // ── Read source ────────────────────────────────────────────────────────
    if (m_opts.verbose)
        std::cout << "[nslc] Reading " << m_opts.inputFile << "\n";
    std::string source = readSource(m_opts.inputFile);
    if (m_diag.hasErrors()) { m_diag.printAll(); return 1; }

    // ── Lex ────────────────────────────────────────────────────────────────
    if (m_opts.verbose) std::cout << "[nslc] Lexing...\n";
    auto tokens = lex(source, m_opts.inputFile);
    if (m_diag.hasErrors()) { m_diag.printAll(); return 1; }

    if (m_opts.emitTarget == EmitTarget::Tokens) {
        for (auto& t : tokens)
            std::cout << t.loc.toString() << "  " << t.kindName()
                      << "  [" << t.text << "]\n";
        return 0;
    }

    // ── Parse ──────────────────────────────────────────────────────────────
    if (m_opts.verbose) std::cout << "[nslc] Parsing...\n";
    auto prog = parse(std::move(tokens), m_opts.inputFile);
    if (!prog || m_diag.hasErrors()) { m_diag.printAll(); return 1; }

    if (m_opts.emitTarget == EmitTarget::AST) {
        // Simple AST dump (visitor-based pretty printer)
        std::cout << "program " << prog->programName << "\n";
        std::cout << "  declarations: " << prog->decls.size() << "\n";
        return 0;
    }

    // ── Semantic Analysis ──────────────────────────────────────────────────
    if (m_opts.verbose) std::cout << "[nslc] Semantic analysis...\n";
    // Run sema – errors are collected in m_diag
    {
        SemanticAnalyzer analyzer(m_diag);
        analyzer.analyse(*prog);
    }
    if (m_diag.hasErrors()) { m_diag.printAll(); return 1; }

    // ── IR Generation ──────────────────────────────────────────────────────
    if (m_opts.emitTarget == EmitTarget::IR) {
        if (m_opts.verbose) std::cout << "[nslc] Generating IR...\n";
        SemanticAnalyzer semaPass(m_diag);
        semaPass.analyse(*prog);
        IRGen irgen(m_diag, semaPass);
        ir::Module mod = irgen.generate(*prog);
        std::cout << mod.toString();
        return m_diag.hasErrors() ? 1 : 0;
    }

    // ── C Code Generation ──────────────────────────────────────────────────
    if (m_opts.verbose) std::cout << "[nslc] Generating C code...\n";
    std::string cCode = genC(*prog);
    if (m_diag.hasErrors()) { m_diag.printAll(); return 1; }

    if (m_opts.emitTarget == EmitTarget::C) {
        std::string outName = m_opts.outputFile.empty()
            ? (defaultOutputName() + ".c")
            : m_opts.outputFile;
        if (outName == "-") {
            std::cout << cCode;
        } else {
            std::ofstream f(outName);
            if (!f) {
                std::cerr << "nslc: error: cannot write to '" << outName << "'\n";
                return 1;
            }
            f << cCode;
            if (m_opts.verbose)
                std::cout << "[nslc] C output written to " << outName << "\n";
        }
        return 0;
    }

    // ── Compile C → native binary ──────────────────────────────────────────
    // Write C to a temp file
    std::string tmpC = "/tmp/__nsl_" + defaultOutputName() + ".c";
    {
        std::ofstream f(tmpC);
        if (!f) {
            std::cerr << "nslc: error: cannot write temporary C file\n";
            return 1;
        }
        f << cCode;
    }

    std::string outBin = m_opts.outputFile.empty()
        ? defaultOutputName()
        : m_opts.outputFile;

    if (m_opts.verbose) std::cout << "[nslc] Compiling C to binary...\n";
    int ret = compileC(tmpC, outBin);

    // Clean up temp file
    std::remove(tmpC.c_str());

    if (ret != 0) {
        std::cerr << "nslc: error: C compilation failed (exit " << ret << ")\n";
        m_diag.printAll();
        return 1;
    }

    if (m_opts.verbose)
        std::cout << "[nslc] Output: " << outBin << "\n";

    m_diag.printAll();
    return m_diag.hasErrors() ? 1 : 0;
}

} // namespace nsl

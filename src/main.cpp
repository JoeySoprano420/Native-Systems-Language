// nslc – Native Systems Language Compiler
// Entry point

#include "compiler.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

static void printUsage() {
    std::cout <<
        "nslc – Native Systems Language Compiler\n"
        "50 Words. Clean Syntax. Huge Compiler. Native Power. Machine Truth.\n\n"
        "Usage:\n"
        "  nslc <file.nsl> [options]\n\n"
        "Options:\n"
        "  -o <file>         Output file (default: derived from input name)\n"
        "  --debug           Build in debug mode (default)\n"
        "  --release         Build with optimizations\n"
        "  --fast            Build with aggressive optimizations\n"
        "  --safe            Build with sanitizers enabled\n"
        "  --secure          Build for secure execution\n"
        "  --realtime        Build for real-time execution\n"
        "  --embedded        Build for embedded targets\n"
        "  --size            Build optimized for binary size\n"
        "  --profile         Build with profiling support\n"
        "  --obfuscate       Build with symbol obfuscation\n"
        "  --emit-c          Emit generated C source and stop\n"
        "  --emit-ir         Emit NSL-IR and stop\n"
        "  --emit-ast        Emit AST dump and stop\n"
        "  --emit-tokens     Emit token stream and stop\n"
        "  -v, --verbose     Verbose output\n"
        "  --version         Print version and exit\n"
        "  -h, --help        Show this help\n\n"
        "Examples:\n"
        "  nslc hello.nsl\n"
        "  nslc hello.nsl --release\n"
        "  nslc hello.nsl --emit-c\n"
        "  nslc hello.nsl --emit-ir\n"
        "  nslc main.nsl -o myprogram --fast\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    nsl::CompilerOptions opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        }
        if (arg == "--version") {
            opts.printVersion = true;
            continue;
        }
        if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
            continue;
        }
        if (arg == "-o" && i + 1 < argc) {
            opts.outputFile = argv[++i];
            continue;
        }
        if (arg == "--debug")     { opts.mode = nsl::BuildMode::Debug;     continue; }
        if (arg == "--release")   { opts.mode = nsl::BuildMode::Release;   continue; }
        if (arg == "--fast")      { opts.mode = nsl::BuildMode::Fast;      continue; }
        if (arg == "--safe")      { opts.mode = nsl::BuildMode::Safe;      continue; }
        if (arg == "--secure")    { opts.mode = nsl::BuildMode::Secure;    continue; }
        if (arg == "--realtime")  { opts.mode = nsl::BuildMode::Realtime;  continue; }
        if (arg == "--embedded")  { opts.mode = nsl::BuildMode::Embedded;  continue; }
        if (arg == "--size")      { opts.mode = nsl::BuildMode::Size;      continue; }
        if (arg == "--profile")   { opts.mode = nsl::BuildMode::Profile;   continue; }
        if (arg == "--obfuscate") { opts.mode = nsl::BuildMode::Obfuscate; continue; }

        if (arg == "--emit-c")      { opts.emitTarget = nsl::EmitTarget::C;      continue; }
        if (arg == "--emit-ir")     { opts.emitTarget = nsl::EmitTarget::IR;     continue; }
        if (arg == "--emit-ast")    { opts.emitTarget = nsl::EmitTarget::AST;    continue; }
        if (arg == "--emit-tokens") { opts.emitTarget = nsl::EmitTarget::Tokens; continue; }

        if (arg[0] == '-') {
            std::cerr << "nslc: warning: unknown option '" << arg << "'\n";
            continue;
        }

        // Positional argument: input file
        if (opts.inputFile.empty()) {
            opts.inputFile = arg;
        } else {
            std::cerr << "nslc: warning: multiple input files given; ignoring '" << arg << "'\n";
        }
    }

    nsl::Compiler compiler(std::move(opts));
    return compiler.run();
}

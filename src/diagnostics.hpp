#pragma once
// NSL Diagnostic Engine
// Collects and reports errors, warnings, and notes.

#include "token.hpp"
#include <string>
#include <vector>
#include <functional>
#include <iostream>

namespace nsl {

enum class DiagSeverity { Note, Warning, Error, Fatal };

struct Diagnostic {
    DiagSeverity   severity;
    std::string    message;
    SourceLocation loc;
};

class DiagnosticEngine {
public:
    using Sink = std::function<void(const Diagnostic&)>;

    explicit DiagnosticEngine(Sink sink = nullptr);

    void error(const std::string& msg, const SourceLocation& loc);
    void warning(const std::string& msg, const SourceLocation& loc);
    void note(const std::string& msg, const SourceLocation& loc);

    bool hasErrors()    const { return m_errorCount > 0; }
    int  errorCount()   const { return m_errorCount; }
    int  warningCount() const { return m_warnCount; }

    const std::vector<Diagnostic>& diagnostics() const { return m_diags; }

    // Print all diagnostics to stderr in a clang-style format.
    void printAll(std::ostream& out = std::cerr) const;

private:
    Sink m_sink;
    std::vector<Diagnostic> m_diags;
    int m_errorCount = 0;
    int m_warnCount  = 0;

    void emit(DiagSeverity sev, const std::string& msg, const SourceLocation& loc);
};

} // namespace nsl

// NSL Diagnostic Engine Implementation

#include "diagnostics.hpp"
#include <sstream>
#include <iomanip>

namespace nsl {

DiagnosticEngine::DiagnosticEngine(Sink sink) : m_sink(std::move(sink)) {}

void DiagnosticEngine::emit(DiagSeverity sev, const std::string& msg,
                            const SourceLocation& loc) {
    Diagnostic d{sev, msg, loc};
    m_diags.push_back(d);
    if (sev == DiagSeverity::Error || sev == DiagSeverity::Fatal) ++m_errorCount;
    if (sev == DiagSeverity::Warning) ++m_warnCount;
    if (m_sink) m_sink(d);
}

void DiagnosticEngine::error(const std::string& msg, const SourceLocation& loc) {
    emit(DiagSeverity::Error, msg, loc);
}

void DiagnosticEngine::warning(const std::string& msg, const SourceLocation& loc) {
    emit(DiagSeverity::Warning, msg, loc);
}

void DiagnosticEngine::note(const std::string& msg, const SourceLocation& loc) {
    emit(DiagSeverity::Note, msg, loc);
}

void DiagnosticEngine::printAll(std::ostream& out) const {
    for (const auto& d : m_diags) {
        const std::string& file = d.loc.filename;
        out << (file.empty() ? "<input>" : file)
            << ":" << d.loc.line << ":" << d.loc.column << ": ";
        switch (d.severity) {
            case DiagSeverity::Note:    out << "note: ";    break;
            case DiagSeverity::Warning: out << "warning: "; break;
            case DiagSeverity::Error:   out << "error: ";   break;
            case DiagSeverity::Fatal:   out << "fatal: ";   break;
        }
        out << d.message << "\n";
    }
}

} // namespace nsl

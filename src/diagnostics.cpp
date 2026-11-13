#include "bminor/diagnostics.h"

#include <ostream>

namespace bminor {

void DiagnosticBag::report(Severity severity, SourceLocation loc, std::string message) {
  if (severity == Severity::Error) ++errorCount_;
  diagnostics_.push_back(Diagnostic{severity, std::move(message), loc});
}

void DiagnosticBag::print(std::ostream& out) const {
  for (const Diagnostic& d : diagnostics_) {
    const char* label = "note";
    if (d.severity == Severity::Error) label = "error";
    else if (d.severity == Severity::Warning) label = "warning";
    out << d.loc.toString(file_) << ": " << label << ": " << d.message << "\n";
  }
}

}  // namespace bminor

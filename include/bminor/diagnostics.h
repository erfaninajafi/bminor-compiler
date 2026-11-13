#pragma once

#include <string>
#include <vector>

#include "bminor/source.h"

namespace bminor {

enum class Severity { Note, Warning, Error };

struct Diagnostic {
  Severity severity = Severity::Error;
  std::string message;
  SourceLocation loc;
};

/// Collects diagnostics and can print them in a `file:line:col: error: ...`
/// format. Lexer and parser never abort on an error: they record a diagnostic
/// and keep going.
class DiagnosticBag {
 public:
  void report(Severity severity, SourceLocation loc, std::string message);
  void error(SourceLocation loc, std::string message) {
    report(Severity::Error, loc, std::move(message));
  }
  void warning(SourceLocation loc, std::string message) {
    report(Severity::Warning, loc, std::move(message));
  }

  const std::vector<Diagnostic>& all() const { return diagnostics_; }
  bool hasErrors() const { return errorCount_ > 0; }
  std::size_t errorCount() const { return errorCount_; }

  void setFileName(const std::string& name) { file_ = name; }
  const std::string& fileName() const { return file_; }

  void print(std::ostream& out) const;

 private:
  std::vector<Diagnostic> diagnostics_;
  std::size_t errorCount_ = 0;
  std::string file_ = "<input>";
};

}  // namespace bminor

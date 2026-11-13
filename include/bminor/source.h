#pragma once

#include <cstddef>
#include <string>

namespace bminor {

/// A position inside a source file.
///
/// `offset` is the zero based byte offset used to slice the original source
/// text back out (needed by the highlighter), while `line` and `column` are
/// one based human readable coordinates used in diagnostics. `line` is 1 when
/// the position does not come from real source text (a synthetic token created
/// during error recovery), and `valid` is false in that case.
struct SourceLocation {
  std::size_t offset = 0;
  int line = 1;
  int column = 1;
  bool valid = true;

  static SourceLocation invalid() {
    SourceLocation loc;
    loc.line = 0;
    loc.column = 0;
    loc.valid = false;
    return loc;
  }

  /// `file:line:column`, or `<unknown>` for synthetic positions.
  std::string toString(const std::string& file) const {
    if (!valid) return "<unknown>";
    return file + ":" + std::to_string(line) + ":" + std::to_string(column);
  }
};

}  // namespace bminor

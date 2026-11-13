#pragma once

#include <string>
#include <vector>

#include "bminor/ast.h"
#include "bminor/diagnostics.h"
#include "bminor/token.h"

namespace bminor {

/// Colour categories defined by Table 1 of the project description.
enum class HighlightClass {
  Keyword,
  TypeName,
  Variable,
  Function,
  Number,
  Text,      ///< string and character literals
  Boolean,
  Operator,
  Comment,
  Error,
};

struct ClassifiedToken {
  const Token* token;
  HighlightClass cls;
};

/// Assigns a colour category to every token.
///
/// Most categories follow directly from the token kind, but identifiers need
/// the AST: a name that is called or declared as a function is coloured
/// differently from a variable. That is the context-sensitive step a purely
/// regular-expression based highlighter cannot perform.
std::vector<ClassifiedToken> classifyTokens(const std::vector<Token>& tokens, const Node* ast);

/// Rebuilds the source exactly, wrapping each token in ANSI escape codes.
std::string renderAnsi(const std::string& source, const std::vector<ClassifiedToken>& tokens,
                       bool useColor);

/// Renders a standalone HTML document: one `<pre>` block with `<span
/// class="...">` elements and an embedded stylesheet (no JavaScript).
std::string renderHtml(const std::string& source, const std::vector<ClassifiedToken>& tokens,
                       const std::string& title, const DiagnosticBag& diagnostics);

}  // namespace bminor

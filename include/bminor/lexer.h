#pragma once

#include <string>
#include <vector>

#include "bminor/diagnostics.h"
#include "bminor/token.h"

namespace bminor {

/// Converts the raw character stream into a typed token stream.
///
/// The scanner is a hand written DFA: every state is one `case` in the scanner
/// loop, which implements maximal munch (the longest matching rule wins) and
/// keyword-before-identifier priority. Whitespace is skipped but still tracked
/// for line/column counting; comments are returned as `Comment` tokens because
/// the highlighter has to colour them. Lexical errors never stop the scan: an
/// `Invalid` token is emitted, a diagnostic is recorded and scanning resumes.
class Lexer {
 public:
  Lexer(std::string fileName, std::string source, DiagnosticBag& diagnostics);

  std::vector<Token> tokenize();

 private:
  bool atEnd() const { return pos_ >= source_.size(); }
  char peek(std::size_t lookahead = 0) const;
  char advance();
  void skipWhitespace();

  SourceLocation here() const;
  Token makeToken(TokenKind kind, std::size_t startOffset, SourceLocation startLoc);

  void scanIdentifierOrKeyword(std::vector<Token>& out);
  void scanNumber(std::vector<Token>& out);
  void scanString(std::vector<Token>& out);
  void scanCharLiteral(std::vector<Token>& out);
  void scanLineComment(std::vector<Token>& out);
  void scanBlockComment(std::vector<Token>& out);

  std::string fileName_;
  std::string source_;
  DiagnosticBag& diagnostics_;
  std::size_t pos_ = 0;
  int line_ = 1;
  int column_ = 1;
};

}  // namespace bminor

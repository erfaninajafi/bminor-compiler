#include "bminor/lexer.h"

#include <cctype>
#include <cstdio>

namespace bminor {

Lexer::Lexer(std::string fileName, std::string source, DiagnosticBag& diagnostics)
    : fileName_(std::move(fileName)), source_(std::move(source)), diagnostics_(diagnostics) {}

char Lexer::peek(std::size_t lookahead) const {
  const std::size_t index = pos_ + lookahead;
  return index < source_.size() ? source_[index] : '\0';
}

char Lexer::advance() {
  if (atEnd()) return '\0';
  const char c = source_[pos_++];
  if (c == '\n') {
    ++line_;
    column_ = 1;
  } else {
    ++column_;
  }
  return c;
}

SourceLocation Lexer::here() const {
  SourceLocation loc;
  loc.offset = pos_;
  loc.line = line_;
  loc.column = column_;
  loc.valid = true;
  return loc;
}

Token Lexer::makeToken(TokenKind kind, std::size_t startOffset, SourceLocation startLoc) {
  Token token;
  token.kind = kind;
  token.offset = startOffset;
  token.length = pos_ - startOffset;
  token.lexeme = source_.substr(startOffset, token.length);
  token.loc = startLoc;
  return token;
}

void Lexer::skipWhitespace() {
  while (!atEnd() && (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n' ||
                      peek() == '\f' || peek() == '\v')) {
    advance();
  }
}

void Lexer::scanIdentifierOrKeyword(std::vector<Token>& out) {
  const std::size_t start = pos_;
  const SourceLocation startLoc = here();
  while (!atEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
    advance();
  }
  Token token = makeToken(TokenKind::Identifier, start, startLoc);
  // Keyword-before-identifier priority: a reserved word is never an IDENT.
  token.kind = keywordKindFor(token.lexeme);
  out.push_back(std::move(token));
}

void Lexer::scanNumber(std::vector<Token>& out) {
  const std::size_t start = pos_;
  const SourceLocation startLoc = here();
  while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
  // A digit immediately followed by an identifier character is a malformed
  // literal such as `42abc`; keep maximal munch and report it once.
  if (!atEnd() && (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_')) {
    while (!atEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
      advance();
    }
    Token token = makeToken(TokenKind::Invalid, start, startLoc);
    diagnostics_.error(startLoc, "malformed integer literal '" + token.lexeme + "'");
    out.push_back(std::move(token));
    return;
  }
  out.push_back(makeToken(TokenKind::IntegerLiteral, start, startLoc));
}

void Lexer::scanString(std::vector<Token>& out) {
  const std::size_t start = pos_;
  const SourceLocation startLoc = here();
  advance();  // opening quote
  bool terminated = false;
  while (!atEnd()) {
    const char c = peek();
    if (c == '"') {
      advance();
      terminated = true;
      break;
    }
    if (c == '\n') break;  // unterminated: do not swallow the newline
    if (c == '\\') {
      advance();
      if (atEnd()) break;
      const char escape = peek();
      switch (escape) {
        case 'n': case 't': case 'r': case '0': case '\\':
        case '"': case '\'': case 'a': case 'b': case 'f': case 'v':
          advance();
          break;
        default:
          diagnostics_.warning(here(), std::string("unknown escape sequence '\\") + escape +
                                           "' in string literal");
          advance();
          break;
      }
      continue;
    }
    advance();
  }
  if (!terminated) {
    diagnostics_.error(startLoc, "unterminated string literal");
  }
  out.push_back(makeToken(TokenKind::StringLiteral, start, startLoc));
}

void Lexer::scanCharLiteral(std::vector<Token>& out) {
  const std::size_t start = pos_;
  const SourceLocation startLoc = here();
  advance();  // opening quote
  if (atEnd() || peek() == '\n') {
    diagnostics_.error(startLoc, "unterminated character literal");
    out.push_back(makeToken(TokenKind::CharLiteral, start, startLoc));
    return;
  }
  if (peek() == '\\') {
    advance();
    if (!atEnd()) advance();
  } else {
    if (peek() == '\'') {
      diagnostics_.error(startLoc, "empty character literal");
    }
    advance();
  }
  if (peek() == '\'') {
    advance();
    out.push_back(makeToken(TokenKind::CharLiteral, start, startLoc));
    return;
  }
  // Maximal munch for the malformed forms: take the rest of the literal up to
  // the closing quote (or end of line) and report it.
  while (!atEnd() && peek() != '\'' && peek() != '\n') advance();
  if (peek() == '\'') advance();
  diagnostics_.error(startLoc, "character literal must contain exactly one character");
  out.push_back(makeToken(TokenKind::Invalid, start, startLoc));
}

void Lexer::scanLineComment(std::vector<Token>& out) {
  const std::size_t start = pos_;
  const SourceLocation startLoc = here();
  while (!atEnd() && peek() != '\n') advance();
  out.push_back(makeToken(TokenKind::Comment, start, startLoc));
}

void Lexer::scanBlockComment(std::vector<Token>& out) {
  const std::size_t start = pos_;
  const SourceLocation startLoc = here();
  advance();  // '/'
  advance();  // '*'
  bool terminated = false;
  while (!atEnd()) {
    if (peek() == '*' && peek(1) == '/') {
      advance();
      advance();
      terminated = true;
      break;
    }
    advance();
  }
  if (!terminated) {
    diagnostics_.error(startLoc, "unterminated block comment");
  }
  out.push_back(makeToken(TokenKind::Comment, start, startLoc));
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> out;

  while (true) {
    skipWhitespace();
    if (atEnd()) break;

    const char c = peek();

    if (c == '/' && peek(1) == '/') {
      scanLineComment(out);
      continue;
    }
    if (c == '/' && peek(1) == '*') {
      scanBlockComment(out);
      continue;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      scanIdentifierOrKeyword(out);
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      scanNumber(out);
      continue;
    }
    if (c == '"') {
      scanString(out);
      continue;
    }
    if (c == '\'') {
      scanCharLiteral(out);
      continue;
    }

    const std::size_t start = pos_;
    const SourceLocation startLoc = here();

    // Two character operators first (maximal munch), then single characters.
    auto emit = [&](TokenKind kind, std::size_t width) {
      for (std::size_t i = 0; i < width; ++i) advance();
      out.push_back(makeToken(kind, start, startLoc));
    };

    const char next = peek(1);
    switch (c) {
      case '+':
        emit(next == '=' ? TokenKind::PlusAssign : TokenKind::Plus, next == '=' ? 2 : 1);
        continue;
      case '-':
        emit(next == '=' ? TokenKind::MinusAssign : TokenKind::Minus, next == '=' ? 2 : 1);
        continue;
      case '*':
        emit(next == '=' ? TokenKind::StarAssign : TokenKind::Star, next == '=' ? 2 : 1);
        continue;
      case '/':
        emit(next == '=' ? TokenKind::SlashAssign : TokenKind::Slash, next == '=' ? 2 : 1);
        continue;
      case '%':
        emit(next == '=' ? TokenKind::PercentAssign : TokenKind::Percent, next == '=' ? 2 : 1);
        continue;
      case '=':
        emit(next == '=' ? TokenKind::EqualEqual : TokenKind::Assign, next == '=' ? 2 : 1);
        continue;
      case '!':
        emit(next == '=' ? TokenKind::BangEqual : TokenKind::Bang, next == '=' ? 2 : 1);
        continue;
      case '<':
        emit(next == '=' ? TokenKind::LessEqual : TokenKind::Less, next == '=' ? 2 : 1);
        continue;
      case '>':
        emit(next == '=' ? TokenKind::GreaterEqual : TokenKind::Greater, next == '=' ? 2 : 1);
        continue;
      case '&':
        if (next == '&') {
          emit(TokenKind::AmpAmp, 2);
        } else {
          advance();
          diagnostics_.error(startLoc, "unexpected character '&'; did you mean '&&'?");
          out.push_back(makeToken(TokenKind::Invalid, start, startLoc));
        }
        continue;
      case '|':
        if (next == '|') {
          emit(TokenKind::PipePipe, 2);
        } else {
          advance();
          diagnostics_.error(startLoc, "unexpected character '|'; did you mean '||'?");
          out.push_back(makeToken(TokenKind::Invalid, start, startLoc));
        }
        continue;
      case '(':
        emit(TokenKind::LParen, 1);
        continue;
      case ')':
        emit(TokenKind::RParen, 1);
        continue;
      case '{':
        emit(TokenKind::LBrace, 1);
        continue;
      case '}':
        emit(TokenKind::RBrace, 1);
        continue;
      case '[':
        emit(TokenKind::LBracket, 1);
        continue;
      case ']':
        emit(TokenKind::RBracket, 1);
        continue;
      case ',':
        emit(TokenKind::Comma, 1);
        continue;
      case ';':
        emit(TokenKind::Semicolon, 1);
        continue;
      case ':':
        emit(TokenKind::Colon, 1);
        continue;
      default:
        break;
    }

    // Unknown character: report, skip it and keep scanning (never abort).
    advance();
    std::string shown(1, c);
    if (static_cast<unsigned char>(c) < 0x20) {
      char buffer[8];
      std::snprintf(buffer, sizeof buffer, "\\x%02X", static_cast<unsigned char>(c));
      shown = buffer;
    }
    diagnostics_.error(startLoc, "unexpected character '" + shown + "' in input");
    out.push_back(makeToken(TokenKind::Invalid, start, startLoc));
  }

  Token eof;
  eof.kind = TokenKind::EndOfFile;
  eof.offset = pos_;
  eof.length = 0;
  eof.loc = here();
  out.push_back(eof);
  return out;
}

}  // namespace bminor

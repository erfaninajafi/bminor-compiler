#pragma once

#include <string>

#include "bminor/source.h"

namespace bminor {

/// Every token category of B-Minor (see docs/tokens.md for the formal
/// regular expression of each one and the priority rules between them).
enum class TokenKind {
  // Keywords (checked before identifiers: keyword-before-identifier rule).
  KwArray,
  KwBoolean,
  KwChar,
  KwElse,
  KwFalse,
  KwFor,
  KwFunction,
  KwIf,
  KwInteger,
  KwPrint,
  KwReturn,
  KwString,
  KwTrue,
  KwVar,
  KwVoid,
  KwWhile,

  Identifier,
  IntegerLiteral,
  StringLiteral,
  CharLiteral,

  // Operators.
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Assign,
  PlusAssign,
  MinusAssign,
  StarAssign,
  SlashAssign,
  PercentAssign,
  EqualEqual,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  AmpAmp,
  PipePipe,
  Bang,

  // Delimiters.
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Semicolon,
  Colon,

  Comment,
  Whitespace,
  Invalid,
  EndOfFile,
};

/// A token produced by the lexer.
///
/// Besides the three pieces of information the specification requires (type,
/// lexeme, source location) the token keeps `offset`/`length` so later stages
/// can slice the exact original text out of the buffer.
struct Token {
  TokenKind kind = TokenKind::Invalid;
  std::string lexeme;
  SourceLocation loc;
  std::size_t offset = 0;
  std::size_t length = 0;

  std::size_t end() const { return offset + length; }
};

const char* tokenKindName(TokenKind kind);

bool isKeyword(TokenKind kind);
/// `array`, `boolean`, `char`, `integer`, `string`, `void`: used as type names.
bool isTypeKeyword(TokenKind kind);
/// `true`, `false`.
bool isBooleanLiteralKeyword(TokenKind kind);
bool isOperator(TokenKind kind);
bool isDelimiter(TokenKind kind);

/// Returns the keyword kind for `word`, or `TokenKind::Identifier` when the
/// word is not reserved.
TokenKind keywordKindFor(const std::string& word);

}  // namespace bminor

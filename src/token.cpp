#include "bminor/token.h"

#include <unordered_map>

namespace bminor {
namespace {

const std::unordered_map<std::string, TokenKind>& keywords() {
  static const std::unordered_map<std::string, TokenKind> table = {
      {"array", TokenKind::KwArray},     {"boolean", TokenKind::KwBoolean},
      {"char", TokenKind::KwChar},       {"else", TokenKind::KwElse},
      {"false", TokenKind::KwFalse},     {"for", TokenKind::KwFor},
      {"function", TokenKind::KwFunction}, {"if", TokenKind::KwIf},
      {"integer", TokenKind::KwInteger}, {"print", TokenKind::KwPrint},
      {"return", TokenKind::KwReturn},   {"string", TokenKind::KwString},
      {"true", TokenKind::KwTrue},       {"var", TokenKind::KwVar},
      {"void", TokenKind::KwVoid},
      {"while", TokenKind::KwWhile},
  };
  return table;
}

}  // namespace

const char* tokenKindName(TokenKind kind) {
  switch (kind) {
    case TokenKind::KwArray: return "KEYWORD(array)";
    case TokenKind::KwBoolean: return "KEYWORD(boolean)";
    case TokenKind::KwChar: return "KEYWORD(char)";
    case TokenKind::KwElse: return "KEYWORD(else)";
    case TokenKind::KwFalse: return "KEYWORD(false)";
    case TokenKind::KwFor: return "KEYWORD(for)";
    case TokenKind::KwFunction: return "KEYWORD(function)";
    case TokenKind::KwIf: return "KEYWORD(if)";
    case TokenKind::KwInteger: return "KEYWORD(integer)";
    case TokenKind::KwPrint: return "KEYWORD(print)";
    case TokenKind::KwReturn: return "KEYWORD(return)";
    case TokenKind::KwString: return "KEYWORD(string)";
    case TokenKind::KwTrue: return "KEYWORD(true)";
    case TokenKind::KwVar: return "KEYWORD(var)";
    case TokenKind::KwVoid: return "KEYWORD(void)";
    case TokenKind::KwWhile: return "KEYWORD(while)";
    case TokenKind::Identifier: return "IDENT";
    case TokenKind::IntegerLiteral: return "INT_LITERAL";
    case TokenKind::StringLiteral: return "STRING_LITERAL";
    case TokenKind::CharLiteral: return "CHAR_LITERAL";
    case TokenKind::Plus: return "OP(+)";
    case TokenKind::Minus: return "OP(-)";
    case TokenKind::Star: return "OP(*)";
    case TokenKind::Slash: return "OP(/)";
    case TokenKind::Percent: return "OP(%)";
    case TokenKind::Assign: return "OP(=)";
    case TokenKind::PlusAssign: return "OP(+=)";
    case TokenKind::MinusAssign: return "OP(-=)";
    case TokenKind::StarAssign: return "OP(*=)";
    case TokenKind::SlashAssign: return "OP(/=)";
    case TokenKind::PercentAssign: return "OP(%=)";
    case TokenKind::EqualEqual: return "OP(==)";
    case TokenKind::BangEqual: return "OP(!=)";
    case TokenKind::Less: return "OP(<)";
    case TokenKind::LessEqual: return "OP(<=)";
    case TokenKind::Greater: return "OP(>)";
    case TokenKind::GreaterEqual: return "OP(>=)";
    case TokenKind::AmpAmp: return "OP(&&)";
    case TokenKind::PipePipe: return "OP(||)";
    case TokenKind::Bang: return "OP(!)";
    case TokenKind::LParen: return "DELIM(()";
    case TokenKind::RParen: return "DELIM())";
    case TokenKind::LBrace: return "DELIM({)";
    case TokenKind::RBrace: return "DELIM(})";
    case TokenKind::LBracket: return "DELIM([)";
    case TokenKind::RBracket: return "DELIM(])";
    case TokenKind::Comma: return "DELIM(,)";
    case TokenKind::Semicolon: return "DELIM(;)";
    case TokenKind::Colon: return "DELIM(:)";
    case TokenKind::Comment: return "COMMENT";
    case TokenKind::Whitespace: return "WHITESPACE";
    case TokenKind::Invalid: return "INVALID";
    case TokenKind::EndOfFile: return "EOF";
  }
  return "UNKNOWN";
}

bool isKeyword(TokenKind kind) {
  return kind >= TokenKind::KwArray && kind <= TokenKind::KwWhile;
}

bool isTypeKeyword(TokenKind kind) {
  switch (kind) {
    case TokenKind::KwArray:
    case TokenKind::KwBoolean:
    case TokenKind::KwChar:
    case TokenKind::KwInteger:
    case TokenKind::KwString:
    case TokenKind::KwVoid:
      return true;
    default:
      return false;
  }
}

bool isBooleanLiteralKeyword(TokenKind kind) {
  return kind == TokenKind::KwTrue || kind == TokenKind::KwFalse;
}

bool isOperator(TokenKind kind) {
  return kind >= TokenKind::Plus && kind <= TokenKind::Bang;
}

bool isDelimiter(TokenKind kind) {
  return kind >= TokenKind::LParen && kind <= TokenKind::Colon;
}

TokenKind keywordKindFor(const std::string& word) {
  const auto& table = keywords();
  auto it = table.find(word);
  return it == table.end() ? TokenKind::Identifier : it->second;
}

}  // namespace bminor

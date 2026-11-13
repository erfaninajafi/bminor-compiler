#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "bminor/ast.h"
#include "bminor/diagnostics.h"
#include "bminor/token.h"

namespace bminor {

/// Recursive-descent (LL(1)) parser for the B-Minor grammar documented in
/// docs/grammar.md.
///
/// The grammar handed to this parser is free of left recursion and of
/// FIRST/FOLLOW conflicts, so a single token of lookahead is enough. On a
/// syntax error the parser reports a diagnostic and switches to panic-mode
/// recovery: it discards tokens until a synchronization point (`;`, `}` or the
/// beginning of a statement) and then continues, so one bad statement does not
/// hide the rest of the file.
class Parser {
 public:
  Parser(const std::vector<Token>& tokens, DiagnosticBag& diagnostics);

  /// Parses a whole translation unit. Never returns nullptr: a partially
  /// recovered tree is produced even for badly broken input.
  std::unique_ptr<Program> parseProgram();

 private:
  const Token& peek(std::size_t lookahead = 0) const;
  const Token& previous() const;
  bool check(TokenKind kind) const;
  bool checkAny(std::initializer_list<TokenKind> kinds) const;
  const Token& advance();
  bool match(TokenKind kind);
  const Token& expect(TokenKind kind, const std::string& message);

  void errorAt(const Token& token, const std::string& message);
  void synchronize();
  static bool isSynchronizationPoint(TokenKind kind);

  DeclPtr parseTopLevelDecl();
  DeclPtr parseFunctionDecl();
  DeclPtr parseVarDecl();
  TypeSpecPtr parseTypeSpec();
  std::unique_ptr<Block> parseBlock();
  NodePtr parseBlockItem();
  StmtPtr parseStatement();

  StmtPtr parseExprStmt();
  StmtPtr parseIfStmt();
  StmtPtr parseWhileStmt();
  StmtPtr parseForStmt();
  StmtPtr parseReturnStmt();
  StmtPtr parsePrintStmt();

  ExprPtr parseExpression();
  ExprPtr parseAssignment();
  ExprPtr parseLogicalOr();
  ExprPtr parseLogicalAnd();
  ExprPtr parseEquality();
  ExprPtr parseRelational();
  ExprPtr parseAdditive();
  ExprPtr parseMultiplicative();
  ExprPtr parseUnary();
  ExprPtr parsePostfix();
  ExprPtr parsePrimary();
  std::vector<ExprPtr> parseArgumentList();

  static ExprPtr makeBinary(TokenKind op, Token opToken, ExprPtr lhs, ExprPtr rhs);

  std::vector<Token> tokens_;  ///< comments/whitespace already removed
  DiagnosticBag& diagnostics_;
  std::size_t index_ = 0;
  bool panicMode_ = false;
  Token synthetic_;  ///< storage for tokens invented by error recovery
};

}  // namespace bminor

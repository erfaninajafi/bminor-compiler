#include "bminor/parser.h"

#include <cstdlib>

namespace bminor {
namespace {

std::string describeToken(const Token& token) {
  if (token.kind == TokenKind::EndOfFile) return "end of file";
  return "'" + token.lexeme + "'";
}

Token makeSyntheticToken(TokenKind kind, SourceLocation loc) {
  Token token;
  token.kind = kind;
  token.lexeme = "<missing>";
  token.offset = loc.offset;
  token.length = 0;
  token.loc = SourceLocation::invalid();
  return token;
}

/// Placeholder expression used when a primary expression is unparsable. The
/// node keeps the tree well formed so later phases and the highlighter can keep
/// working after a syntax error.
ExprPtr makeErrorExpr(SourceLocation loc) {
  auto expr = std::make_unique<IdentifierExpr>(loc);
  expr->name.kind = TokenKind::Invalid;
  expr->name.lexeme = "<error>";
  expr->name.offset = loc.offset;
  expr->name.length = 0;
  expr->name.loc = loc;
  return expr;
}

}  // namespace

Parser::Parser(const std::vector<Token>& tokens, DiagnosticBag& diagnostics)
    : diagnostics_(diagnostics) {
  tokens_.reserve(tokens.size());
  for (const Token& token : tokens) {
    if (token.kind == TokenKind::Comment || token.kind == TokenKind::Whitespace) continue;
    tokens_.push_back(token);
  }
  if (tokens_.empty() || tokens_.back().kind != TokenKind::EndOfFile) {
    Token eof;
    eof.kind = TokenKind::EndOfFile;
    eof.lexeme = "";
    tokens_.push_back(eof);
  }
}

const Token& Parser::peek(std::size_t lookahead) const {
  const std::size_t index = index_ + lookahead;
  return index < tokens_.size() ? tokens_[index] : tokens_.back();
}

const Token& Parser::previous() const { return index_ == 0 ? tokens_.front() : tokens_[index_ - 1]; }

bool Parser::check(TokenKind kind) const { return peek().kind == kind; }

bool Parser::checkAny(std::initializer_list<TokenKind> kinds) const {
  for (TokenKind kind : kinds) {
    if (check(kind)) return true;
  }
  return false;
}

const Token& Parser::advance() {
  const Token& token = tokens_[index_];
  if (index_ + 1 < tokens_.size()) ++index_;
  return token;
}

bool Parser::match(TokenKind kind) {
  if (!check(kind)) return false;
  advance();
  return true;
}

const Token& Parser::expect(TokenKind kind, const std::string& message) {
  if (check(kind)) return advance();
  errorAt(peek(), message);
  synthetic_ = makeSyntheticToken(kind, peek().loc);
  return synthetic_;
}

void Parser::errorAt(const Token& token, const std::string& message) {
  if (panicMode_) return;  // report the first error of a statement only
  panicMode_ = true;
  diagnostics_.error(token.loc, message + ", found " + describeToken(token));
}

bool Parser::isSynchronizationPoint(TokenKind kind) {
  switch (kind) {
    case TokenKind::Semicolon:
    case TokenKind::RBrace:
    case TokenKind::KwFunction:
    case TokenKind::KwVar:
    case TokenKind::KwIf:
    case TokenKind::KwWhile:
    case TokenKind::KwFor:
    case TokenKind::KwReturn:
    case TokenKind::KwPrint:
      return true;
    default:
      return false;
  }
}

void Parser::synchronize() {
  while (!check(TokenKind::EndOfFile)) {
    if (check(TokenKind::Semicolon)) {
      advance();
      return;
    }
    if (check(TokenKind::RBrace)) return;  // let the enclosing block close itself
    if (isSynchronizationPoint(peek().kind)) return;
    advance();
  }
}

std::unique_ptr<Program> Parser::parseProgram() {
  auto program = std::make_unique<Program>(peek().loc);
  while (!check(TokenKind::EndOfFile)) {
    panicMode_ = false;
    const std::size_t start = index_;
    DeclPtr decl = parseTopLevelDecl();
    if (decl) program->declarations.push_back(std::move(decl));
    if (index_ == start) advance();  // guarantee progress even after recovery
  }
  return program;
}

DeclPtr Parser::parseTopLevelDecl() {
  if (check(TokenKind::KwFunction)) return parseFunctionDecl();
  if (check(TokenKind::KwVar)) return parseVarDecl();
  errorAt(peek(), "expected 'function' or 'var' at top level");
  synchronize();
  return nullptr;
}

DeclPtr Parser::parseFunctionDecl() {
  const Token& keyword = advance();  // 'function'
  auto decl = std::make_unique<FunctionDecl>(keyword.loc);
  decl->name = expect(TokenKind::Identifier, "expected a function name after 'function'");

  expect(TokenKind::LParen, "expected '(' after the function name");
  if (!check(TokenKind::RParen)) {
    while (true) {
      panicMode_ = false;
      auto param = std::make_unique<Param>(peek().loc);
      param->name = expect(TokenKind::Identifier, "expected a parameter name");
      expect(TokenKind::Colon, "expected ':' after the parameter name");
      param->type = parseTypeSpec();
      decl->params.push_back(std::move(param));
      if (!match(TokenKind::Comma)) break;
    }
  }
  expect(TokenKind::RParen, "expected ')' after the parameter list");

  if (match(TokenKind::Colon)) {
    decl->returnType = parseTypeSpec();
  } else {
    // An omitted result type means `void`.
    auto type = std::make_unique<TypeSpec>(decl->name.loc);
    type->keyword = makeSyntheticToken(TokenKind::KwVoid, decl->name.loc);
    type->keyword.lexeme = "void";
    type->keyword.kind = TokenKind::KwVoid;
    decl->returnType = std::move(type);
  }

  decl->body = parseBlock();
  return decl;
}

DeclPtr Parser::parseVarDecl() {
  const Token& keyword = advance();  // 'var'
  auto decl = std::make_unique<VarDecl>(keyword.loc);
  decl->name = expect(TokenKind::Identifier, "expected a variable name after 'var'");
  expect(TokenKind::Colon, "expected ':' after the variable name");
  decl->type = parseTypeSpec();
  if (match(TokenKind::Assign)) {
    decl->initializer = parseExpression();
  }
  if (!match(TokenKind::Semicolon)) {
    errorAt(peek(), "expected ';' after the variable declaration");
    synchronize();
  }
  return decl;
}

TypeSpecPtr Parser::parseTypeSpec() {
  if (isTypeKeyword(peek().kind)) {
    const Token& keyword = advance();
    auto type = std::make_unique<TypeSpec>(keyword.loc);
    type->keyword = keyword;
    if (keyword.kind == TokenKind::KwArray && match(TokenKind::LBracket)) {
      type->element = parseTypeSpec();
      expect(TokenKind::RBracket, "expected ']' after the array element type");
    }
    return type;
  }
  errorAt(peek(), "expected a type name (integer, boolean, char, string, void, array)");
  synchronize();
  auto type = std::make_unique<TypeSpec>(peek().loc);
  type->keyword = makeSyntheticToken(TokenKind::KwVoid, peek().loc);
  return type;
}

std::unique_ptr<Block> Parser::parseBlock() {
  auto block = std::make_unique<Block>(peek().loc);
  if (!check(TokenKind::LBrace)) {
    errorAt(peek(), "expected '{' to begin a block");
    synchronize();
    if (!check(TokenKind::LBrace)) return block;
  }
  advance();  // '{'

  while (!check(TokenKind::RBrace) && !check(TokenKind::EndOfFile)) {
    panicMode_ = false;
    const std::size_t start = index_;
    NodePtr item = parseBlockItem();
    if (item) block->items.push_back(std::move(item));
    if (index_ == start) advance();  // guarantee progress
  }

  if (!match(TokenKind::RBrace)) {
    errorAt(peek(), "expected '}' to close the block");
  }
  return block;
}

NodePtr Parser::parseBlockItem() {
  if (check(TokenKind::KwVar)) return parseVarDecl();
  return parseStatement();
}

StmtPtr Parser::parseStatement() {
  switch (peek().kind) {
    case TokenKind::KwIf:
      return parseIfStmt();
    case TokenKind::KwWhile:
      return parseWhileStmt();
    case TokenKind::KwFor:
      return parseForStmt();
    case TokenKind::KwReturn:
      return parseReturnStmt();
    case TokenKind::KwPrint:
      return parsePrintStmt();
    case TokenKind::LBrace:
      return parseBlock();
    case TokenKind::Semicolon: {
      const Token& token = advance();
      return std::make_unique<EmptyStmt>(token.loc);
    }
    default:
      return parseExprStmt();
  }
}

StmtPtr Parser::parseExprStmt() {
  auto stmt = std::make_unique<ExprStmt>(peek().loc);
  stmt->expr = parseExpression();
  if (!match(TokenKind::Semicolon)) {
    errorAt(peek(), "expected ';' after the expression");
    synchronize();  // statement level recovery: one bad token, one message
  }
  return stmt;
}

StmtPtr Parser::parseIfStmt() {
  const Token& keyword = advance();  // 'if'
  auto stmt = std::make_unique<IfStmt>(keyword.loc);
  expect(TokenKind::LParen, "expected '(' after 'if'");
  stmt->condition = parseExpression();
  expect(TokenKind::RParen, "expected ')' after the condition");
  stmt->thenBranch = parseStatement();
  if (match(TokenKind::KwElse)) {
    stmt->elseBranch = parseStatement();
  }
  return stmt;
}

StmtPtr Parser::parseWhileStmt() {
  const Token& keyword = advance();  // 'while'
  auto stmt = std::make_unique<WhileStmt>(keyword.loc);
  expect(TokenKind::LParen, "expected '(' after 'while'");
  stmt->condition = parseExpression();
  expect(TokenKind::RParen, "expected ')' after the condition");
  stmt->body = parseStatement();
  return stmt;
}

StmtPtr Parser::parseForStmt() {
  const Token& keyword = advance();  // 'for'
  auto stmt = std::make_unique<ForStmt>(keyword.loc);
  expect(TokenKind::LParen, "expected '(' after 'for'");
  if (!check(TokenKind::Semicolon)) stmt->init = parseExpression();
  expect(TokenKind::Semicolon, "expected ';' after the for initializer");
  if (!check(TokenKind::Semicolon)) stmt->condition = parseExpression();
  expect(TokenKind::Semicolon, "expected ';' after the for condition");
  if (!check(TokenKind::RParen)) stmt->step = parseExpression();
  expect(TokenKind::RParen, "expected ')' after the for clauses");
  stmt->body = parseStatement();
  return stmt;
}

StmtPtr Parser::parseReturnStmt() {
  const Token& keyword = advance();  // 'return'
  auto stmt = std::make_unique<ReturnStmt>(keyword.loc);
  if (!check(TokenKind::Semicolon) && !check(TokenKind::RBrace)) {
    stmt->value = parseExpression();
  }
  if (!match(TokenKind::Semicolon)) {
    errorAt(peek(), "expected ';' after the return statement");
    synchronize();
  }
  return stmt;
}

StmtPtr Parser::parsePrintStmt() {
  const Token& keyword = advance();  // 'print'
  auto stmt = std::make_unique<PrintStmt>(keyword.loc);
  if (!check(TokenKind::Semicolon)) {
    stmt->arguments.push_back(parseExpression());
    while (match(TokenKind::Comma)) {
      stmt->arguments.push_back(parseExpression());
    }
  }
  if (!match(TokenKind::Semicolon)) {
    errorAt(peek(), "expected ';' after the print statement");
    synchronize();
  }
  return stmt;
}

ExprPtr Parser::parseExpression() { return parseAssignment(); }

ExprPtr Parser::parseAssignment() {
  ExprPtr target = parseLogicalOr();
  if (checkAny({TokenKind::Assign, TokenKind::PlusAssign, TokenKind::MinusAssign,
                TokenKind::StarAssign, TokenKind::SlashAssign, TokenKind::PercentAssign})) {
    const Token& op = advance();
    auto expr = std::make_unique<AssignmentExpr>(target->loc);
    expr->op = op;
    expr->target = std::move(target);
    expr->value = parseAssignment();  // right associative
    return expr;
  }
  return target;
}

ExprPtr Parser::makeBinary(TokenKind op, Token opToken, ExprPtr lhs, ExprPtr rhs) {
  const SourceLocation loc = lhs ? lhs->loc : opToken.loc;
  auto expr = std::make_unique<BinaryExpr>(loc);
  expr->op = op;
  expr->opToken = std::move(opToken);
  expr->lhs = std::move(lhs);
  expr->rhs = std::move(rhs);
  return expr;
}

ExprPtr Parser::parseLogicalOr() {
  ExprPtr expr = parseLogicalAnd();
  while (check(TokenKind::PipePipe)) {
    const Token& op = advance();
    expr = makeBinary(op.kind, op, std::move(expr), parseLogicalAnd());
  }
  return expr;
}

ExprPtr Parser::parseLogicalAnd() {
  ExprPtr expr = parseEquality();
  while (check(TokenKind::AmpAmp)) {
    const Token& op = advance();
    expr = makeBinary(op.kind, op, std::move(expr), parseEquality());
  }
  return expr;
}

ExprPtr Parser::parseEquality() {
  ExprPtr expr = parseRelational();
  while (checkAny({TokenKind::EqualEqual, TokenKind::BangEqual})) {
    const Token& op = advance();
    expr = makeBinary(op.kind, op, std::move(expr), parseRelational());
  }
  return expr;
}

ExprPtr Parser::parseRelational() {
  ExprPtr expr = parseAdditive();
  while (checkAny({TokenKind::Less, TokenKind::LessEqual, TokenKind::Greater,
                   TokenKind::GreaterEqual})) {
    const Token& op = advance();
    expr = makeBinary(op.kind, op, std::move(expr), parseAdditive());
  }
  return expr;
}

ExprPtr Parser::parseAdditive() {
  ExprPtr expr = parseMultiplicative();
  while (checkAny({TokenKind::Plus, TokenKind::Minus})) {
    const Token& op = advance();
    expr = makeBinary(op.kind, op, std::move(expr), parseMultiplicative());
  }
  return expr;
}

ExprPtr Parser::parseMultiplicative() {
  ExprPtr expr = parseUnary();
  while (checkAny({TokenKind::Star, TokenKind::Slash, TokenKind::Percent})) {
    const Token& op = advance();
    expr = makeBinary(op.kind, op, std::move(expr), parseUnary());
  }
  return expr;
}

ExprPtr Parser::parseUnary() {
  if (checkAny({TokenKind::Bang, TokenKind::Minus, TokenKind::Plus})) {
    const Token& op = advance();
    ExprPtr operand = parseUnary();
    // `-42` becomes a single signed integer literal node (see docs/tokens.md).
    if (op.kind == TokenKind::Minus && operand && operand->kind == NodeKind::IntLiteral) {
      auto* literal = static_cast<IntLiteral*>(operand.get());
      literal->isNegative = true;
      literal->value = -literal->value;
      literal->loc = op.loc;
      return operand;
    }
    auto expr = std::make_unique<UnaryExpr>(op.loc);
    expr->op = op.kind;
    expr->opToken = op;
    expr->operand = std::move(operand);
    return expr;
  }
  return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
  ExprPtr expr = parsePrimary();
  while (true) {
    if (check(TokenKind::LBracket)) {
      const Token& bracket = advance();
      auto indexExpr = std::make_unique<IndexExpr>(expr->loc);
      indexExpr->base = std::move(expr);
      indexExpr->index = parseExpression();
      expect(TokenKind::RBracket, "expected ']' after the index expression");
      expr = std::move(indexExpr);
      (void)bracket;
      continue;
    }
    if (check(TokenKind::LParen)) {
      const Token& paren = advance();
      auto call = std::make_unique<CallExpr>(expr->loc);
      call->callee = std::move(expr);
      if (!check(TokenKind::RParen)) {
        call->arguments = parseArgumentList();
      }
      expect(TokenKind::RParen, "expected ')' after the argument list");
      expr = std::move(call);
      (void)paren;
      continue;
    }
    break;
  }
  return expr;
}

std::vector<ExprPtr> Parser::parseArgumentList() {
  std::vector<ExprPtr> arguments;
  arguments.push_back(parseExpression());
  while (match(TokenKind::Comma)) {
    arguments.push_back(parseExpression());
  }
  return arguments;
}

ExprPtr Parser::parsePrimary() {
  const Token& token = peek();
  switch (token.kind) {
    case TokenKind::IntegerLiteral: {
      advance();
      auto literal = std::make_unique<IntLiteral>(token.loc);
      literal->value = std::strtoll(token.lexeme.c_str(), nullptr, 10);
      return literal;
    }
    case TokenKind::StringLiteral: {
      advance();
      auto literal = std::make_unique<StringLiteral>(token.loc);
      literal->value = decodeStringLiteral(token.lexeme);
      return literal;
    }
    case TokenKind::CharLiteral: {
      advance();
      auto literal = std::make_unique<CharLiteral>(token.loc);
      literal->value = decodeCharLiteral(token.lexeme);
      return literal;
    }
    case TokenKind::KwTrue:
    case TokenKind::KwFalse: {
      advance();
      auto literal = std::make_unique<BoolLiteral>(token.loc);
      literal->value = token.kind == TokenKind::KwTrue;
      return literal;
    }
    case TokenKind::Identifier: {
      advance();
      auto identifier = std::make_unique<IdentifierExpr>(token.loc);
      identifier->name = token;
      return identifier;
    }
    case TokenKind::LParen: {
      advance();
      ExprPtr inner = parseExpression();
      expect(TokenKind::RParen, "expected ')' after the expression");
      return inner;
    }
    default:
      break;
  }
  errorAt(token, "expected an expression");
  synchronize();
  return makeErrorExpr(token.loc);
}

}  // namespace bminor

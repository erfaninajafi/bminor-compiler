#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "bminor/source.h"
#include "bminor/token.h"

namespace bminor {

enum class NodeKind {
  Program,
  FunctionDecl,
  Param,
  VarDecl,
  TypeSpec,
  Block,
  ExprStmt,
  IfStmt,
  WhileStmt,
  ForStmt,
  ReturnStmt,
  PrintStmt,
  EmptyStmt,
  AssignmentExpr,
  BinaryExpr,
  UnaryExpr,
  CallExpr,
  IndexExpr,
  IntLiteral,
  StringLiteral,
  CharLiteral,
  BoolLiteral,
  IdentifierExpr,
};

const char* nodeKindName(NodeKind kind);

/// Base class of every AST node.
///
/// `loc` points at the first token of the construct and `typeAnnotation` is
/// empty at construction time: it is reserved for the semantic analysis phase
/// (phase 2), which will fill it with the resolved type of the node.
struct Node {
  NodeKind kind;
  SourceLocation loc;
  std::optional<std::string> typeAnnotation;

  Node(NodeKind k, SourceLocation l) : kind(k), loc(std::move(l)) {}
  virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

struct Expr : Node {
  using Node::Node;
};
using ExprPtr = std::unique_ptr<Expr>;

struct Stmt : Node {
  using Node::Node;
};
using StmtPtr = std::unique_ptr<Stmt>;

struct Decl : Node {
  using Node::Node;
};
using DeclPtr = std::unique_ptr<Decl>;

struct TypeSpec;
using TypeSpecPtr = std::unique_ptr<TypeSpec>;

/// `integer`, `boolean`, `char`, `string`, `void` or `array[<element>]`.
struct TypeSpec : Node {
  Token keyword;                 ///< the type keyword token (for highlighting)
  TypeSpecPtr element;           ///< element type, only for `array`
  explicit TypeSpec(SourceLocation loc_) : Node(NodeKind::TypeSpec, std::move(loc_)) {}
};

struct Param : Node {
  Token name;
  TypeSpecPtr type;
  explicit Param(SourceLocation loc_) : Node(NodeKind::Param, std::move(loc_)) {}
};

struct Program : Node {
  std::vector<DeclPtr> declarations;
  explicit Program(SourceLocation loc_) : Node(NodeKind::Program, std::move(loc_)) {}
};

struct FunctionDecl : Decl {
  Token name;
  std::vector<std::unique_ptr<Param>> params;
  TypeSpecPtr returnType;
  std::unique_ptr<Node> body;  ///< always a Block in this phase
  explicit FunctionDecl(SourceLocation loc_) : Decl(NodeKind::FunctionDecl, std::move(loc_)) {}
};

struct VarDecl : Decl {
  Token name;
  TypeSpecPtr type;
  ExprPtr initializer;  ///< may be null
  explicit VarDecl(SourceLocation loc_) : Decl(NodeKind::VarDecl, std::move(loc_)) {}
};

/// A block mixes declarations and statements in source order, so it stores
/// them as a single child list. It is a statement itself, which keeps the
/// `statement ::= ... | block` production of the grammar trivial.
struct Block : Stmt {
  std::vector<NodePtr> items;
  explicit Block(SourceLocation loc_) : Stmt(NodeKind::Block, std::move(loc_)) {}
};

struct ExprStmt : Stmt {
  ExprPtr expr;
  explicit ExprStmt(SourceLocation loc_) : Stmt(NodeKind::ExprStmt, std::move(loc_)) {}
};

struct IfStmt : Stmt {
  ExprPtr condition;
  StmtPtr thenBranch;
  StmtPtr elseBranch;  ///< may be null
  explicit IfStmt(SourceLocation loc_) : Stmt(NodeKind::IfStmt, std::move(loc_)) {}
};

struct WhileStmt : Stmt {
  ExprPtr condition;
  StmtPtr body;
  explicit WhileStmt(SourceLocation loc_) : Stmt(NodeKind::WhileStmt, std::move(loc_)) {}
};

struct ForStmt : Stmt {
  ExprPtr init;
  ExprPtr condition;
  ExprPtr step;
  StmtPtr body;
  explicit ForStmt(SourceLocation loc_) : Stmt(NodeKind::ForStmt, std::move(loc_)) {}
};

struct ReturnStmt : Stmt {
  ExprPtr value;  ///< may be null (`return;`)
  explicit ReturnStmt(SourceLocation loc_) : Stmt(NodeKind::ReturnStmt, std::move(loc_)) {}
};

struct PrintStmt : Stmt {
  std::vector<ExprPtr> arguments;
  explicit PrintStmt(SourceLocation loc_) : Stmt(NodeKind::PrintStmt, std::move(loc_)) {}
};

struct EmptyStmt : Stmt {
  explicit EmptyStmt(SourceLocation loc_) : Stmt(NodeKind::EmptyStmt, std::move(loc_)) {}
};

struct AssignmentExpr : Expr {
  Token op;  ///< `=`, `+=`, `-=`, `*=`, `/=`, `%=`
  ExprPtr target;
  ExprPtr value;
  explicit AssignmentExpr(SourceLocation loc_) : Expr(NodeKind::AssignmentExpr, std::move(loc_)) {}
};

struct BinaryExpr : Expr {
  TokenKind op;  ///< the operator token kind
  Token opToken;
  ExprPtr lhs;
  ExprPtr rhs;
  explicit BinaryExpr(SourceLocation loc_) : Expr(NodeKind::BinaryExpr, std::move(loc_)) {}
};

struct UnaryExpr : Expr {
  TokenKind op;
  Token opToken;
  ExprPtr operand;
  explicit UnaryExpr(SourceLocation loc_) : Expr(NodeKind::UnaryExpr, std::move(loc_)) {}
};

struct CallExpr : Expr {
  ExprPtr callee;  ///< usually an IdentifierExpr
  std::vector<ExprPtr> arguments;
  explicit CallExpr(SourceLocation loc_) : Expr(NodeKind::CallExpr, std::move(loc_)) {}
};

struct IndexExpr : Expr {
  ExprPtr base;
  ExprPtr index;
  explicit IndexExpr(SourceLocation loc_) : Expr(NodeKind::IndexExpr, std::move(loc_)) {}
};

struct IntLiteral : Expr {
  long long value = 0;
  bool isNegative = false;  ///< set when a unary minus folds into the literal
  explicit IntLiteral(SourceLocation loc_) : Expr(NodeKind::IntLiteral, std::move(loc_)) {}
};

struct StringLiteral : Expr {
  std::string value;  ///< unescaped contents
  explicit StringLiteral(SourceLocation loc_) : Expr(NodeKind::StringLiteral, std::move(loc_)) {}
};

struct CharLiteral : Expr {
  int value = 0;  ///< the decoded character code
  explicit CharLiteral(SourceLocation loc_) : Expr(NodeKind::CharLiteral, std::move(loc_)) {}
};

struct BoolLiteral : Expr {
  bool value = false;
  explicit BoolLiteral(SourceLocation loc_) : Expr(NodeKind::BoolLiteral, std::move(loc_)) {}
};

struct IdentifierExpr : Expr {
  Token name;
  explicit IdentifierExpr(SourceLocation loc_) : Expr(NodeKind::IdentifierExpr, std::move(loc_)) {}
};

/// Calls `fn` for every direct child of `node` (pre-order friendly helper used
/// by the AST printer, the highlighter and future analyses).
void forEachChild(const Node& node, const std::function<void(const Node&)>& fn);

/// Human readable tree dump, used by `bminor --ast`.
void dumpAst(const Node& node, std::ostream& out, int indent = 0);

/// Byte offsets of identifiers that denote functions: the name of every
/// `FunctionDecl` and the callee of every `CallExpr`. The highlighter uses this
/// to distinguish function names from variable identifiers, which a
/// token-stream-only highlighter cannot do.
std::unordered_set<std::size_t> collectFunctionNameOffsets(const Node& root);

/// Decodes a B-Minor string literal (the lexeme, quotes included).
std::string decodeStringLiteral(const std::string& lexeme);
/// Decodes a B-Minor character literal (the lexeme, quotes included).
int decodeCharLiteral(const std::string& lexeme);

}  // namespace bminor

#include "bminor/ast.h"

#include <ostream>
#include <sstream>

namespace bminor {

const char* nodeKindName(NodeKind kind) {
  switch (kind) {
    case NodeKind::Program: return "Program";
    case NodeKind::FunctionDecl: return "FuncDecl";
    case NodeKind::Param: return "Param";
    case NodeKind::VarDecl: return "VarDecl";
    case NodeKind::TypeSpec: return "TypeSpec";
    case NodeKind::Block: return "Block";
    case NodeKind::ExprStmt: return "ExprStmt";
    case NodeKind::IfStmt: return "IfStmt";
    case NodeKind::WhileStmt: return "WhileStmt";
    case NodeKind::ForStmt: return "ForStmt";
    case NodeKind::ReturnStmt: return "ReturnStmt";
    case NodeKind::PrintStmt: return "PrintStmt";
    case NodeKind::EmptyStmt: return "EmptyStmt";
    case NodeKind::AssignmentExpr: return "AssignmentExpr";
    case NodeKind::BinaryExpr: return "BinaryExpr";
    case NodeKind::UnaryExpr: return "UnaryExpr";
    case NodeKind::CallExpr: return "CallExpr";
    case NodeKind::IndexExpr: return "IndexExpr";
    case NodeKind::IntLiteral: return "IntLiteral";
    case NodeKind::StringLiteral: return "StringLiteral";
    case NodeKind::CharLiteral: return "CharLiteral";
    case NodeKind::BoolLiteral: return "BoolLiteral";
    case NodeKind::IdentifierExpr: return "IdentifierExpr";
  }
  return "Unknown";
}

void forEachChild(const Node& node, const std::function<void(const Node&)>& fn) {
  switch (node.kind) {
    case NodeKind::Program:
      for (const DeclPtr& decl : static_cast<const Program&>(node).declarations) {
        if (decl) fn(*decl);
      }
      break;
    case NodeKind::FunctionDecl: {
      const auto& decl = static_cast<const FunctionDecl&>(node);
      for (const auto& param : decl.params) {
        if (param) fn(*param);
      }
      if (decl.returnType) fn(*decl.returnType);
      if (decl.body) fn(*decl.body);
      break;
    }
    case NodeKind::Param: {
      const auto& param = static_cast<const Param&>(node);
      if (param.type) fn(*param.type);
      break;
    }
    case NodeKind::VarDecl: {
      const auto& decl = static_cast<const VarDecl&>(node);
      if (decl.type) fn(*decl.type);
      if (decl.initializer) fn(*decl.initializer);
      break;
    }
    case NodeKind::TypeSpec: {
      const auto& type = static_cast<const TypeSpec&>(node);
      if (type.element) fn(*type.element);
      break;
    }
    case NodeKind::Block:
      for (const NodePtr& item : static_cast<const Block&>(node).items) {
        if (item) fn(*item);
      }
      break;
    case NodeKind::ExprStmt: {
      const auto& stmt = static_cast<const ExprStmt&>(node);
      if (stmt.expr) fn(*stmt.expr);
      break;
    }
    case NodeKind::IfStmt: {
      const auto& stmt = static_cast<const IfStmt&>(node);
      if (stmt.condition) fn(*stmt.condition);
      if (stmt.thenBranch) fn(*stmt.thenBranch);
      if (stmt.elseBranch) fn(*stmt.elseBranch);
      break;
    }
    case NodeKind::WhileStmt: {
      const auto& stmt = static_cast<const WhileStmt&>(node);
      if (stmt.condition) fn(*stmt.condition);
      if (stmt.body) fn(*stmt.body);
      break;
    }
    case NodeKind::ForStmt: {
      const auto& stmt = static_cast<const ForStmt&>(node);
      if (stmt.init) fn(*stmt.init);
      if (stmt.condition) fn(*stmt.condition);
      if (stmt.step) fn(*stmt.step);
      if (stmt.body) fn(*stmt.body);
      break;
    }
    case NodeKind::ReturnStmt: {
      const auto& stmt = static_cast<const ReturnStmt&>(node);
      if (stmt.value) fn(*stmt.value);
      break;
    }
    case NodeKind::PrintStmt:
      for (const ExprPtr& arg : static_cast<const PrintStmt&>(node).arguments) {
        if (arg) fn(*arg);
      }
      break;
    case NodeKind::AssignmentExpr: {
      const auto& expr = static_cast<const AssignmentExpr&>(node);
      if (expr.target) fn(*expr.target);
      if (expr.value) fn(*expr.value);
      break;
    }
    case NodeKind::BinaryExpr: {
      const auto& expr = static_cast<const BinaryExpr&>(node);
      if (expr.lhs) fn(*expr.lhs);
      if (expr.rhs) fn(*expr.rhs);
      break;
    }
    case NodeKind::UnaryExpr: {
      const auto& expr = static_cast<const UnaryExpr&>(node);
      if (expr.operand) fn(*expr.operand);
      break;
    }
    case NodeKind::CallExpr: {
      const auto& expr = static_cast<const CallExpr&>(node);
      if (expr.callee) fn(*expr.callee);
      for (const ExprPtr& arg : expr.arguments) {
        if (arg) fn(*arg);
      }
      break;
    }
    case NodeKind::IndexExpr: {
      const auto& expr = static_cast<const IndexExpr&>(node);
      if (expr.base) fn(*expr.base);
      if (expr.index) fn(*expr.index);
      break;
    }
    case NodeKind::EmptyStmt:
    case NodeKind::IntLiteral:
    case NodeKind::StringLiteral:
    case NodeKind::CharLiteral:
    case NodeKind::BoolLiteral:
    case NodeKind::IdentifierExpr:
      break;
  }
}

namespace {

std::string describe(const Node& node) {
  std::ostringstream out;
  switch (node.kind) {
    case NodeKind::FunctionDecl:
      out << " name=" << static_cast<const FunctionDecl&>(node).name.lexeme;
      break;
    case NodeKind::Param:
      out << " name=" << static_cast<const Param&>(node).name.lexeme;
      break;
    case NodeKind::VarDecl:
      out << " name=" << static_cast<const VarDecl&>(node).name.lexeme;
      break;
    case NodeKind::TypeSpec: {
      const auto& type = static_cast<const TypeSpec&>(node);
      out << " text=" << (type.keyword.lexeme.empty() ? "<missing>" : type.keyword.lexeme);
      break;
    }
    case NodeKind::AssignmentExpr:
      out << " op=" << static_cast<const AssignmentExpr&>(node).op.lexeme;
      break;
    case NodeKind::BinaryExpr:
      out << " op=" << static_cast<const BinaryExpr&>(node).opToken.lexeme;
      break;
    case NodeKind::UnaryExpr:
      out << " op=" << static_cast<const UnaryExpr&>(node).opToken.lexeme;
      break;
    case NodeKind::IdentifierExpr:
      out << " name=" << static_cast<const IdentifierExpr&>(node).name.lexeme;
      break;
    case NodeKind::IntLiteral: {
      const auto& lit = static_cast<const IntLiteral&>(node);
      out << " value=" << lit.value;
      break;
    }
    case NodeKind::StringLiteral:
      out << " value=\"" << static_cast<const StringLiteral&>(node).value << "\"";
      break;
    case NodeKind::CharLiteral: {
      const auto& lit = static_cast<const CharLiteral&>(node);
      out << " value=" << lit.value;
      break;
    }
    case NodeKind::BoolLiteral:
      out << " value=" << (static_cast<const BoolLiteral&>(node).value ? "true" : "false");
      break;
    default:
      break;
  }
  if (node.loc.valid) {
    out << " @" << node.loc.line << ":" << node.loc.column;
  } else {
    out << " @<synthetic>";
  }
  out << " type=" << (node.typeAnnotation ? *node.typeAnnotation : "<null>");
  return out.str();
}

}  // namespace

void dumpAst(const Node& node, std::ostream& out, int indent) {
  out << std::string(static_cast<std::size_t>(indent) * 2, ' ') << nodeKindName(node.kind)
      << describe(node) << "\n";
  forEachChild(node, [&](const Node& child) { dumpAst(child, out, indent + 1); });
}

std::unordered_set<std::size_t> collectFunctionNameOffsets(const Node& root) {
  std::unordered_set<std::size_t> offsets;
  if (root.kind == NodeKind::FunctionDecl) {
    offsets.insert(static_cast<const FunctionDecl&>(root).name.offset);
  } else if (root.kind == NodeKind::CallExpr) {
    const auto& call = static_cast<const CallExpr&>(root);
    if (call.callee && call.callee->kind == NodeKind::IdentifierExpr) {
      offsets.insert(static_cast<const IdentifierExpr&>(*call.callee).name.offset);
    }
  }
  forEachChild(root, [&](const Node& child) { offsets.merge(collectFunctionNameOffsets(child)); });
  return offsets;
}

std::string decodeStringLiteral(const std::string& lexeme) {
  std::string value;
  std::size_t i = 0;
  if (!lexeme.empty() && lexeme.front() == '"') ++i;
  const std::size_t limit = (!lexeme.empty() && lexeme.back() == '"' && lexeme.size() > 1)
                                ? lexeme.size() - 1
                                : lexeme.size();
  while (i < limit) {
    const char c = lexeme[i++];
    if (c != '\\' || i >= limit) {
      value.push_back(c);
      continue;
    }
    const char escape = lexeme[i++];
    switch (escape) {
      case 'n': value.push_back('\n'); break;
      case 't': value.push_back('\t'); break;
      case 'r': value.push_back('\r'); break;
      case '0': value.push_back('\0'); break;
      case 'a': value.push_back('\a'); break;
      case 'b': value.push_back('\b'); break;
      case 'f': value.push_back('\f'); break;
      case 'v': value.push_back('\v'); break;
      case '\\': value.push_back('\\'); break;
      case '"': value.push_back('"'); break;
      case '\'': value.push_back('\''); break;
      default:
        value.push_back('\\');
        value.push_back(escape);
        break;
    }
  }
  return value;
}

int decodeCharLiteral(const std::string& lexeme) {
  if (lexeme.size() < 3) return 0;
  const std::string inner = lexeme.substr(1, lexeme.size() - 2);
  if (inner.empty()) return 0;
  if (inner[0] != '\\') return static_cast<unsigned char>(inner[0]);
  const std::string decoded = decodeStringLiteral("\"" + inner + "\"");
  return decoded.empty() ? 0 : static_cast<unsigned char>(decoded[0]);
}

}  // namespace bminor

#include "bminor/highlight.h"

#include <unordered_set>

namespace bminor {
namespace {

const char* cssClass(HighlightClass cls) {
  switch (cls) {
    case HighlightClass::Keyword: return "kw";
    case HighlightClass::TypeName: return "type";
    case HighlightClass::Variable: return "var";
    case HighlightClass::Function: return "func";
    case HighlightClass::Number: return "num";
    case HighlightClass::Text: return "str";
    case HighlightClass::Boolean: return "bool";
    case HighlightClass::Operator: return "op";
    case HighlightClass::Comment: return "com";
    case HighlightClass::Error: return "err";
  }
  return "var";
}

/// ANSI SGR sequence for a category. 256 colour codes are used where a basic
/// palette has no close match (orange, gold, warm green).
const char* ansiCode(HighlightClass cls) {
  switch (cls) {
    case HighlightClass::Keyword: return "\x1b[34;1m";      // bold blue
    case HighlightClass::TypeName: return "\x1b[36m";       // teal / cyan
    case HighlightClass::Variable: return "";               // terminal default
    case HighlightClass::Function: return "\x1b[38;5;220m"; // gold
    case HighlightClass::Number: return "\x1b[38;5;208m";   // orange
    case HighlightClass::Text: return "\x1b[38;5;114m";     // warm green
    case HighlightClass::Boolean: return "\x1b[38;5;208m";  // orange
    case HighlightClass::Operator: return "\x1b[38;5;250m"; // light gray
    case HighlightClass::Comment: return "\x1b[3;38;5;242m";// dim gray, italic
    case HighlightClass::Error: return "\x1b[4;31m";        // red underline
  }
  return "";
}

std::string escapeHtml(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += c; break;
    }
  }
  return out;
}

}  // namespace

std::vector<ClassifiedToken> classifyTokens(const std::vector<Token>& tokens, const Node* ast) {
  std::unordered_set<std::size_t> functionNames;
  if (ast) functionNames = collectFunctionNameOffsets(*ast);

  std::vector<ClassifiedToken> classified;
  classified.reserve(tokens.size());
  for (const Token& token : tokens) {
    if (token.kind == TokenKind::EndOfFile || token.kind == TokenKind::Whitespace) continue;

    HighlightClass cls = HighlightClass::Variable;
    if (token.kind == TokenKind::Identifier) {
      cls = functionNames.count(token.offset) > 0 ? HighlightClass::Function
                                                  : HighlightClass::Variable;
    } else if (isTypeKeyword(token.kind)) {
      cls = HighlightClass::TypeName;
    } else if (isBooleanLiteralKeyword(token.kind)) {
      cls = HighlightClass::Boolean;
    } else if (isKeyword(token.kind)) {
      cls = HighlightClass::Keyword;
    } else if (token.kind == TokenKind::IntegerLiteral) {
      cls = HighlightClass::Number;
    } else if (token.kind == TokenKind::StringLiteral || token.kind == TokenKind::CharLiteral) {
      cls = HighlightClass::Text;
    } else if (isOperator(token.kind) || isDelimiter(token.kind)) {
      cls = HighlightClass::Operator;
    } else if (token.kind == TokenKind::Comment) {
      cls = HighlightClass::Comment;
    } else if (token.kind == TokenKind::Invalid) {
      cls = HighlightClass::Error;
    }
    classified.push_back(ClassifiedToken{&token, cls});
  }
  return classified;
}

std::string renderAnsi(const std::string& source, const std::vector<ClassifiedToken>& tokens,
                       bool useColor) {
  if (!useColor) return source;

  std::string out;
  out.reserve(source.size() + tokens.size() * 12);
  std::size_t cursor = 0;
  for (const ClassifiedToken& item : tokens) {
    const Token& token = *item.token;
    if (token.offset < cursor || token.end() > source.size()) continue;  // defensive
    out.append(source, cursor, token.offset - cursor);  // untouched whitespace / gaps
    out += ansiCode(item.cls);
    out += token.lexeme;
    out += "\x1b[0m";
    cursor = token.end();
  }
  out.append(source, cursor, source.size() - cursor);
  return out;
}

std::string renderHtml(const std::string& source, const std::vector<ClassifiedToken>& tokens,
                       const std::string& title, const DiagnosticBag& diagnostics) {
  std::string body;
  body.reserve(source.size() + tokens.size() * 40);
  std::size_t cursor = 0;
  for (const ClassifiedToken& item : tokens) {
    const Token& token = *item.token;
    if (token.offset < cursor || token.end() > source.size()) continue;
    body += escapeHtml(source.substr(cursor, token.offset - cursor));
    body += "<span class=\"";
    body += cssClass(item.cls);
    body += "\">";
    body += escapeHtml(token.lexeme);
    body += "</span>";
    cursor = token.end();
  }
  body += escapeHtml(source.substr(cursor));

  std::string html;
  html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
  html += "<title>" + escapeHtml(title) + "</title>\n<style>\n";
  html +=
      "  body { background: #1e1e1e; color: #f0f0f0; font-family: 'DejaVu Sans Mono', "
      "Consolas, monospace; margin: 0; padding: 24px; }\n"
      "  h1 { font-size: 15px; font-weight: normal; color: #9da5b4; margin: 0 0 12px 0; }\n"
      "  pre.bminor { font-size: 14px; line-height: 1.5; padding: 16px; border-radius: 6px;\n"
      "              background: #252526; border: 1px solid #333333; overflow-x: auto; }\n"
      "  .kw   { color: #4c9aff; font-weight: bold; }\n"
      "  .type { color: #2ec4b6; }\n"
      "  .var  { color: #f0f0f0; }\n"
      "  .func { color: #e5c07b; }\n"
      "  .num  { color: #ff9f43; }\n"
      "  .str  { color: #98c379; }\n"
      "  .bool { color: #ff9f43; }\n"
      "  .op   { color: #c8c8c8; }\n"
      "  .com  { color: #7f848e; font-style: italic; }\n"
      "  .err  { color: #ff5555; text-decoration: underline wavy #ff5555; }\n"
      "  .legend { font-size: 12px; color: #9da5b4; margin-top: 16px; }\n"
      "  .legend span { margin-right: 14px; }\n"
      "  .diagnostics { font-size: 13px; color: #ff8080; margin-top: 12px; }\n"
      "</style>\n</head>\n<body>\n";
  html += "<h1>B-Minor &mdash; " + escapeHtml(title) + "</h1>\n";
  html += "<pre class=\"bminor\">";
  html += body;
  html += "</pre>\n";

  html += "<div class=\"legend\">";
  html += "<span class=\"kw\">keyword</span><span class=\"type\">type name</span>";
  html += "<span class=\"var\">variable</span><span class=\"func\">function</span>";
  html += "<span class=\"num\">integer</span><span class=\"str\">string/char</span>";
  html += "<span class=\"bool\">boolean</span><span class=\"op\">operator</span>";
  html += "<span class=\"com\">comment</span><span class=\"err\">invalid</span>";
  html += "</div>\n";

  if (!diagnostics.all().empty()) {
    html += "<div class=\"diagnostics\"><ul>\n";
    for (const Diagnostic& d : diagnostics.all()) {
      const char* label = d.severity == Severity::Error
                              ? "error"
                              : (d.severity == Severity::Warning ? "warning" : "note");
      html += "<li>" + escapeHtml(d.loc.toString(diagnostics.fileName()) + ": " + label + ": " +
                                  d.message) +
              "</li>\n";
    }
    html += "</ul></div>\n";
  }

  html += "</body>\n</html>\n";
  return html;
}

}  // namespace bminor

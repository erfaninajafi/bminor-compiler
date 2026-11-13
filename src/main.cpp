#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bminor/ast.h"
#include "bminor/diagnostics.h"
#include "bminor/highlight.h"
#include "bminor/lexer.h"
#include "bminor/parser.h"
#include "bminor/token.h"

namespace {

struct Options {
  std::string input;
  std::string output;
  std::string format = "ansi";  // ansi | html | none
  bool dumpTokens = false;
  bool dumpAst = false;
  bool useColor = true;
};

void printUsage(std::ostream& out) {
  out << "usage: bminor [options] <file.bminor>\n\n"
      << "Phase one front-end for the B-Minor language: lexical analysis, parsing,\n"
      << "AST construction and AST driven syntax highlighting.\n\n"
      << "options:\n"
      << "  --tokens             print the token stream (type, lexeme, location)\n"
      << "  --ast                print the abstract syntax tree\n"
      << "  --highlight=<fmt>    output format: ansi (default), html or none\n"
      << "  --out <file>         write the highlighted result to <file>\n"
      << "  --no-color           do not emit ANSI escape sequences\n"
      << "  -h, --help           show this help\n";
}

std::string replaceExtension(const std::string& path, const std::string& extension) {
  const std::size_t slash = path.find_last_of('/');
  const std::size_t dot = path.find_last_of('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
    return path + extension;
  }
  return path.substr(0, dot) + extension;
}

std::string readFile(const std::string& path, bool& ok) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    ok = false;
    return {};
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  ok = true;
  return buffer.str();
}

void dumpTokens(const std::vector<bminor::Token>& tokens, std::ostream& out) {
  for (const bminor::Token& token : tokens) {
    out << bminor::tokenKindName(token.kind);
    if (!token.lexeme.empty()) out << " \"" << token.lexeme << "\"";
    out << " @ " << token.loc.line << ":" << token.loc.column << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  Options options;
  std::vector<std::string> positional;

  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "-h" || argument == "--help") {
      printUsage(std::cout);
      return 0;
    } else if (argument == "--tokens") {
      options.dumpTokens = true;
    } else if (argument == "--ast") {
      options.dumpAst = true;
    } else if (argument == "--no-color") {
      options.useColor = false;
    } else if (argument.rfind("--highlight=", 0) == 0) {
      options.format = argument.substr(std::string("--highlight=").size());
      if (options.format != "ansi" && options.format != "html" && options.format != "none") {
        std::cerr << "bminor: unknown highlight format '" << options.format << "'\n";
        return 2;
      }
    } else if (argument == "--out") {
      if (i + 1 >= argc) {
        std::cerr << "bminor: --out requires a file name\n";
        return 2;
      }
      options.output = argv[++i];
    } else if (!argument.empty() && argument[0] == '-') {
      std::cerr << "bminor: unknown option '" << argument << "'\n";
      printUsage(std::cerr);
      return 2;
    } else {
      positional.push_back(argument);
    }
  }

  if (positional.size() != 1) {
    printUsage(std::cerr);
    return 2;
  }
  options.input = positional.front();

  bool readOk = false;
  const std::string source = readFile(options.input, readOk);
  if (!readOk) {
    std::cerr << "bminor: cannot read '" << options.input << "'\n";
    return 2;
  }

  bminor::DiagnosticBag diagnostics;
  diagnostics.setFileName(options.input);

  bminor::Lexer lexer(options.input, source, diagnostics);
  const std::vector<bminor::Token> tokens = lexer.tokenize();

  bminor::Parser parser(tokens, diagnostics);
  const std::unique_ptr<bminor::Program> program = parser.parseProgram();

  if (options.dumpTokens) dumpTokens(tokens, std::cout);
  if (options.dumpAst) bminor::dumpAst(*program, std::cout);

  if (options.format == "ansi") {
    const auto classified = bminor::classifyTokens(tokens, program.get());
    const std::string rendered = bminor::renderAnsi(source, classified, options.useColor);
    if (options.output.empty()) {
      std::cout << rendered;
      if (!rendered.empty() && rendered.back() != '\n') std::cout << '\n';
    } else {
      std::ofstream out(options.output, std::ios::binary);
      if (!out) {
        std::cerr << "bminor: cannot write '" << options.output << "'\n";
        return 2;
      }
      out << rendered;
    }
  }

  if (options.format == "html") {
    const auto classified = bminor::classifyTokens(tokens, program.get());
    const std::string document =
        bminor::renderHtml(source, classified, options.input, diagnostics);
    const std::string target =
        options.output.empty() ? replaceExtension(options.input, ".html") : options.output;
    std::ofstream out(target, std::ios::binary);
    if (!out) {
      std::cerr << "bminor: cannot write '" << target << "'\n";
      return 2;
    }
    out << document;
    std::cerr << "bminor: wrote " << target << "\n";
  }

  diagnostics.print(std::cerr);
  if (!diagnostics.all().empty()) {
    std::cerr << "bminor: " << diagnostics.errorCount() << " error(s) reported\n";
  }
  return diagnostics.hasErrors() ? 1 : 0;
}

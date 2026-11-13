<p align="center">
  <img src="docs/assets/amirkabir-logo.png" width="110" alt="Amirkabir University Logo">
</p>

<p align="center">
  <strong>Erfan Najafi</strong><br>
  40113901<br>
  Compiler Project<br>
  Instructor: Prof.Dr. Shirali-Shareza
</p>

---

# B-Minor front-end — phase one

Phase one of the compiler-design project: a C++ front-end for the B-Minor
language with a lexer, an LL(1) recursive-descent parser, an abstract syntax
tree and an AST-driven syntax highlighter that prints to the terminal (ANSI) or
to a standalone HTML file.

Everything is implemented with the C++17 standard library only — no external
dependencies, no regex library, no parser generator.

## Build

```sh
make            # builds bin/bminor
make test       # runs the test suite
make clean
```

Requirements: `g++` (C++17) and `make`. Verified with GCC 15 under Linux.

## Usage

```
bminor [options] <file.bminor>

  --tokens             print the token stream (type, lexeme, line:column)
  --ast                print the abstract syntax tree
  --highlight=<fmt>    output format: ansi (default), html or none
  --out <file>         write the highlighted result to <file>
  --no-color           do not emit ANSI escape sequences
  -h, --help
```

Examples:

```sh
# coloured source in the terminal
./bin/bminor tests/valid/highlight_context.bminor

# a standalone, script-free HTML rendering next to the source file
./bin/bminor --highlight=html tests/valid/highlight_context.bminor
# (use --out to choose the destination, e.g. --out out/sample.html)

# inspect the token stream and the AST
./bin/bminor --tokens --ast --highlight=none tests/valid/expressions.bminor
```

Diagnostics always go to `stderr` in `file:line:column: error: message` form,
and the exit status is `1` when any error was reported, `2` for usage
problems, `0` otherwise.

## Language

B-Minor as specified for this course: `integer`, `boolean`, `char`, `string`,
`void` and `array[<type>]` types, functions with parameters and result types,
`var` declarations, `if`/`else`, `while`, `for`, `return`, `print`, both
`//` line comments and `/* … */` block comments, the operator set
`+ - * / % = += -= *= /= %= == != < <= > >= && || !`, and string/character
literals with escape sequences.

The grammar is documented formally in [`docs/grammar.md`](docs/grammar.md)
(EBNF, left-recursion removal, LL(1) analysis); the lexical rules and their
regular expressions are in [`docs/tokens.md`](docs/tokens.md); the architecture
and design decisions are in [`docs/design.md`](docs/design.md).

## Project layout

```
include/bminor/    public headers (source locations, tokens, AST, lexer, parser, highlighting)
src/               implementation and the bminor driver
docs/              grammar, lexical specification, design notes, logo asset
tests/valid/       programs that must lex and parse cleanly
tests/invalid/     programs with lexical or syntax errors and their expected diagnostics
tests/run_tests.sh test driver
```

## Highlighting categories

Colours follow the table in the project description: keywords (bold blue), type
names (teal), variable identifiers (default), function names (gold), integer
literals (orange), string and character literals (warm green), boolean literals
(orange), operators (light gray), comments (dim gray, italic) and invalid tokens
(red, underlined in HTML).

The distinction between a function name and a variable identifier is taken from
the AST (declaration names and call callees), not from the token stream, which
is what makes the colouring context-sensitive.

## Tests

```sh
make test
```

The suite checks four things: valid programs produce no diagnostics, invalid
programs produce exactly the recorded diagnostics while still finishing the
analysis, the coloured output is byte-identical to the source once escape codes
are removed, and the HTML document contains the required `<pre>` block, span
classes and diagnostics — without JavaScript.

## Scope of this phase

Lexing, parsing, AST construction, error reporting/recovery and highlighting
only. There is no symbol table or type checker yet: every AST node keeps an
empty type slot that the semantic analysis of phase two will fill.

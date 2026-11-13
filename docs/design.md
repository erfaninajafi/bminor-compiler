# Design notes — phase one front-end

## 1. Pipeline

The system follows the classic compiler front-end pipeline extended with the
services an IDE needs:

```
source text
    │  (1) lexer            → token stream + lexical diagnostics
    ▼
token stream
    │  (2) parser           → AST + syntax diagnostics (panic-mode recovery)
    ▼
AST (typed slots empty)
    │  (3) classifier       → colour category per token (AST + token kinds)
    ▼
classified tokens
    │  (4) renderer         → ANSI terminal output or standalone HTML
    ▼
highlighted source
```

Stages only communicate through data structures (`Token`, `Node`, `Diagnostic`,
`ClassifiedToken`), never through globals, so each one can be tested and
replaced independently: the lexer can be swapped for a table-driven DFA, or the
renderer for an editor plug-in, without touching the others.

## 2. Modules

| File | Responsibility |
|------|----------------|
| `include/bminor/source.h` | `SourceLocation` (line, column, byte offset) |
| `include/bminor/diagnostics.h`, `src/diagnostics.cpp` | `DiagnosticBag`: collect and print `file:line:col: error: …` |
| `include/bminor/token.h`, `src/token.cpp` | token kinds, category predicates, keyword table |
| `include/bminor/lexer.h`, `src/lexer.cpp` | DFA scanner with maximal munch and error recovery |
| `include/bminor/ast.h`, `src/ast.cpp` | node hierarchy, traversal, tree dump, function-name index |
| `include/bminor/parser.h`, `src/parser.cpp` | LL(1) recursive-descent parser with panic-mode recovery |
| `include/bminor/highlight.h`, `src/highlight.cpp` | classification and ANSI/HTML rendering |
| `src/main.cpp` | command line driver |

## 3. Key data structures

**`Token`** — type, lexeme, `SourceLocation`, plus `offset`/`length`. The byte
offsets are what allow the highlighter to rebuild the original file exactly:
the renderer copies the raw text between two tokens verbatim and only wraps the
token lexemes themselves. Consequently whitespace, blank lines, comments and
odd indentation survive untouched.

**`SourceLocation`** — line and column for humans, byte offset for machines,
plus a `valid` flag used for synthetic positions created during recovery.

**`DiagnosticBag`** — a vector of diagnostics plus an error counter. The lexer
and the parser both push into the same bag, so the driver can print one ordered
list and decide the exit status.

**AST** — one class per construct, all deriving from `Node`, which stores
`kind`, `loc`, and `typeAnnotation`. `typeAnnotation` is an empty
`std::optional<std::string>` after parsing: phase 2 fills it during semantic
analysis. The tree is owned by `std::unique_ptr`, and `forEachChild` gives a
single traversal primitive used by the dump, the function-name index and future
analyses.

## 4. Algorithms

**Scanning.** A hand-written DFA: the scanner loop dispatches on the current
character, then consumes the longest matching lexeme. Two-character operators
are attempted before one-character ones, `//`/`/*` are attempted before `/`,
and identifiers are looked up in a hash table of the sixteen keywords after
their full lexeme is read. Every malformed input produces a token and a
diagnostic and scanning resumes, so one bad character cannot hide the rest of
the file. Complexity is O(n) in the input length with a constant factor of at
most two characters of lookahead.

**Parsing.** Recursive descent over the LL(1) grammar in `docs/grammar.md`.
Precedence climbing is encoded as a chain of functions (`parseAdditive` calls
`parseMultiplicative` calls `parseUnary` …), which mirrors the grammar levels
and makes precedence and associativity obvious from the code. Assignment is
parsed right-recursively so `a = b = c` associates to the right.

**Error recovery.** On an unexpected token the parser records one message,
enters panic mode (which suppresses cascading messages for the same statement)
and calls `synchronize()`, which discards tokens until `;` (consumed), `}`
(left for the enclosing block) or the start of a new statement/declaration.
Every loop (`parseProgram`, `parseBlock`, parameter and argument lists) forces
progress if recovery consumed nothing, so the parser terminates on any input.
`if (...) { return 1; }` with a missing `)` still yields a usable `IfStmt`
because the branch is parsed at statement level after the diagnostic.

**Highlighting.** Base categories come from the token kind (keywords, type
keywords, boolean literals, literals, operators, comments, invalid). The one
context-sensitive decision an IDE cannot make from the token stream alone —
"is this identifier a function name or a variable?" — is taken from the AST:
`collectFunctionNameOffsets` gathers the byte offsets of every `FuncDecl` name
and every `CallExpr` callee, and the classifier colours exactly those names as
functions. This is why a pure regex-based highlighter is not used and would not
satisfy the requirement: `gcd(limit, 6)` and `limit` in
`tests/valid/highlight_context.bminor` are textually identical identifiers but
receive different categories.

## 5. Design decisions

| Decision | Rationale |
|----------|-----------|
| Hand-written DFA instead of a regex library | no dependencies, exact control over maximal munch and error recovery, and it documents the regular expressions it implements |
| Unsigned integer literals in the lexer, sign folded into the AST | keeps `a-1` correct (see `tokens.md` §5) |
| Comments emitted as tokens, filtered by the parser | the highlighter must colour them, the parser must not see them |
| Whitespace skipped, offsets remembered | exact source reproduction without storing whitespace tokens |
| `Block` derives from `Stmt` | `statement ::= … \| block` becomes a direct return, and blocks still nest in the child list of their parent |
| Panic mode with a synchronization *set* rather than a single token | `;`, `}`, `if`, `while`, `for`, `return`, `print`, `var`, `function` cover both statement and declaration boundaries, so recovery resumes at a unit the parser can trust |
| HTML output contains no JavaScript | required by the specification; only embedded CSS is used |
| Byte-based columns | keeps the scanner simple and locations consistent with editor byte offsets; non-ASCII characters count per byte |

`=`/`<` note: the project text illustrates maximal munch with `=<`. B-Minor's
operator is `<=`; `=<` is not in the language, so it lexes as two tokens. The
example is read as `<=`; see `tokens.md` §3.

## 6. What phase one does not do

* No symbol table and no type checking yet: `typeAnnotation` stays empty for
  every node, as the specification requires for this phase.
* No semantic constraints on the syntax tree (duplicate parameters, `break`
  outside a loop, and so on); those belong to phase two.
* Arrays are typed (`array[integer]`) but have no literal syntax and no size
  in the declarator; the phase-two semantic analysis can decide how sizes are
  supplied without changing the grammar.
* No CFG/call-graph construction: phase three adds those modules on top of the
  AST, which is why the AST keeps precise locations and a generic traversal.

## 7. Testing

`tests/run_tests.sh` runs four groups:

1. **valid programs** — `tests/valid/*.bminor` must tokenize without `INVALID`
   tokens and parse with no diagnostics; the AST is also dumped to make sure
   every construct traverses.
2. **invalid programs** — `tests/invalid/*.bminor` must exit with status 1 and
   print exactly the diagnostics in the matching `.expected` file, which checks
   message text, order and location, and proves that recovery continues instead
   of aborting.
3. **highlight amplification** — the ANSI output with escapes stripped, and the
   `--no-color` output, must be byte-identical to the input, so colouring never
   alters the source.
4. **HTML contract** — the document must contain a `<pre class="bminor">`
   block, the expected class names for every category, an `err` span for
   invalid input, the diagnostics list, and no JavaScript.

# The B-Minor grammar

This document is the formal, complete grammar of the B-Minor subset that the
phase-one front-end accepts. It is written in EBNF and is the single source of
truth: `src/parser.cpp` implements exactly these productions, one function per
non-terminal.

## 1. Notation

| Construct | Meaning |
|-----------|---------|
| `name ::= rest` | `name` is defined as `rest` |
| `a b c` | sequence: `a` then `b` then `c` |
| `a \| b` | choice: `a` or `b` |
| `[ a ]` | option: `a` may appear zero or one time |
| `{ a }` | repetition: `a` may appear zero or more times |
| `'text'` | a terminal token (its lexeme) |
| `UPPER` | a token class produced by the lexer (see `tokens.md`) |

Comments (`// ...` and `/* ... */`) are removed by the lexer before parsing,
so they never appear in the grammar; semantically they behave like whitespace.

## 2. Lexical level

```
IDENT          ::= [A-Za-z_] [A-Za-z0-9_]*
INT_LITERAL    ::= [0-9]+
STRING_LITERAL ::= '"' { character | escape } '"'
CHAR_LITERAL   ::= "'" ( character | escape ) "'"
```

The complete set of keywords and operators, and the priority rules that decide
between them, are documented in [`tokens.md`](tokens.md).

## 3. Syntactic grammar (EBNF, as implemented)

```
program         ::= { top_level_decl } EOF

top_level_decl  ::= function_decl
                  | var_decl

function_decl   ::= 'function' IDENT '(' [ param_list ] ')' [ ':' type_spec ] block
param_list      ::= param { ',' param }
param           ::= IDENT ':' type_spec

var_decl        ::= 'var' IDENT ':' type_spec [ '=' expression ] ';'

type_spec       ::= 'integer'
                  | 'boolean'
                  | 'char'
                  | 'string'
                  | 'void'
                  | 'array' [ '[' type_spec ']' ]

block           ::= '{' { block_item } '}'
block_item      ::= var_decl
                  | statement

statement       ::= expression_stmt
                  | if_stmt
                  | while_stmt
                  | for_stmt
                  | return_stmt
                  | print_stmt
                  | block
                  | empty_stmt

expression_stmt ::= expression ';'
empty_stmt      ::= ';'

if_stmt         ::= 'if' '(' expression ')' statement [ 'else' statement ]
while_stmt      ::= 'while' '(' expression ')' statement
for_stmt        ::= 'for' '(' [ expression ] ';' [ expression ] ';' [ expression ] ')' statement
return_stmt     ::= 'return' [ expression ] ';'
print_stmt      ::= 'print' [ expression { ',' expression } ] ';'

expression      ::= assignment
assignment      ::= logical_or [ assign_op assignment ]
assign_op       ::= '=' | '+=' | '-=' | '*=' | '/=' | '%='

logical_or      ::= logical_and { '||' logical_and }
logical_and     ::= equality { '&&' equality }
equality        ::= relational { ( '==' | '!=' ) relational }
relational      ::= additive { ( '<' | '<=' | '>' | '>=' ) additive }
additive        ::= multiplicative { ( '+' | '-' ) multiplicative }
multiplicative  ::= unary { ( '*' | '/' | '%' ) unary }
unary           ::= ( '!' | '-' | '+' ) unary
                  | postfix
postfix         ::= primary { '[' expression ']' | '(' [ arg_list ] ')' }
primary         ::= INT_LITERAL
                  | STRING_LITERAL
                  | CHAR_LITERAL
                  | 'true'
                  | 'false'
                  | IDENT
                  | '(' expression ')'
arg_list        ::= expression { ',' expression }
```

Coverage of the constructs the phase-one system must process:

| Construct | Production | AST node |
|-----------|------------|----------|
| translation unit | `program` | `Program` |
| function definition | `function_decl` | `FuncDecl` |
| parameters | `param_list`, `param` | `Param` |
| variable declaration (global and local) | `var_decl` | `VarDecl` |
| types, including arrays | `type_spec` | `TypeSpec` |
| block | `block` | `Block` |
| expression statement | `expression_stmt` | `ExprStmt` |
| empty statement | `empty_stmt` | `EmptyStmt` |
| conditional | `if_stmt` | `IfStmt` |
| loop | `while_stmt`, `for_stmt` | `WhileStmt`, `ForStmt` |
| return | `return_stmt` | `ReturnStmt` |
| output | `print_stmt` | `PrintStmt` |
| assignment and compound assignment | `assignment` | `AssignmentExpr` |
| binary operations | `logical_or` … `multiplicative` | `BinaryExpr` |
| unary operations | `unary` | `UnaryExpr` |
| call and indexing | `postfix` | `CallExpr`, `IndexExpr` |
| literals and names | `primary` | `IntLiteral`, `StringLiteral`, `CharLiteral`, `BoolLiteral`, `IdentifierExpr` |

An omitted result type (`function f() { ... }`) is the same as `: void`; the
parser materialises an implicit `void` `TypeSpec` node for it.

## 4. Left recursion: analysis and removal

A recursive-descent parser needs a grammar that is *not* left recursive,
otherwise the corresponding function calls itself before consuming a token and
overflows the stack. The "natural" way to write the expression grammar (the one
usually found in language references) is left recursive:

```
additive       ::= additive '+' multiplicative
                 | additive '-' multiplicative
                 | multiplicative
multiplicative ::= multiplicative '*' unary
                 | multiplicative '/' unary
                 | multiplicative '%' unary
                 | unary
postfix        ::= postfix '[' expression ']'
                 | postfix '(' [ arg_list ] ')'
                 | primary
```

Every one of these rules is left recursive (`additive` starts with `additive`,
`postfix` starts with `postfix`, …). Each is removed with the standard
left-recursion elimination rule

```
A ::= A α | β        ⟹        A ::= β { α }
```

which yields exactly the rules of section 3:

```
additive       ::= multiplicative { ( '+' | '-' ) multiplicative }
multiplicative ::= unary { ( '*' | '/' | '%' ) unary }
postfix        ::= primary { '[' expression ']' | '(' [ arg_list ] ')' }
equality       ::= relational { ( '==' | '!=' ) relational }
relational     ::= additive { ( '<' | '<=' | '>' | '>=' ) additive }
logical_and    ::= equality { '&&' equality }
logical_or    ::= logical_and { '||' logical_and }
```

No other non-terminal is left recursive; `assignment` and `type_spec` are
right recursive or iterative, which is harmless for a top-down parser.
`type_spec ::= 'array' [ '[' type_spec ']' ]` is right recursive and always
consumes the `'array'` token first.

## 5. LL(1) properties

The grammar was checked for FIRST/FOLLOW conflicts; one token of lookahead is
sufficient for every decision the parser makes.

**Statement alternatives.** The FIRST sets are pairwise disjoint:

| Alternative | FIRST |
|-------------|-------|
| `expression_stmt` | `(`, `INT_LITERAL`, `STRING_LITERAL`, `CHAR_LITERAL`, `true`, `false`, `IDENT`, `!`, `-`, `+` |
| `if_stmt` | `if` |
| `while_stmt` | `while` |
| `for_stmt` | `for` |
| `return_stmt` | `return` |
| `print_stmt` | `print` |
| `block` | `{` |
| `empty_stmt` | `;` |

**`block_item ::= var_decl | statement`.** `FIRST(var_decl) = { var }` and
`var` is in none of the FIRST sets above, so the alternative is decided by one
token. This is also why `var` is a reserved word: an expression can never start
with it.

**`type_spec` alternatives** all start with a distinct keyword, so the choice
is immediate; `array` is followed by an optional `'['`, which cannot be
confused with anything else because no other production may continue a type.

**Options.** The three optional parts are also decided by one token:
`[ ':' type_spec ]` in `function_decl` is entered only on `':'` and the
alternative is `'{'`; `[ '=' expression ]` in `var_decl` on `'='` versus `';'`;
`[ 'else' statement ]` on `'else'`.

**Dangling `else`.** The ambiguous classic case
`if (c1) if (c2) s1 else s2` is resolved in the usual way: `else` binds to the
nearest preceding `if`. In a recursive-descent parser this resolution is
automatic, because the inner `parseIfStmt` is the one that consumes the
`'else'` before returning to the outer call.

**Left factoring.** Not needed: no alternative pair shares a common prefix of
more than one token.

## 6. Parsing strategy

`src/parser.cpp` implements the grammar with a hand-written recursive-descent
(LL(1)) parser: one member function per non-terminal, `peek()` for one token of
lookahead, `advance()`/`match()` for consumption. Binary operator levels are
parsed by the "loop over repetitions" shape produced by left-recursion removal,
which makes precedence and associativity explicit in the call graph:

```
parseAdditive → parseMultiplicative → parseUnary → parsePostfix → parsePrimary
```

Syntax errors are handled with panic-mode recovery (section 6 of the project
description): the parser reports the error once, then discards tokens until a
synchronization point — `;`, `}`, or the start of a new statement or
declaration (`if`, `while`, `for`, `return`, `print`, `var`, `function`) — and
resumes. Every loop in the parser also forces progress when recovery consumed
nothing, which guarantees termination even on pathological input.

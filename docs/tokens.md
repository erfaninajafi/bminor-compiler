# B-Minor lexical specification

This document states, formally, every token category of B-Minor, the regular
expression that defines it, and the rules that decide which token wins when
several rules match at the same position. `src/lexer.cpp` implements this
specification as a hand-written DFA (one state per `case` of the scanner loop),
so the specification and the code stay one-to-one.

## 1. Token record

Every emitted token carries at least:

| Field | Meaning |
|-------|---------|
| type | the token category (`KEYWORD(return)`, `IDENT`, `INT_LITERAL`, `OP(<=)`, `DELIM(;)`, `COMMENT`, `INVALID`, `EOF`, …) |
| lexeme | the exact substring of the source that matched |
| location | line and column of the first character (1-based), plus the byte offset used by the highlighter |

`bminor --tokens file.bminor` prints exactly these three fields per token.

## 2. Regular expressions

Notation: `Σ` is the ASCII character set; `[...]` is a character class; `*`, `+`,
`?` have the usual meaning; `|` is alternation; `"..."` is literal text.
`escape` is defined once and used by string and character literals.

```
letter        := [A-Za-z]
digit         := [0-9]
alnum_us      := [A-Za-z0-9_]

whitespace    := [ \t\r\n\f\v]+                       (skipped, tracked for location)
line_comment  := "//" [^\n]*                          (COMMENT)
block_comment := "/*" ( [^*] | "*" [^/] )* "*/"       (COMMENT)

keyword       := "array" | "boolean" | "char" | "else" | "false" | "for"
               | "function" | "if" | "integer" | "print" | "return" | "string"
               | "true" | "var" | "void" | "while"       (KEYWORD(x))

identifier    := letter alnum_us*                        (IDENT)
int_literal   := digit+                                  (INT_LITERAL)

escape        := "\\" [ntr0\\\"'abfv]
string_char   := [^"\\\n] | escape
string_literal:= '"' string_char* '"'                    (STRING_LITERAL)
char_literal  := "'" ( [^'\\\n] | escape ) "'"           (CHAR_LITERAL)

op_2          := "+=" | "-=" | "*=" | "/=" | "%="
               | "==" | "!=" | "<=" | ">=" | "&&" | "||"
op_1          := "+" | "-" | "*" | "/" | "%" | "=" | "<" | ">" | "!"
delimiter     := "(" | ")" | "{" | "}" | "[" | "]" | "," | ";" | ":"
```

The complete operator set is therefore

```
+  -  *  /  %  =  +=  -=  *=  /=  %=  ==  !=  <  <=  >  >=  &&  ||  !
```

and the delimiter set is `( ) { } [ ] , ; :` — the full language-specific set
required by the token table of the project description.

## 3. Priority and maximal munch

When two or more rules match at the current input position, the lexer applies
the following order:

1. **Longest match / maximal munch.** The scanner always takes the longest
   possible lexeme; ties are broken by the rule order below. Concretely, the
   two-character operators are tried before their one-character prefixes, so

   | Input | Tokens |
   |-------|--------|
   | `a<=b` | `IDENT(a)` `OP(<=)` `IDENT(b)` — never `<` `=` |
   | `a<b` | `IDENT(a)` `OP(<)` `IDENT(b)` |
   | `i+=1` | `IDENT(i)` `OP(+=)` `INT_LITERAL(1)` |
   | `x==y` | `IDENT(x)` `OP(==)` `IDENT(y)` |
   | `n!=0` | `IDENT(n)` `OP(!=)` `INT_LITERAL(0)` |
   | `a&&b` | `IDENT(a)` `OP(&&)` `IDENT(b)` |
   | `12abc` | `INVALID(12abc)` — one maximal but malformed literal, one error |

2. **Comment before division.** `//` and `/*` are recognised before the `/`
   operator, so `1/2` is a division while `1//2` starts a comment and `1/*2*/`
   is `INT_LITERAL(1)` followed by a comment.
3. **Keyword before identifier.** A lexeme that matches a reserved word is
   never an identifier: `return` is `KEYWORD(return)`, not `IDENT(return)`,
   and `iff` is `IDENT(iff)` because identifiers are matched by their full
   lexeme before the keyword table is consulted.
4. **Anything else.** A character that starts no rule produces an `INVALID`
   token; scanning then continues after that character.

The `=<` sequence mentioned in the project text is the operator `<=` written in
the wrong order: `=<` is not part of the B-Minor operator set, so `a =< b`
lexes as `=` followed by `<`. `docs/design.md` records this reading of the
example.

## 4. Escapes

| Escape | Value | Escape | Value |
|--------|-------|--------|-------|
| `\n` | newline (10) | `\\` | backslash |
| `\t` | tab (9) | `\"` | double quote |
| `\r` | carriage return (13) | `\'` | single quote |
| `\0` | NUL (0) | `\a` `\b` `\f` `\v` | bell, backspace, form feed, vertical tab |

Any other escape (`\q`, …) is reported as a warning and kept verbatim in the
literal, so the token is still produced and highlighting stays aligned with the
source.

## 5. Signed integer literals

The token table describes integer literals as "decimal signed integers". The
lexer emits unsigned decimal digits and the parser folds a directly preceding
unary `-` into the literal node, so `-42` becomes a single `IntLiteral` with
value `-42`.

Reason: including the sign in the literal's regular expression would break
context-free lexing. With `int_literal := "-"? digit+`, the expression `a-1`
would be scanned as `IDENT(a)` `INT_LITERAL(-1)` because maximal munch prefers
the longer literal, and the subtraction would silently disappear. Keeping the
sign as a unary operator (folded later, when the parser knows the context) is
the standard solution and preserves both readings:

| Source | Tokens | AST |
|--------|--------|-----|
| `-42` | `OP(-)` `INT_LITERAL(42)` | `IntLiteral(-42)` |
| `a-1` | `IDENT(a)` `OP(-)` `INT_LITERAL(1)` | `BinaryExpr(-, a, 1)` |

## 6. Lexical error handling

The lexer never raises, never exits and never stops early. For every malformed
input it emits a token, records a diagnostic with the exact location, and
continues with the next character:

| Situation | Token | Diagnostic |
|-----------|-------|------------|
| unknown character, e.g. `@` | `INVALID` | `unexpected character '@' in input` |
| lone `&` or `|` | `INVALID` | `unexpected character '&'; did you mean '&&'?` |
| `"abc` (no closing quote) | `STRING_LITERAL` up to end of line | `unterminated string literal` |
| `/* ...` (no `*/`) | `COMMENT` up to end of file | `unterminated block comment` |
| `''` | `INVALID` | `empty character literal` |
| `'ab'` | `INVALID` | `character literal must contain exactly one character` |
| `'a` (no closing quote) | `CHAR_LITERAL` | `unterminated character literal` |
| `12ab` | `INVALID` | `malformed integer literal '12ab'` |
| `"\q"` | `STRING_LITERAL` | warning: `unknown escape sequence '\q' in string literal` |

Whitespace is skipped but still tracked, so `line`/`column` stay accurate for
every following token; this is the "tracked for location; usually discarded"
behaviour required by the specification. Comments are kept as `COMMENT` tokens
because the highlighter has to colour them; the parser filters them out and
treats them as whitespace.

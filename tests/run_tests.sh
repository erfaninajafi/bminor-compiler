#!/usr/bin/env bash
# Test driver for phase one.
#
#   * every tests/valid/*.bminor must lex and parse without diagnostics;
#   * every tests/invalid/*.bminor must fail with exactly the diagnostics in
#     the matching .expected file (and must terminate, never hang or crash);
#   * the highlighter must reproduce the source exactly and must use AST level
#     information to colour function names differently from variables.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/bin/bminor"
cd "$ROOT" || exit 1

pass=0
fail=0
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

if [[ ! -x "$BIN" ]]; then
  echo "error: $BIN not found; run 'make' first" >&2
  exit 1
fi

ok() { pass=$((pass + 1)); printf '  ok   %s\n' "$1"; }
ko() { fail=$((fail + 1)); printf '  FAIL %s\n' "$1"; }

strip_ansi() { sed -E 's/\x1b\[[0-9;]*m//g'; }

echo "valid programs (must parse cleanly)"
for file in tests/valid/*.bminor; do
  name="$(basename "$file")"
  if ! "$BIN" --tokens --ast --highlight=none "$file" >"$tmp/out.txt" 2>"$tmp/err.txt"; then
    ko "$name (exit status $?)"
    sed 's/^/       /' "$tmp/err.txt"
    continue
  fi
  if [[ -s "$tmp/err.txt" ]]; then
    ko "$name (unexpected diagnostics)"
    sed 's/^/       /' "$tmp/err.txt"
    continue
  fi
  if grep -q 'INVALID' "$tmp/out.txt"; then
    ko "$name (lexer produced INVALID tokens)"
    continue
  fi
  ok "$name"
done

echo "invalid programs (must report the expected diagnostics)"
for file in tests/invalid/*.bminor; do
  name="$(basename "$file")"
  expected="${file%.bminor}.expected"
  timeout 10 "$BIN" --highlight=none "$file" >/dev/null 2>"$tmp/err.txt"
  status=$?
  if [[ $status -ne 1 ]]; then
    ko "$name (expected exit status 1, got $status)"
    continue
  fi
  if ! diff -u "$expected" "$tmp/err.txt" >"$tmp/diff.txt"; then
    ko "$name (diagnostics differ)"
    sed 's/^/       /' "$tmp/diff.txt"
    continue
  fi
  ok "$name"
done

echo "highlighting"
for file in tests/valid/*.bminor; do
  name="$(basename "$file")"
  "$BIN" --no-color "$file" >"$tmp/plain.txt" 2>/dev/null
  "$BIN" "$file" 2>/dev/null | strip_ansi >"$tmp/stripped.txt"
  if ! cmp -s "$file" "$tmp/stripped.txt"; then
    ko "$name (ANSI output is not the original source)"
    continue
  fi
  if ! cmp -s "$file" "$tmp/plain.txt"; then
    ko "$name (--no-color output differs from the source)"
    continue
  fi
  ok "$name (ANSI round-trip)"
done

"$BIN" --highlight=html --out "$tmp/hl.html" tests/valid/highlight_context.bminor 2>/dev/null
for needle in \
  '<span class="func">gcd</span>' \
  '<span class="var">limit</span>' \
  '<span class="kw">while</span>' \
  '<span class="type">integer</span>' \
  '<span class="num">48</span>' \
  '<span class="bool">true</span>' \
  '<span class="op">%</span>' \
  '<span class="op">+=</span>' \
  '<span class="com">// Context sensitive colouring: gcd is a function, a/b/limit are variables.</span>' \
  '<pre class="bminor">'; do
  if grep -qF -- "$needle" "$tmp/hl.html"; then
    ok "html contains $needle"
  else
    ko "html missing $needle"
  fi
done
if grep -q '<script' "$tmp/hl.html"; then
  ko "html must not need JavaScript"
else
  ok "html has no JavaScript"
fi

"$BIN" --highlight=html --out "$tmp/hl_err.html" tests/invalid/lexical_errors.bminor >/dev/null 2>&1
if grep -q 'class="err"' "$tmp/hl_err.html"; then
  ok "html marks invalid tokens"
else
  ko "html does not mark invalid tokens"
fi
if grep -q 'unterminated string literal' "$tmp/hl_err.html"; then
  ok "html lists diagnostics"
else
  ko "html does not list diagnostics"
fi

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[[ $fail -eq 0 ]]

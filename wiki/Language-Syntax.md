# Language Syntax

CupidScript uses a C-like surface syntax.

## Comments

Supported comment forms:

```c
// line comment

/* block comment */
```

Comments are skipped by the lexer alongside whitespace.

## Tokens

### Identifiers

* Start: letter or `_`
* Continue: letters, digits, `_`

### Integers

Decimal and hexadecimal integers with optional underscores:

```c
0
12345
1_000_000
0xFF
0xFF_FF_FF
```

* Decimal: `0-9` digits
* Hexadecimal: `0x` or `0X` followed by hex digits (`0-9`, `a-f`, `A-F`)
* Underscores `_` can appear anywhere in the number (ignored)

### Floats

Floating-point literals with decimal point:

```c
3.14
2.5
0.001
1.5e-3
2.0E+10
```

* Must contain a decimal point: `.`
* Optional exponent: `e` or `E` followed by optional `+`/`-` and digits
* Scientific notation: `1.5e-3` equals `0.0015`

### Strings

Double-quoted strings:

```c
"hello"
"with \n escapes"
"quote: \""
"backslash: \\"
```

Escape handling (in runtime unescape):

* `\n`, `\t`, `\r`, `\"`, `\\`
* Any unknown escape `\x` becomes literal `x`

### List Literals

```c
[]
[1, 2, 3]
["a", "b", "c"]
```

### Map Literals

```c
{}
{"name": "Frank", "age": 30}
{"key": value_expr}
```

Keys must be string expressions (can be any expression that evaluates to a string).

## Operators & Punctuation

### Punctuation

`(` `)` `[` `]` `{` `}` `,` `;` `.`

### Operators

Assignment:

* `=`
* `+=`, `-=`, `*=`, `/=` (compound assignment)

Arithmetic:

* `+ - * / %`

Unary:

* `! -`

Comparison:

* `== != < <= > >=`

Boolean:

* `&& ||`

Range:

* `..` (exclusive range, e.g., `0..5` → `[0,1,2,3,4]`)
* `..=` (inclusive range, e.g., `0..=5` → `[0,1,2,3,4,5]`)

## Precedence (high to low)

1. Primary: literals, identifiers, parenthesized `(expr)`, list literals `[...]`, map literals `{...}`
2. Postfix: calls `f(...)`, indexing `a[b]`, field access `a.b`
3. Unary: `!expr`, `-expr`
4. Multiplicative: `* / %`
5. Additive: `+ -`
6. Range: `start..end`, `start..=end`
7. Comparison: `< <= > >=`
8. Equality: `== !=`
9. AND: `&&`
10. OR: `||`
11. Ternary: `cond ? then : else`

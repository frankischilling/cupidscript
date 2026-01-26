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

Raw (backtick) strings:

```c
`C:\\temp\\file`
`line1
line2`
```

Notes:

* Backtick strings do not process escapes.
* Backtick strings can span multiple lines.
* String interpolation is not supported inside backtick strings.
* A backtick cannot appear inside a backtick string.

String interpolation is supported with `${...}` inside a string:

```c
let name = "Ada";
print("Hello ${name}");
print("sum=${1 + 2}");
```

Escape handling (in runtime unescape):

* `\n`, `\t`, `\r`, `\"`, `\\`
* `\e` (ASCII ESC, 27)
* `\xHH` (single byte from two hex digits)
* Any unknown escape `\x` becomes literal `x`

### List Literals

```c
[]
[1, 2, 3]
["a", "b", "c"]
```

Trailing commas are allowed:

```c
[1, 2, 3,]
[]
```

### Map Literals

```c
{}
{"name": "Frank", "age": 30}
{"key": value_expr}
```

Trailing commas are allowed:

```c
{"a": 1, "b": 2,}
{}
```

Keys must be string expressions (can be any expression that evaluates to a string).

## Functions

Named functions:

```c
fn add(a, b = 1) {
  return a + b;
}
```

Function literals:

```c
let inc = fn(x, step = 1) { return x + step; };
```

Defaults use `=` in the parameter list. Required parameters must come before default parameters. Missing arguments use their default values.

## Arrow Functions

Arrow syntax supports concise single-expression functions:

```c
let double = fn(x) => x * 2;
let add = fn(a, b) => a + b;
```

Block-bodied arrows require explicit `return`:

```c
let f = fn(x) => { return x + 1; };
```

## Spread & Rest

Spread (`...`) expands lists and maps:

```c
let a = [1, 2, 3];
let b = [0, ...a, 4];

let defaults = {theme: "dark", size: 12};
let config = {...defaults, size: 14};
```

Rest parameters collect remaining arguments into a list:

```c
fn log_all(prefix, ...items) {
  for item in items { print(prefix, item); }
}
```

## Pipe Operator

The pipe operator passes the left value into the right call.

```c
fn add(a, b) { return a + b; }

let r1 = 10 |> add(5);     // add(10, 5)
let r2 = 10 |> add(5, _);  // add(5, 10)
```

If `_` is present, it marks where the left value should go. Otherwise, the left value becomes the first argument.

## Classes

Class declarations define constructors and methods with single inheritance.

```c
class File {
  fn new(path) { self.path = path; }
  fn is_hidden() { return starts_with(self.path, "."); }
}

class ImageFile : File {
  fn new(path) { super.new(path); self.is_img = ends_with(path, ".png"); }
  fn is_image() { return self.is_img; }
}

let f = File(".secret");
let img = ImageFile("cat.png");
```

* `self` refers to the instance inside methods.
* `super` refers to the parent class inside methods.
* Instances are created by calling the class as a function.

## Operators & Punctuation

### Punctuation

`(` `)` `[` `]` `{` `}` `,` `;` `.`

### Operators

Assignment:

* `=`
* `+=`, `-=`, `*=`, `/=` (compound assignment, works on variables, fields, and index expressions)

Arithmetic:

* `+ - * / %`

Unary:

* `! -`

Comparison:

* `== != < <= > >=`

Boolean:

* `&& ||`

Nullish coalescing:

* `??` (returns left operand if not `nil`, otherwise right)

Range:

* `..` (exclusive range, e.g., `0..5` → `[0,1,2,3,4]`)
* `..=` (inclusive range, e.g., `0..=5` → `[0,1,2,3,4,5]`)

## Precedence (high to low)

1. Primary: literals, identifiers, parenthesized `(expr)`, list literals `[...]`, map literals `{...}`
2. Postfix: calls `f(...)`, indexing `a[b]`, field access `a.b` (field access works after any expression, e.g. `(expr).field`)
3. Unary: `!expr`, `-expr`
4. Multiplicative: `* / %`
5. Additive: `+ -`
6. Range: `start..end`, `start..=end`
7. Comparison: `< <= > >=`
8. Equality: `== !=`
9. AND: `&&`
10. OR: `||`
11. Nullish: `a ?? b`
12. Ternary: `cond ? then : else`

### Optional chaining

Optional chaining accesses a field only if the target is not `nil`:

```c
user?.name
```

If the target is `nil`, the result is `nil` (no error). Otherwise, it behaves like normal field access.

## `switch` Statement

CupidScript supports `switch` as a statement:

```c
switch (x) {
  case 1 { print("one"); }
  case 2 { print("two"); }
  default { print("other"); }
}
```

Notes:

* Cases are compared using the same equality rules as `==` for basic types.
* Non-fallthrough: only the first matching case runs.

## `defer` Statement

Schedule cleanup work to run when the current block exits:

```c
defer close(file);
defer { print("cleanup"); }
```

Notes:

* Defers run in LIFO order when the block exits.
* Defers run even if the block exits via `return`, `break`, `continue`, or `throw`.

## `match` Expression

`match` is like `switch`, but returns a value and is used inside expressions.

```c
let label = match (x) {
  case 1: "one";
  case 2: "two";
  default: "other";
};
```

Patterns and guards:

```c
let label = match (value) {
  case [a, b]: a + b;
  case {x, y: z}: x + z;
  case n if n > 10: "big";
  case _: "small";
};
```

Notes:

* Patterns can bind identifiers; bindings are visible in the guard and case result.
* Guards (`if expr`) are optional and run only when the pattern matches.
* The first matching case expression is evaluated and returned.
* `default` is optional; if omitted and no case matches, the result is `nil`.

## `try` / `catch` / `finally` Statements

Syntax:

```c
try {
  // body
} catch (e) {
  // handler
} finally {
  // cleanup
}

// finally can also be a single expression:
try { work(); } catch (e) { print(e); } finally cleanup();
```

Notes:

* `catch` is required; `finally` is optional.
* `finally` executes after the `try`/`catch` regardless of `return` or `throw`.
* `finally` can be a block or a single expression statement.

## `export` Statements

Syntax:

```c
export name = expr;
export {name, local as exported};
```

Defines a value in the module's implicit `exports` map. `require()` returns this map.

## `import` Statements

Syntax:

```c
import "./path/to/module.cs";           // side effects only
import mod from "./path/to/module.cs";  // bind exports map
import {foo, bar as baz} from "./path/to/module.cs";
import mod, {foo} from "./path/to/module.cs";
```

Notes:

* `import` is syntax sugar over `require()`.
* Named imports read fields from the module's exports map.
* If an imported name is missing, the binding is set to `nil`.

## `const` Bindings

Use `const` to create an immutable binding:

```c
const x = 10;
```

Notes:

* Reassignment to a `const` binding raises a runtime error.
* `const` requires an initializer.
* Destructuring also works with `const`:

```c
const [a, b] = [1, 2];
const {name, age} = {"name": "Ada", "age": 36};
```

## Destructuring `let`

List and map destructuring are supported in `let` declarations.

### List destructuring

```c
let [a, b, c] = [1, 2];
// a=1, b=2, c=nil
```

### Map destructuring

```c
let {x, y} = {"x": 10, "y": 20};
// x=10, y=20

let {key: alias} = {"key": "value"};
// alias="value"
```

Notes:

* Missing list elements or map keys produce `nil`.
* `_` can be used to ignore a binding.
* Destructuring requires an initializer.
* List destructuring expects a list; map destructuring expects a map (runtime error otherwise).

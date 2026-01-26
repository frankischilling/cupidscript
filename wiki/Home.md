# CupidScript Wiki

CupidScript is a small embeddable scripting language with:

* A C lexer + recursive-descent parser
* A tree-walking VM (interpreter)
* Closures (function literals capture their environment)
* Basic built-in types: `nil`, `bool`, `int`, `float`, `string`, `list`, `map`, `strbuf`
* A practical standard library for scripts (printing, strings, collections, paths, math, time)

## Quick Example

```c
// example.cs
fn add(a, b) { return a + b; }

let xs = list();
push(xs, 10);
push(xs, 32);

print("sum:", add(xs[0], xs[1]));
```

## Language at a Glance

### Keywords

`let`, `fn`, `if`, `else`, `while`, `switch`, `case`, `default`, `for`, `in`, `break`, `continue`, `return`, `throw`, `try`, `catch`, `true`, `false`, `nil`

### Expressions

* Literals: `123`, `0xFF`, `1_000`, `3.14`, `1.5e-3`, `"hello"`, `true`, `false`, `nil`, `[1, 2, 3]`, `{"key": "value"}`
* Arithmetic: `+ - * / %`
* Comparisons: `< <= > >= == !=`
* Boolean: `&& || !`
* Range: `start..end` (exclusive), `start..=end` (inclusive)
* Ternary: `cond ? then_val : else_val`
* Calls: `f(a, b)`
* Indexing: `xs[0]`, `m["key"]`
* Field access: `obj.field` (maps) + dotted global fallback
* Function literals: `fn(a, b) { ... }`

### Statements

* `let name = expr;`
* `name = expr;` (assign)
* `name += expr;` (compound assign: `+=`, `-=`, `*=`, `/=`)
* `xs[i] = expr;` (setindex)
* `if (cond) { ... } else { ... }`
* `while (cond) { ... }`
* `for name in iterable { ... }` (for-in loops)
* `for (init; cond; incr) { ... }` (C-style for loops)
* `break;` / `continue;`
* `throw expr;`
* `try { ... } catch (e) { ... }`
* `return expr;`
* `expr;` (expression statement)

## Wiki Pages

* [Getting Started](Getting-Started)
* [Language Syntax](Language-Syntax)
* [Types & Values](Types-and-Values)
* [Control Flow](Control-Flow)
* [Functions & Closures](Functions-and-Closures)
* [Collections (list/map)](Collections)
* [Strings & strbuf](Strings-and-Strbuf)
* [Standard Library](Standard-Library)
* [Modules (load/require) & Paths](Modules-and-Paths)
* [Errors & Stack Traces](Errors-and-Debugging)
* [Host Safety Controls](Host-Safety-Controls)
* [Implementation Notes (Lexer/Parser/VM)](Implementation-Notes)

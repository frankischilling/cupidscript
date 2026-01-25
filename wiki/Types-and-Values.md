# Types & Values

CupidScript is dynamically typed.

## Built-in Types

### `nil`

Represents "no value".

* Printed as `nil`
* Falsy in conditionals

### `bool`

`true` / `false`

* Only `false` is falsy (besides `nil`)

### `int`

Signed integer (implemented as 64-bit in the VM: `int64_t`).

### `float`

Floating-point number (implemented as 64-bit double: `double`).

* Float literals: `3.14`, `2.5`, `1.0e-3`
* Arithmetic with mixed int/float returns float
* Division `/` always returns float for precision
* Comparisons work between int and float

### `string`

Heap-allocated refcounted string (`cs_string`).

* `+` concatenates if either operand is a string

### `list`

Dynamic array of `cs_value`.

* Indexing: `xs[i]` where `i` is int
* Out-of-range or negative index returns `nil`

### `map`

Simple key/value map with **string keys**.

* Indexing: `m["key"]`
* Missing keys return `nil`
* Field access is sugar: `m.key` reads `"key"` from map

### `strbuf`

Mutable string builder object.

* Has method-like calls: `sb.append(x)`, `sb.str()`, `sb.clear()`, `sb.len()`

## Truthiness Rules

Used by `if` and `while`, and by `!`, `&&`, `||`:

* `nil` → false
* `bool` → its value
* everything else → true

That means `0` is truthy (because it's an `int`, not `nil`/`bool`).

## Equality

`==` / `!=` behavior:

* If types differ → not equal
* `nil` equals `nil`
* `bool`, `int`, `string` compare by value
* Other heap objects compare by pointer identity (same object)

## Comparisons

`< <= > >=` only work when:

* both operands are `int` or `float` (can be mixed), or
* both operands are `string`

Otherwise runtime error: comparisons require both numbers or both strings.

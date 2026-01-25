# Control Flow

## Blocks

Blocks are `{ ... }` and appear in:

* function bodies
* `if` / `else`
* `while`

A block contains a sequence of statements.

## `if` / `else`

Syntax:

```c
if (cond) { 
  // then
} else {
  // else
}
```

Notes:

* Parentheses are required: `if ( ... )`
* `else` block is optional

## `while`

Syntax:

```c
while (cond) {
  // body
}
```

Notes:

* Parentheses are required: `while ( ... )`
* Condition is re-evaluated each iteration

## `for...in`

Syntax:

```c
for name in iterable {
  // body
}
```

Iterates over:

* Lists: iterates over elements
* Maps: iterates over keys (use `keys(m)`, `values(m)`, or `items(m)`)

Example:

```c
let xs = [10, 20, 30];
for x in xs {
  print(x);
}

// Using range() for numeric iteration
for i in range(10) {
  print(i); // 0 through 9
}

for i in range(2, 8, 2) {
  print(i); // 2, 4, 6
}

let m = {"a": 1, "b": 2};
for pair in items(m) {
  print(pair[0], "=", pair[1]);
}
```

## C-Style `for` Loops

Syntax:

```c
for (init; condition; increment) {
  // body
}
```

All three parts are optional. This enables traditional C-style loops:

```c
// Basic counter loop
for (i = 0; i < 10; i = i + 1) {
  print(i);
}

// Multiple initializations (using comma isn't supported, use separate statements)
let i = 0;
let j = 10;
for (; i < j; i = i + 1) {
  print(i, j);
  j = j - 1;
}

// Infinite loop
for (;;) {
  if (some_condition()) {
    break;
  }
}
```

## Range Operator

Syntax:

* `start..end` - Exclusive range (up to but not including end)
* `start..=end` - Inclusive range (up to and including end)

Creates a list of integers from `start` to `end`:

```c
let nums = 0..5;      // [0, 1, 2, 3, 4]
let incl = 0..=5;     // [0, 1, 2, 3, 4, 5]
let down = 10..5;     // [10, 9, 8, 7, 6]
let down_incl = 10..=5; // [10, 9, 8, 7, 6, 5]

// Use with for-in loops
for i in 0..10 {
  print(i); // 0 through 9
}

for i in 1..=100 {
  print(i); // 1 through 100
}
```

The range operator works in both directions (ascending and descending) automatically.

## `return`

Syntax:

```c
return;
return expr;
```

Return can appear inside any block; it returns from the current function call.

## `break` / `continue`

* `break;` exits the innermost loop
* `continue;` skips to the next iteration

Valid in `while`, `for...in`, and C-style `for` loops.

Example:

```c
while (true) {
  let x = pop(xs);
  if (x == nil) { break; }
  if (x < 0) { continue; }
  print(x);
}
```

## Ternary Expression

Syntax:

```c
cond ? then_val : else_val
```

Evaluates `cond`. If truthy, returns `then_val`, otherwise `else_val`.

Example:

```c
let max = a > b ? a : b;
```

## Exception Handling

### `throw`

Syntax:

```c
throw expr;
```

Throws an exception. The value can be any type (commonly a string or map).

Example:

```c
if (x < 0) {
  throw "negative value not allowed";
}

throw {"msg": "error", "code": 404};
```

### `try / catch`

Syntax:

```c
try {
  // code that might throw
} catch (e) {
  // handle exception
}
```

If code in the `try` block throws, execution jumps to the `catch` block with the thrown value in variable `e`.

Example:

```c
try {
  do_risky_operation();
} catch (e) {
  print("Error:", e);
}
```

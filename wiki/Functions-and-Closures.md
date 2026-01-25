# Functions & Closures

CupidScript supports **named functions** and **function literals**.

## Named Functions: `fn name(...) { ... }`

Syntax:

```c
fn add(a, b) {
  return a + b;
}
```

* Functions are values (callable)
* Parameters are identifiers
* The body is a block `{ ... }`

## Function Literals: `fn (...) { ... }`

Syntax:

```c
let f = fn(x) { return x + 1; };
print(f(10));
```

Function literals capture the current environment (closure). This is implemented by storing `closure = env` when the function value is created.

## Calls

Syntax:

```c
f();
f(1, 2, 3);
```

Argument count must match exactly for user-defined functions (runtime error on mismatch).

## Closures Example

```c
fn counter() {
  let i = 0;
  return fn() {
    i = i + 1;
    return i;
  };
}

let c = counter();
print(c()); // 1
print(c()); // 2
```

`i` lives in the captured environment and persists across calls.

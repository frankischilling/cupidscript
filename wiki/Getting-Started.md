# Getting Started

This wiki describes the *language behavior* implemented by the current lexer/parser/VM/stdlib.

## Running Scripts

CupidScript is designed to be embedded. Typical host flow:

1. Create a `cs_vm`
2. Register stdlib (`cs_register_stdlib(vm)`)
3. Run a script (string or file)

Your stdlib includes:

* `load("file.cs")` to run a file (relative to VM "current directory" stack)
* `require("file.cs")` to run-once include (tracks already-required paths)
* `require_optional("file.cs")` to conditionally load a file (returns `nil` if not found)

## Hello World

```c
print("hello from cupidscript");
```

## Common Patterns

### Variables + arithmetic

```c
let a = 10;
let b = 20;
print(a + b);

let pi = 3.14159;
let radius = 5.0;
let area = pi * radius * radius;
print("circle area:", area);
```

### Lists

```c
let xs = list();
push(xs, 1);
push(xs, 2);
print("len:", len(xs));
print(xs[0], xs[1]);
```

### Maps

```c
let m = map();
mset(m, "name", "Frank");
print(mget(m, "name"));
print(mhas(m, "name"));
print(keys(m));
```

### While loops

```c
let i = 0;
while (i < 5) {
  print("i =", i);
  i = i + 1;
}
```

### For-in loops

```c
let xs = [1, 2, 3];
for x in xs {
  print("x =", x);
}

for i in range(5) {
  print("i =", i); // 0, 1, 2, 3, 4
}

let m = {"a": 10, "b": 20};
for k in keys(m) {
  print(k, "=", m[k]);
}
```

### C-style for loops

```c
// Traditional counter loop
for (i = 0; i < 10; i = i + 1) {
  print(i);
}

// Range operator for concise iteration
for i in 0..10 {
  print(i); // 0 through 9
}

for i in 0..=10 {
  print(i); // 0 through 10 (inclusive)
}
```

### Functions + closures

```c
fn make_adder(x) {
  return fn(y) { return x + y; };
}

let add10 = make_adder(10);
print(add10(5)); // 15
```

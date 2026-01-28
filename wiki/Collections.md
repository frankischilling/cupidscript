# Collections (list / map / set)

## Table of Contents

- [Lists](#lists)
- [Maps](#maps)
- [Sets](#sets)
- [Comprehensions](#comprehensions)
- [Destructuring](#destructuring)
- [Data Quality-of-Life Functions](#data-quality-of-life-functions)

## Lists

### Create

```c
let xs = list();
let ys = [];           // empty list literal
let zs = [1, 2, 3];    // list literal with elements
```

### Spread

Use `...` to expand a list literal:

```c
let a = [1, 2, 3];
let b = [0, ...a, 4];  // [0, 1, 2, 3, 4]
```

### Push / Pop

```c
push(xs, 123);
let v = pop(xs);
```

* `push(list, value)` appends
* `pop(list)` returns last element or `nil` if empty

### Length

```c
len(xs)
```

### Indexing

```c
xs[0]
xs[i]
```

Rules:

* index must be `int`
* negative or out of range returns `nil`

### Assigning by index

```c
xs[2] = 999;
```

This can grow the list:

* Missing intermediate elements become `nil`

### Insert / Remove

```c
insert(xs, index, value)  // insert at position
remove(xs, index)         // remove and return element at index
```

### Slice

```c
let sub = slice(xs, start, length);
```

Returns a new list with elements from `start` (inclusive) for `length` elements.

### Extend

```c
extend(xs, ys); // appends all elements of ys into xs
```

### Index Of

```c
let i = index_of(xs, 123); // returns index or -1

### Utilities

```c
list_unique(xs)   // remove duplicates (preserve order)
list_flatten(xs)  // flatten one level
list_chunk(xs, 2) // [[...], ...]
list_compact(xs)  // remove nils
list_sum(xs)      // sum numbers (nil ignored)
```
```

### Sort

```c
sort(xs); // in-place, default comparison (insertion)
sort(xs, "quick");
sort(xs, "merge");

fn desc(a, b) { return b - a; }
sort(xs, desc, "quick"); // custom comparator + algorithm
```

## Maps

Maps accept **any value** as a key.

Key equality follows `==`:

* `int` and `float` compare by numeric value (so `1` equals `1.0`)
* `string` compares by content
* Lists/maps/functions compare by identity (same object)

### Create

```c
let m = map();
let m2 = {};                          // empty map literal
let m3 = {"name": "Frank", "age": 30}; // map literal
let m4 = {1: "one", true: "yes", nil: "none"};
```

### Spread

Use `...` to expand a map literal (later keys override earlier keys):

```c
let defaults = {theme: "dark", size: 12};
let config = {...defaults, size: 14};
```

### Set / Get

## Comprehensions

CupidScript supports **list and map comprehensions** for concise collection creation:

### List Comprehensions

Transform and filter iterables in a single expression:

```c
// Basic syntax: [expression for variable in iterable]
let squares = [x * x for x in range(10)]
// [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

// With filter: [expression for variable in iterable if condition]
let evens = [x for x in range(20) if x % 2 == 0]
// [0, 2, 4, 6, 8, 10, 12, 14, 16, 18]

// From lists
let words = ["hello", "world", "cupid"]
let uppercase = [w.upper() for w in words]
// ["HELLO", "WORLD", "CUPID"]

// From maps
let data = {a: 1, b: 2, c: 3}
let pairs = [k + ":" + str(v) for k, v in data]
// ["a:1", "b:2", "c:3"]
```

### Map Comprehensions

Create or transform maps:

```c
// Basic syntax: {key_expr: value_expr for key_var, value_var in iterable}
let numbers = {a: 1, b: 2, c: 3}
let doubled = {k: v * 2 for k, v in numbers}
// {a: 2, b: 4, c: 6}

// With filter
let high_values = {k: v for k, v in numbers if v > 1}
// {b: 2, c: 3}

// From list to map
let fruits = ["apple", "banana", "cherry"]
let indexed = {i: fruit for i, fruit in fruits}
// {0: "apple", 1: "banana", 2: "cherry"}
```

### Nested Comprehensions

Create multi-dimensional structures:

```c
// Multiplication table
let table = [[i * j for j in range(1, 6)] for i in range(1, 6)]

// Coordinate pairs
let coords = [(x, y) for x in range(3) for y in range(3)]
// [(0,0), (0,1), (0,2), (1,0), ...]
```

**See [Comprehensions](COMPREHENSIONS.md) for complete documentation.**

## Destructuring

Lists and maps can be destructured in `let` declarations:

```c
let [a, b] = [1, 2];
let {x, y} = {"x": 10, "y": 20};
let {key: alias} = {"key": "value"};
```

Rest patterns capture remaining elements:

```c
let [a, b, ...rest] = [1, 2, 3, 4];
let {x, ...other} = {x: 10, y: 20, z: 30};
```

Missing entries produce `nil`. Use `_` to ignore a binding.

```c
mset(m, "name", "Frank");
print(mget(m, "name"));

// non-string keys
mset(m, 42, "answer");
print(mget(m, 42));
```

* Missing key returns `nil`

### Has Key

```c
mhas(m, "name") // bool
mhas(m, 42)      // bool
```

### Keys

```c
let ks = keys(m); // list of keys (any value)
```

### Values

```c
let vs = values(m); // list of values
```

### Items (key-value pairs)

```c
let pairs = items(m); // list of [key, value] lists
```

### Map Iteration

## Sets

Sets store unique values (based on `==`).

### Create

```c
let s = set();
let s2 = set([1, 2, 2, 3]); // duplicates ignored
let s3 = set({a: 1, b: 2}); // keys from map
```

### Add / Has / Del

```c
set_add(s, 10);   // true if inserted
set_has(s, 10);   // true if present
set_del(s, 10);   // true if removed
```

### Values

```c
let xs = set_values(s); // list of values (order unspecified)
```

### Iteration

```c
for v in s {
  print(v);
}
```

Additional helpers for map iteration:

* `map_values(m)` - Get all values as a list

```c
let m = {"a": 1, "b": 2, "c": 3};
for v in map_values(m) {
  print("Value:", v);
}
```

### Iteration Helpers

```c
let xs = [1, 2, 3];
print(enumerate(xs)); // [[0,1], [1,2], [2,3]]

print(zip(["a", "b"], [10, 20, 30])); // [["a",10], ["b",20]]

fn is_even(x) { return x % 2 == 0; }
print(any(xs, is_even)); // true
print(all(xs, is_even)); // false

print(filter(xs, is_even)); // [2]
print(map(xs, fn(x) => x * 2)); // [2, 4, 6]

fn sum(a, b) { return a + b; }
print(reduce(xs, sum)); // 6
```

## Data Quality-of-Life Functions

### Copy and Deep Copy

* `copy(x)` - Shallow copy of list or map
* `deepcopy(x)` - Deep copy with cycle detection

```c
let original = [1, 2, [3, 4]];
let shallow = copy(original);
let deep = deepcopy(original);

shallow[2][0] = 99;  // Affects original
deep[2][1] = 88;     // Does not affect original
```

### Reverse

* `reverse(list)` - In-place reversal
* `reversed(list)` - Returns new reversed list

```c
let list = [1, 2, 3, 4, 5];
reverse(list);  // list is now [5, 4, 3, 2, 1]

let rev = reversed([1, 2, 3]);  // Returns [3, 2, 1], original unchanged
```

### Contains

* `contains(container, item)` - Check if item exists

Works with lists, maps (checks keys), and strings:

```c
print(contains([1, 2, 3], 2));           // true
print(contains({a: 1, b: 2}, "a"));      // true
print(contains("hello world", "world")); // true
```

### Delete Key

```c
mdel(m, "key"); // removes the key
```

### Indexing with keys

```c
m["name"]
m[42]
```

### Field access on maps

If `m` is a map, `m.name` reads `m["name"]`.

If `m` is *not* a map, field access is a runtime error.

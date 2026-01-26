# Collections (list / map)

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

Additional helpers for map iteration:

* `map_values(m)` - Get all values as a list

```c
let m = {"a": 1, "b": 2, "c": 3};
for v in map_values(m) {
  print("Value:", v);
}
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

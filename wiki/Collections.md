# Collections (list / map)

## Lists

### Create

```c
let xs = list();
let ys = [];           // empty list literal
let zs = [1, 2, 3];    // list literal with elements
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

## Maps

Maps use **string keys**.

### Create

```c
let m = map();
let m2 = {};                          // empty map literal
let m3 = {"name": "Frank", "age": 30}; // map literal
```

### Set / Get

```c
mset(m, "name", "Frank");
print(mget(m, "name"));
```

* Missing key returns `nil`

### Has Key

```c
mhas(m, "name") // bool
```

### Keys

```c
let ks = keys(m); // list of strings
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

### Indexing with strings

```c
m["name"]
```

### Field access on maps

If `m` is a map, `m.name` reads `m["name"]`.

If `m` is *not* a map, field access is a runtime error.

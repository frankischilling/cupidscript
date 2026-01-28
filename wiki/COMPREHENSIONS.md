# Comprehensions

Comprehensions provide a concise, readable syntax for creating lists and maps by transforming and filtering existing collections. They bring functional programming elegance to CupidScript with a familiar Python-like syntax.

## Table of Contents

- [Overview](#overview)
- [List Comprehensions](#list-comprehensions)
  - [Basic Syntax](#basic-syntax)
  - [With Filters](#with-filters)
  - [From Different Iterables](#from-different-iterables)
- [Map Comprehensions](#map-comprehensions)
  - [Basic Map Comprehensions](#basic-map-comprehensions)
  - [Transforming Maps](#transforming-maps)
  - [Creating Maps from Lists](#creating-maps-from-lists)
- [Nested Comprehensions](#nested-comprehensions)
- [Advanced Patterns](#advanced-patterns)
  - [Multiple Filters](#multiple-filters)
  - [Function Calls](#function-calls)
  - [Complex Expressions](#complex-expressions)
- [Comparison with Loops](#comparison-with-loops)
- [Best Practices](#best-practices)
- [Performance Notes](#performance-notes)
- [Examples](#examples)
- [See Also](#see-also)

## Overview

Comprehensions transform the common pattern of:
1. Create empty collection
2. Loop over source
3. Transform/filter elements
4. Add to collection

Into a single, expressive line of code.

**Benefits:**
- More readable than loops for simple transformations
- Familiar to Python/JavaScript developers
- Functional programming style
- Less boilerplate code
- Clear intent

## List Comprehensions

### Basic Syntax

```cupidscript
// General form:
// [expression for variable in iterable]

let squares = [x * x for x in range(10)]
// [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

let doubled = [n * 2 for n in [1, 2, 3, 4, 5]]
// [2, 4, 6, 8, 10]

let doubled = [n * 2 for n in [1, 2, 3, 4, 5]]
// [2, 4, 6, 8, 10]
```

### With Filters

Add an `if` clause to filter elements:

```cupidscript
// [expression for variable in iterable if condition]

let evens = [x for x in range(20) if x % 2 == 0]
// [0, 2, 4, 6, 8, 10, 12, 14, 16, 18]

let long_words = [w for w in words if len(w) > 5]

// Complex filter with AND logic
let special = [x for x in range(100) if (x % 3 == 0 && x % 5 != 0)]
```

### From Different Iterables

Comprehensions work with lists, maps, and ranges:

```cupidscript
// From range
let numbers = [i for i in range(1, 6)]
// [1, 2, 3, 4, 5]

// From list
let items = ["a", "b", "c"]
let indexed = [i + ":" + item for i, item in items]

// From map (iterate over keys)
let map_data = {a: 1, b: 2, c: 3}
let keys = [k for k in map_data]

// From map (key-value pairs)
let pairs = [k + "=" + str(v) for k, v in map_data]
```

## Map Comprehensions

### Basic Map Comprehensions

```cupidscript
// General form:
// {key_expr: value_expr for var in iterable}
// {key_expr: value_expr for key_var, val_var in iterable}  // For maps

let numbers = {a: 1, b: 2, c: 3}
let doubled = {k: v * 2 for k, v in numbers}
// {a: 2, b: 4, c: 6}

// With filter
let high_values = {k: v for k, v in numbers if v > 1}
// {b: 2, c: 3}
```

### Transforming Maps

Transform both keys and values:

```cupidscript
let data = {a: 1, b: 2, c: 3}

// Transform keys
let upper_keys = {k.upper(): v for k, v in data}
// {A: 1, B: 2, C: 3}

// Transform both
let transformed = {k + "_new": v * 10 for k, v in data}
// {a_new: 10, b_new: 20, c_new: 30}

// Swap keys and values
let swapped = {v: k for k, v in data}
// {1: "a", 2: "b", 3: "c"}
```

### Creating Maps from Lists

```cupidscript
// Single variable iteration over range
let squares_map = {i: i * i for i in range(5)}
// {0: 0, 1: 1, 2: 4, 3: 9, 4: 16}

// Two variable iteration over map
let fruits_map = {0: "apple", 1: "banana", 2: "cherry"}
let swapped = {v: k for k, v in fruits_map}
// {apple: 0, banana: 1, cherry: 2}

// Transform keys and values
let doubled_map = {k * 10: v * 2 for k, v in fruits_map if k > 0}
// {10: "bananabanana", 20: "cherrycherry"}
```

## Nested Comprehensions

Comprehensions can be nested to create multi-dimensional structures:

```cupidscript
// Multiplication table
let table = [[i * j for j in range(1, 6)] for i in range(1, 6)]
// [[1,2,3,4,5], [2,4,6,8,10], ...]

// Nested comprehensions for coordinate generation
let rows = [[x, y] for x in range(3)]  
let coords = [row for row in [[x, y] for x in range(3) for y in [0, 1, 2]]]
// Note: For complex nested iteration, use explicit loops

// Nested maps
let grid = {i: {j: i * 10 + j for j in range(3)} for i in range(3)}
// {0: {0:0, 1:1, 2:2}, 1: {0:10, 1:11, 2:12}, ...}
```

## Advanced Patterns

### Complex Filters

Combine multiple conditions with logical operators:

```cupidscript
let filtered = [
    x for x in range(100)
    if (x % 2 == 0 && x % 5 != 0 && x > 10)
]
```

### Function Calls

Use functions in expressions or filters:

```cupidscript
fn square(x) { return x * x }
fn is_prime(x) { /* ... */ }

let squared = [square(x) for x in range(10)]

let primes = [x for x in range(2, 100) if is_prime(x)]

// Function calls in expressions
let lengths = [len(s) for s in strings]
let processed = [transform(s) for s in strings]
```

### Complex Expressions

Comprehensions support any valid expression:

```cupidscript
// Ternary operator
let abs_values = [x if x >= 0 else -x for x in numbers]

// String interpolation
let greetings = ["Hello, ${name}!" for name in names]

// Arithmetic
let converted = [(f - 32) * 5 / 9 for f in fahrenheit]

// Tuple creation
let points = [(x: i, y: i * i) for i in range(5)]
```

## Comparison with Loops

**Traditional loop:**
```cupidscript
let squares = []
for x in range(10) {
    push(squares, x * x)
}
```

**Comprehension:**
```cupidscript
let squares = [x * x for x in range(10)]
```

**When to use comprehensions:**
- ✅ Simple transformations
- ✅ Simple filtering
- ✅ Creating new collections
- ✅ Functional programming style

**When to use loops:**
- ❌ Complex logic
- ❌ Multiple statements per iteration
- ❌ Early exit conditions (break/continue)
- ❌ Side effects (printing, modifying external state)

## Best Practices

### 1. Keep It Simple

```cupidscript
// Good - clear and concise
let evens = [x for x in numbers if x % 2 == 0]

// Bad - too complex, use a loop
let result = [
    process(transform(x)) if validate(x) and check(x) else default(x)
    for x in data
    if complex_condition(x)
]
```

### 2. Use Meaningful Variable Names

```cupidscript
// Good
let user_ids = [user["id"] for user in users]

// Less clear
let ids = [u["id"] for u in users]
```

### 3. Limit Nesting

```cupidscript
// Good - clear nested comprehension
let matrix = [[i * j for j in range(5)] for i in range(5)]

// Avoid - too deeply nested
let result = [[[f(x, y, z) for z in a] for y in b] for x in c]
```

### 4. Consider Readability

```cupidscript
// If comprehension is hard to read, use a loop:
let result = []
for item in items {
    if (complex_condition(item)) {
        let processed = transform(item)
        push(result, processed)
    }
}
```

### 5. Don't Abuse for Side Effects

```cupidscript
// Bad - side effects in comprehension
let _ = [print(x) for x in items]  // Don't do this!

// Good - use a loop for side effects
for item in items {
    print(item)
}
```

## Performance Notes

### Eager Evaluation

Comprehensions are **eagerly evaluated** - they immediately create the full collection:

```cupidscript
let big = [x for x in range(1000000)]  // Creates all million elements now
```

For lazy evaluation, use explicit loops or generators (if available).

### Memory Efficiency

```cupidscript
// Memory efficient - processes one at a time
for x in range(1000000) {
    if (x % 1000 == 0) {
        print(x)
    }
}

// Less efficient - creates full filtered list
let filtered = [x for x in range(1000000) if x % 1000 == 0]
for x in filtered {
    print(x)
}
```

### Optimization Tips

1. **Filter early** - put filters in comprehension rather than filtering after
   ```cupidscript
   // Better
   let result = [process(x) for x in data if is_valid(x)]
   
   // Worse  
   let temp = [process(x) for x in data]
   let result = [x for x in temp if is_valid(x)]
   ```

2. **Avoid redundant comprehensions**
   ```cupidscript
   // Inefficient - two passes
   let temp = [x * 2 for x in data]
   let result = [x + 1 for x in temp]
   
   // Better - one pass
   let result = [x * 2 + 1 for x in data]
   ```

3. **Use appropriate data structures**
   ```cupidscript
   // For lookups, create map
   let index = {item: true for i, item in list}
   
   // Then check membership quickly
   if (index[x] == true) { /* ... */ }
   ```

## Examples

See the [examples directory](../examples/):
- [comprehensions.cs](../examples/comprehensions.cs) - Basic comprehension patterns
- [comprehensions_advanced.cs](../examples/comprehensions_advanced.cs) - Advanced techniques and real-world examples

## See Also

- [Collections](Collections.md) - Lists, maps, and sets
- [Control Flow](Control-Flow.md) - For loops and iteration
- [Functions and Closures](Functions-and-Closures.md) - Functional programming
- [Language Syntax](Language-Syntax.md) - General syntax reference

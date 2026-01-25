# Standard Library

These functions are registered by `cs_register_stdlib(vm)`.

## IO / Introspection

### `print(...values)`

Prints values separated by spaces and ends with newline.

### `typeof(value) -> string`

Returns one of:
`"nil" "bool" "int" "float" "string" "list" "map" "strbuf" "function" "native"`

### Type Predicates

Boolean type-checking functions:

* `is_nil(value) -> bool`
* `is_bool(value) -> bool`
* `is_int(value) -> bool`
* `is_float(value) -> bool`
* `is_string(value) -> bool`
* `is_list(value) -> bool`
* `is_map(value) -> bool`
* `is_function(value) -> bool`

Example:

```c
if (is_int(x) || is_float(x)) {
  print("x is a number");
}
```

### `getenv(key: string) -> string | nil`

Returns environment variable value or `nil` if missing.

### `assert(cond, message?)`

* If `cond` is falsy → runtime error
* Optional second argument string overrides error message

## Loading / Modules

### `load(path: string) -> bool`

Runs a file. Returns `true` on success, `nil`/error on failure.

### `require(path: string) -> value`

Runs a file once per resolved path.

* Subsequent calls with the same resolved path return the cached exports without re-running.
* Returns the module's exports (the last evaluated expression or explicit exports)

### `require_optional(path: string) -> bool | nil`

Like `require()`, but returns `nil` if the file doesn't exist instead of throwing an error. Returns `true` if loaded successfully.

## Error Handling

### `error(msg: string) -> map`

### `error(msg: string, code: string) -> map`

Creates a standardized error object with:

* `msg` - Error message
* `code` - Optional error code for categorization
* `stack` - List of stack frame strings

### `is_error(value) -> bool`

Returns `true` if value is an error object (has `msg` and `stack` fields).

Example:

```c
fn validate(x) {
  if (x < 0) {
    throw error("Invalid value: must be non-negative", "INVALID_ARG");
  }
}

try {
  validate(-5);
} catch (e) {
  if (is_error(e)) {
    print("Error:", e.msg);
    print("Code:", e.code);
  }
}
```

## Constructors

### `list() -> list`

### `map() -> map`

### `strbuf() -> strbuf`

## Length

### `len(x) -> int`

Works for:

* string: number of bytes
* list: element count
* map: entry count
* strbuf: byte length
  Else returns 0.

## Iteration

### `range(end)` / `range(start, end)` / `range(start, end, step) -> list[int]`

Generates a list of integers:

* `range(5)` → `[0, 1, 2, 3, 4]`
* `range(2, 7)` → `[2, 3, 4, 5, 6]`
* `range(0, 10, 2)` → `[0, 2, 4, 6, 8]`
* `range(10, 0, -2)` → `[10, 8, 6, 4, 2]`

Default step is `1`. Step cannot be `0`.

Example:

```c
for i in range(10) {
  print(i);
}
```

## List Ops

### `push(list, value)`

### `pop(list) -> value | nil`

### `insert(list, index: int, value)`

Inserts `value` at position `index`, shifting existing elements.

### `remove(list, index: int) -> value | nil`

Removes and returns the element at `index`.

### `slice(list, start: int, length: int) -> list`

Returns a new list containing `length` elements starting at `start`.

## Map Ops

### `mget(map, key: string) -> value | nil`

### `mset(map, key: string, value)`

### `mhas(map, key: string) -> bool`

### `mdel(map, key: string)`

Deletes a key from the map.

### `keys(map) -> list[string]`

### `values(map) -> list[value]`

Returns a list of all values in the map.

### `items(map) -> list[list]`

Returns a list of `[key, value]` pairs.

### `map_values(map) -> list[value]`

Alias for `values()`. Returns all values as a list for convenient iteration.

## Data Quality-of-Life

### `copy(x) -> value`

Returns a shallow copy:

* Lists: creates new list with same elements
* Maps: creates new map with same key-value pairs
* Other types: returns the value as-is

### `deepcopy(x) -> value`

Returns a deep copy with cycle detection:

* Lists and maps are recursively copied
* Handles self-referential structures
* Other types: returns the value as-is

### `reverse(list)`

Reverses a list in-place.

### `reversed(list) -> list`

Returns a new reversed list without modifying the original.

### `contains(container, item) -> bool`

Checks if item exists:

* Lists: checks for element
* Maps: checks for key
* Strings: checks for substring

Example:

```c
let original = [1, [2, 3], 4];
let shallow = copy(original);
let deep = deepcopy(original);

shallow[1][0] = 99;  // Modifies original[1][0]
deep[1][1] = 88;     // Does not affect original

reverse([1, 2, 3]);  // Returns [3, 2, 1], in-place
let rev = reversed([1, 2, 3]);  // original unchanged

print(contains([1, 2, 3], 2));        // true
print(contains({"a": 1}, "a"));       // true
print(contains("hello", "ell"));      // true
```

## String Ops

### `str_find(s: string, sub: string) -> int`

Returns first index or `-1`.

### `str_replace(s, old, rep) -> string`

Replaces all occurrences of `old` with `rep`.

* If `old` is empty, returns original string.

### `str_split(s, sep) -> list[string]`

Splits on a string separator.

* If `sep` is empty, returns `[s]`.

### `substr(s: string, start: int, length: int) -> string`

Returns substring starting at `start` with `length` characters.

### `join(list, separator: string) -> string`

Joins list elements into a string with the given separator.

### String Ergonomics

#### `str_trim(s: string) -> string`

Removes leading and trailing whitespace.

#### `str_ltrim(s: string) -> string`

Removes leading whitespace only.

#### `str_rtrim(s: string) -> string`

Removes trailing whitespace only.

#### `str_lower(s: string) -> string`

Converts string to lowercase (ASCII only).

#### `str_upper(s: string) -> string`

Converts string to uppercase (ASCII only).

#### `str_startswith(s: string, prefix: string) -> bool`

Returns `true` if string starts with the given prefix.

#### `str_endswith(s: string, suffix: string) -> bool`

Returns `true` if string ends with the given suffix.

#### `str_repeat(s: string, count: int) -> string`

Returns a new string with `s` repeated `count` times.

Example:

```c
let text = "  Hello World  ";
print(str_trim(text));           // "Hello World"
print(str_lower("HELLO"));       // "hello"
print(str_upper("hello"));       // "HELLO"
print(str_startswith("test", "te"));  // true
print(str_endswith("test", "st"));    // true
print(str_repeat("ab", 3));      // "ababab"
```

## Path Ops

### `path_join(a: string, b: string) -> string`

Joins with `/` when needed.

### `path_dirname(path: string) -> string`

Returns directory portion:

* no slash → `.`
* `/x` → `/`
* `a/b` → `a`

### `path_basename(path: string) -> string`

Returns last path component.

### `path_ext(path: string) -> string`

Returns extension without dot, or `""` if none.

## Formatting

### `fmt(format_string, ...args) -> string`

Supported specifiers:

* `%d` expects int
* `%b` expects bool
* `%s` expects string
* `%v` accepts any value (uses a generic repr)

Literal percent: `%%`

Example:

```c
print(fmt("x=%d ok=%b name=%s", 10, true, "frank"));
```

## Time

### `now_ms() -> int`

Milliseconds timestamp:

* Uses `gettimeofday` on POSIX
* Uses `clock()` fallback on Windows build

### `sleep(ms: int)`

Sleeps that many milliseconds:

* POSIX: `nanosleep`
* Windows: currently errors ("not supported in stdlib")

## Math Functions

### `abs(x) -> int | float`

Returns absolute value. Preserves type (int → int, float → float).

### `min(...values) -> number`

Returns minimum of one or more numbers. Accepts int and/or float.

### `max(...values) -> number`

Returns maximum of one or more numbers. Accepts int and/or float.

### `floor(x) -> int`

Rounds down to nearest integer. Accepts int or float, always returns int.

### `ceil(x) -> int`

Rounds up to nearest integer. Accepts int or float, always returns int.

### `round(x) -> int`

Rounds to nearest integer (0.5 rounds away from zero). Returns int.

### `sqrt(x) -> float`

Returns square root. Accepts int or float, always returns float.

### `pow(base, exp) -> float`

Returns `base` raised to `exp` power. Accepts int or float, returns float.

Example:

```c
let x = sqrt(16);     // 4.0
let y = pow(2, 8);    // 256.0
let z = abs(-42);     // 42
let m = min(3, 7, 2); // 2
```

## Conversion

### `to_int(value) -> int | nil`

Converts a value to integer:

* `int` → returns as-is
* `float` → truncates toward zero (3.9 → 3, -3.9 → -3)
* `bool` → `0` or `1`
* `string` → parses decimal integer, returns `nil` if invalid
* Otherwise → `nil`

### `to_str(value) -> string`

Converts any value to its string representation.

## Memory Management

### `gc() -> int`

Manually triggers garbage collection cycle detection. Returns the number of cycles collected.

### `gc_stats() -> map`

Returns GC statistics as a map:

* `tracked` - Number of tracked objects (lists/maps)
* `collections` - Total collections performed
* `collected` - Total objects collected
* `allocations` - Total allocations since VM start

### `gc_config() -> map`

Returns current GC configuration:

* `threshold` - Collection threshold (0 = disabled)
* `alloc_trigger` - Allocation trigger interval (0 = disabled)

### `gc_config(threshold, alloc_trigger)`

Configures auto-GC policy. See [Implementation Notes](Implementation-Notes#garbage-collection) for details.

## Safety Controls

Scripts can configure their own execution limits:

### `set_timeout(milliseconds)`

Sets maximum execution time in milliseconds. Script aborts if exceeded.

```c
set_timeout(30000);  // Allow 30 seconds for batch processing
```

### `set_instruction_limit(count)`

Sets maximum instruction count. Script aborts if exceeded.

```c
set_instruction_limit(100000000);  // Allow 100M instructions
```

### `get_timeout() -> int`

Returns current timeout in milliseconds (0 = unlimited).

### `get_instruction_limit() -> int`

Returns current instruction limit (0 = unlimited).

### `get_instruction_count() -> int`

Returns number of instructions executed in current script run.

**Example:**

```c
// Check if we need more time for heavy processing
if (get_instruction_count() > get_instruction_limit() / 2) {
    print("Warning: approaching instruction limit");
    set_instruction_limit(get_instruction_limit() * 2);
}
```

### `gc_stats() -> map`

Returns a map with GC statistics:

* `tracked` - Number of currently tracked objects (lists/maps)
* `collections` - Total number of GC collections performed
* `collected` - Total number of objects collected across all collections
* `allocations` - Number of allocations since last GC

Example:

```c
let stats = gc_stats();
print("Tracked objects:", stats.tracked);
print("Total collections:", stats.collections);
print("Objects collected:", stats.collected);
```

### `gc_config() -> map`

### `gc_config(map) -> bool`

### `gc_config(threshold, alloc_trigger) -> bool`

Get or set GC auto-collect configuration.

Parameters:

* `threshold` - Collect when tracked object count reaches this value (0 = disabled)
* `alloc_trigger` - Collect after every N allocations (0 = disabled)

Examples:

```c
// Get current configuration
let config = gc_config();
print("Threshold:", config.threshold);
print("Alloc trigger:", config.alloc_trigger);

// Set configuration with map
gc_config({"threshold": 1000, "alloc_trigger": 500});

// Set configuration with arguments
gc_config(1000, 500);

// Disable auto-GC
gc_config(0, 0);
```

Auto-GC Policy:

* GC automatically runs when tracked objects >= threshold
* GC automatically runs every N allocations (list/map creations)
* GC automatically runs after `cs_vm_run_file()` and `cs_vm_run_string()`
* Both policies can be enabled simultaneously
* Default: both disabled (manual `gc()` only)

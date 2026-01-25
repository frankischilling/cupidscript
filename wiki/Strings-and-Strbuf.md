# Strings & strbuf

## Strings

### Literals

```c
"hello"
"line1\nline2"
```

Escapes supported:

* `\n \t \r \" \\`
* Unknown escapes become literal character (e.g. `\q` → `q`)

### Concatenation with `+`

* `int + int` → arithmetic
* If either side is a string → string concatenation

  * non-string side is rendered as an integer string in the current implementation (best behavior guaranteed for ints; other objects use a generic representation in some contexts)

Example:

```c
print("x=" + 10);
```

### String helpers (stdlib)

See `Standard-Library.md` for:

* `str_find`
* `str_replace`
* `str_split`
* `substr`
* `join`
* `to_str`
* `to_int`

### String Ergonomics

New string manipulation functions for common operations:

#### Trimming

* `str_trim(s)` - Remove whitespace from both ends
* `str_ltrim(s)` - Remove leading whitespace
* `str_rtrim(s)` - Remove trailing whitespace

```c
let s = "  hello world  ";
print(str_trim(s));    // "hello world"
print(str_ltrim(s));   // "hello world  "
print(str_rtrim(s));   // "  hello world"
```

#### Case Conversion

* `str_lower(s)` - Convert to lowercase
* `str_upper(s)` - Convert to uppercase

```c
print(str_lower("Hello World"));  // "hello world"
print(str_upper("Hello World"));  // "HELLO WORLD"
```

#### Substring Checks

* `str_startswith(s, prefix)` - Check if string starts with prefix
* `str_endswith(s, suffix)` - Check if string ends with suffix

```c
print(str_startswith("hello.txt", "hello"));  // true
print(str_endswith("hello.txt", ".txt"));     // true
print(str_startswith("hello.txt", "world"));  // false
```

#### String Utilities

* `str_repeat(s, count)` - Repeat string N times

```c
print(str_repeat("abc", 3));  // "abcabcabc"
print(str_repeat("x", 5));    // "xxxxx"
```

## strbuf (string builder)

Create:

```c
let b = strbuf();
```

Methods are invoked via "field call" syntax:

```c
b.append("hi");
b.append(123);
b.append(true);
b.append(nil);

print(b.str());
print(b.len());
b.clear();
```

### Methods

#### `append(value)`

Appends:

* string contents
* integer as decimal
* bool as `true`/`false`
* nil as `nil`

Anything else may fail (runtime error).

#### `str()`

Returns current contents as an immutable `string`.

#### `clear()`

Resets buffer to empty.

#### `len()`

Returns current length as `int`.

# CupidScript

A lightweight C99 VM for a small scripting language. This repository contains a compact interpreter/VM written in portable C that exposes a clean C API for embedding and for extending with native functions.

---

## What’s Included

- **Core runtime:** lexer, parser, AST, VM, and a small standard library.
- **Sample CLI (`src/main.c`):** demonstrates embedding and registering `fm.*` natives.
- **Public headers:** `src/cupidscript.h` is the embedder-facing API.
- **Examples/tests:** small scripts that exercise language features and stdlib.

---

## Quick Start

```sh
make
bin/cupidscript examples/features.cs
```

Other useful scripts:
- `bin/cupidscript examples/test.cs`
- `bin/cupidscript examples/stress.cs`
- `bin/cupidscript examples/stacktrace.cs`
- `bin/cupidscript examples/closures.cs`
- `bin/cupidscript examples/time.cs`
- `bin/cupidscript examples/benchmark.cs`

---

## Language Overview

CupidScript is intentionally small: a tree-walk interpreter with dynamic values and a C embedding API.

### Syntax (core)

```cs
let name = expr;     // declaration (optional initializer)
name = expr;         // assignment

fn add(a, b) {       // function definition
  return a + b;
}

if (cond) { ... } else { ... }
while (cond) { ... }
return expr;
```

### Expressions

- Operators with precedence: `||`, `&&`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `+`, `-`, `*`, `/`, `%`, unary `!`, unary `-`
- Strings: `"..."` with escapes `\n`, `\t`, `\r`, `\"`, `\\`
- Dynamic values: `nil`, `true/false`, integers, strings, functions, native functions, lists, maps

Notes:
- `+` supports `int + int` and string concatenation (if either operand is a string).
- `&&` / `||` short-circuit.

### Lists and Maps

Lists and maps are first-class dynamic values.

```cs
let xs = list();
push(xs, 10);
push(xs, 20);
xs[1] = 99;
print(xs[0], xs[1], len(xs)); // 10 99 2

let m = map();
m["name"] = "cupid";
print(m["name"]);             // cupid
print(keys(m));               // list of string keys
```

Indexing rules:
- `list[int]` -> element or `nil` if out of range
- `map[string]` -> value or `nil` if missing
- Assignment supports `xs[i] = v` and `m["k"] = v`

Practical plugin pattern (structured state/config):

```cs
let state = map();
state["enabled"] = true;
state["bindings"] = list();
push(state["bindings"], "F5");
```

Current limitations (by design, for v0 simplicity):
- Map keys are strings only (use `m["key"]` / `mset(m, "key", v)`).
- No built-in `for` loop; iterate with `while` + `len()` + indexing, and use `keys(map)` for map keys.

### Multi-file scripts

Stdlib provides:
- `load("file.cs")`: executes another script file every time it’s called
- `require("file.cs")`: executes a file once per VM (subsequent calls are no-ops)

Paths are resolved relative to the currently-running script file’s directory.

### Field access and method calls

`obj.field` is supported as field access:
- If `obj` is a map, `obj.field` returns the same value as `obj["field"]`.

`obj.method(a, b)` is supported as a method call (currently used by `strbuf`).

Compatibility note: CupidFM-style dotted globals still work (e.g. `fm.status("hi")`) even if `fm` is not a script value; the VM falls back to looking up a global named `"fm.status"`.

### Function references and closures

Functions are values, so you can pass them to native APIs as callbacks:

```cs
fn on_key(key) {
  print("key:", key);
}

// Example: fm.on("key", on_key)
```

CupidScript also supports closures (functions capturing variables) and anonymous function literals:

```cs
fn make_counter() {
  let n = 0;
  return fn() { n = n + 1; return n; };
}

let c = make_counter();
print(c(), c(), c()); // 1 2 3
```

---

## Directory Overview

- `src/cs_value.c`, `src/cs_lexer.c`, `src/cs_parser.c`, `src/cs_vm.c`, `src/cs_stdlib.c` – core runtime and standard library implementations.
- `src/main.c` – sample program showing how to bootstrap the VM and expose native functions (the `fm.*` API in this project).
- **Headers:** `src/cupidscript.h`, `src/cs_vm.h`, `src/cs_value.h` – public API and value types.
- `Makefile` – simple build system producing a library and a small executable.

---

## Build

### Prerequisites

- A C99-compliant compiler (gcc/clang) and a POSIX-like toolchain.
- `make` (as specified by the provided Makefile).

### Build with Make (recommended)

```sh
make all
```

This should produce:
- `libcupidscript.a` in `bin/`
- `cupidscript` (executable) in `bin/`

### Manual Build (if you don’t have make)

```sh
CC=gcc
CFLAGS="-std=c99 -Wall -Wextra -O2 -g -D_POSIX_C_SOURCE=200809L"
SRCDIR=src
OBJDIR=obj
BINDIR=bin
AR=ar
ARFLAGS=rcs

mkdir -p $OBJDIR $BINDIR

# Compile sources
for f in cs_value.c cs_lexer.c cs_parser.c cs_vm.c cs_stdlib.c; do
  $CC $CFLAGS -Isrc -c "$SRCDIR/$f" -o "$OBJDIR/${f%.*}.o"
done

# Create library
$AR $ARFLAGS "$BINDIR/libcupidscript.a" "$OBJDIR"/*.o

# Compile CLI (example main)
$CC $CFLAGS -Isrc -c "$SRCDIR/main.c" -o "$OBJDIR/main.o"
# Link executable
$CC $CFLAGS -Isrc "$OBJDIR/main.o" "$BINDIR/libcupidscript.a" -o "$BINDIR/cupidscript"
```

---

## Usage

#### Embedder usage (typical flow):

1. Create a VM:  
   ```c
   cs_vm* vm = cs_vm_new();
   ```
2. Register stdlib:  
   ```c
   cs_register_stdlib(vm);
   ```
3. Expose natives:  
   ```c
   cs_register_native(vm, "my.native", my_fn, NULL);
   ```
4. Run code from file:  
   ```c
   cs_vm_run_file(vm, "script.cs");
   ```
   Or run from a string:  
   ```c
   cs_vm_run_string(vm, code, "<string>");
   ```
5. Call a named script function:  
   ```c
   cs_call(vm, "function_name", argc, argv, &out);
   ```
   Or call a function value (callback) you previously stored:  
   ```c
   cs_call_value(vm, fn_value, argc, argv, &out);
   ```
6. Retrieve errors:  
   ```c
   const char* err = cs_vm_last_error(vm);
   ```
7. Strings:  
   ```c
   cs_str(vm, "hello"); /* and convert to C with cs_to_cstr(out_val); */
   ```

---

## Key Types and API

- **Value types** (`cs_type`, exposed via `cupidscript.h`):
  - `CS_T_NIL`, `CS_T_BOOL`, `CS_T_INT`, `CS_T_STR`, `CS_T_LIST`, `CS_T_MAP`, `CS_T_FUNC`, `CS_T_NATIVE`
- **Core value wrapper:**  
  ```c
  cs_value { type; union { int b; int64_t i; void* p; } as; }
  ```
- **Public helpers:**
  - `cs_vm_new`, `cs_vm_free`
  - `cs_vm_run_file`, `cs_vm_run_string`
  - `cs_register_native`, `cs_call`
  - `cs_call_value` (call a function value from C)
  - `cs_vm_last_error` / `cs_error` (get/set VM error from native code)
  - `cs_last_error` (compat getter; returns `NULL` if no error)
  - `cs_to_cstr`, `cs_nil`, `cs_bool`, `cs_int`, `cs_str`, `cs_str_take`
  - `cs_list`, `cs_map`
  - `cs_value_copy`, `cs_value_release` (retain/release values for host storage)

---

## Standard Library (Current)

CupidScript’s stdlib is implemented in `src/cs_stdlib.c` and registered via `cs_register_stdlib(vm)`.

### Core helpers

- `print(...)` → prints values to stdout
- `assert(cond, "message")` → sets a VM error and aborts execution if `cond` is falsy
- `typeof(x)` → returns `"nil" | "bool" | "int" | "string" | "list" | "map" | ...`
- `getenv("NAME")` → returns a string or `nil`

### Multi-file loading

- `load(path)` → executes another script file (every call)
- `require(path)` → executes another script once per VM

### List helpers

- `list()` → new list
- `len(list|string|map)` → length
- `push(list, value)` → append
- `pop(list)` → pop last element (or `nil`)

### Map helpers

- `map()` → new map
- `mget(map, key)` → get value (or `nil`)
- `mset(map, key, value)` → set value
- `mhas(map, key)` → bool
- `keys(map)` → list of keys (strings)

Note: You can usually use indexing instead of `mget/mset`:
`m["k"]`, `m["k"] = v`.

### String helpers

- `str_find(s, sub)` → index (or `-1`)
- `str_replace(s, old, repl)` → new string
- `str_split(s, sep)` → list of strings

### Path helpers

- `path_join(a, b)` → joined path (simple join)
- `path_dirname(path)` → directory portion
- `path_basename(path)` → basename
- `path_ext(path)` → extension without dot (or `""`)

### Formatting

- `fmt("x=%d s=%s b=%b v=%v", ...)` → formatted string
  - `%d` int, `%s` string, `%b` bool, `%v` any value (best-effort), `%%` literal percent

### Time helpers

- `now_ms()` → current wall-clock time in milliseconds (int)
- `sleep(ms)` → sleep for `ms` milliseconds (blocking; implemented via POSIX `nanosleep`)

### `strbuf` (string builder)

For high-performance string construction, use the native string builder:

```cs
let b = strbuf();
b.append("x");
b.append(123);
let s = b.str();
```

Methods:
- `b.append(x)` → appends a value (`string`, `int`, `bool`, `nil`)
- `b.str()` → returns a string snapshot
- `b.len()` → current length (bytes)
- `b.clear()` → clears the buffer

---

## Design Notes

- The runtime is split into:
  - **cs_value.c/h**: value representation and helpers for scalar and heap objects.
  - **cs_lexer.c/h** and **cs_parser.c/h**: tokenization and parsing into an AST.
  - **cs_vm.c/h**: execution engine with a small call/stack model and a global environment (`cs_env`) with lexical scoping via closures.
- **cs_stdlib.c**: small host-facing API exposed to CupidScript via `cs_register_native` (includes `print`, `assert`, `load`/`require`, list/map helpers, string/path helpers, and `fmt`).
- **Strings are ref-counted:** The code uses a dedicated `cs_string` type (see `cs_value.h`).  
  *Note:* Ensure you consistently use the public API for string lifetimes.

---

## Error Reporting

Parse errors and runtime errors include `file:line:column` when available.

Runtime errors also include a stack trace, for example:

```text
Runtime error at examples/stacktrace.cs:3:15: division by zero
Stack trace:
  at inner (examples/stacktrace.cs:8:16)
  at outer (examples/stacktrace.cs:11:7)
```

Use `cs_vm_last_error(vm)` to retrieve the current VM error string (returns `""` if there is no error). `cs_last_error(vm)` is a compatibility getter that may return `NULL`.

---

## Examples

- `examples/test.cs` and `examples/stress.cs`: basic language coverage
- `tests/math.cs`: operator precedence checks (uses `assert`)
- `examples/features.cs`: lists/maps, indexing, `fmt`, string/path helpers, and `require`
- `examples/stacktrace.cs`: demonstrates runtime stack traces
- `examples/closures.cs`: demonstrates closures + anonymous `fn() { }`
- `examples/time.cs`: demonstrates `now_ms()` and `sleep(ms)`
- `examples/benchmark.cs`: quick-and-dirty performance benchmark (arith/calls/list/map/string)

---

## Extending with Native Functions

- Implement a function matching `cs_native_fn` and register it:

  ```c
  static int my_native(cs_vm* vm, void* ud, int argc, const cs_value* argv, cs_value* out) {
      // Validate args, operate, and set *out as needed
      if (argc > 0) {
          // example: print first arg if string
      }
      if (out) *out = cs_nil();
      return 0;
  }
  ```

- Register from your bootstrap code:
  ```c
  cs_register_native(vm, "my.native", my_native, NULL);
  ```

- The layout for values and strings is defined in the headers.

### Native error handling pattern

Native functions return `0` on success. To raise an error:
- call `cs_error(vm, "message")`
- return non-zero from the native function

This produces a runtime error and unwinds execution (with a stack trace if the call happened from script).

### Callback pattern (CupidFM-style)

Because functions are values, a host can accept callbacks like:

```cs
fm.on("event", fn(payload) {
  print("event payload:", payload);
});
```

On the C side you typically:
- store the callback `cs_value` using `cs_value_copy`
- later invoke it with `cs_call_value`
- release it with `cs_value_release` when unregistering/unloading

---

## Notes

- This project is a small, embeddable scripting VM with a tiny standard library and some example natives.
- **License:** This project is licensed under the GNU General Public License version 3 (GPLv3).  
  See the `COPYING` file or https://www.gnu.org/licenses/gpl-3.0.html for details.
- For a quick reference or usage guidance, check the header API in `src/cupidscript.h` and the implementation in the `src` directory.

---

## License

- **GPLv3.** This repository is licensed under the GNU General Public License version 3.  
  See [`LICENSE`](LICENSE) or visit [https://www.gnu.org/licenses/gpl-3.0.html](https://www.gnu.org/licenses/gpl-3.0.html) for details.

---

## Known Issues
None currently tracked.

---

## Contributing

- If you’d like to contribute:
  - Start by adding a README-style doc
  - Improve the build/tests
  - Provide small example scripts in the `examples` directory

---

## TODO

- [ ] Add cupidfm lib support

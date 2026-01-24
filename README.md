# CupidScript

A lightweight C99 VM for a small scripting language. This repository contains a compact interpreter/VM written in portable C that exposes a clean C API for embedding and for extending with native functions.

---

## What’s Included

- **Core runtime:** lexer, parser, VM, and a tiny standard library for the host application to expose helpers.
- **Sample main:** for embedding CupidScript from C and a tiny example native API.
- **Public headers:** to integrate CupidScript into your own project.

---

## Directory Overview

- `src/cs_value.c`, `src/cs_lexer.c`, `src/cs_parser.c`, `src/cs_vm.c`, `src/cs_stdlib.c` – core runtime and standard library implementations.
- `src/main.c` – sample program showing how to bootstrap the VM and expose native functions (the `fm.*` API in this project).
- **Headers:** `cupidscript.h`, `cs_vm.h`, `cs_value.h` – public API and value types.
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
CFLAGS="-std=c99 -Wall -Wextra -O2 -g"
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
5. Call script functions:  
   ```c
   cs_call(vm, "function_name", argc, argv, &out);
   ```
6. Retrieve errors:  
   ```c
   const char* err = cs_last_error(vm);
   ```
7. Strings:  
   ```c
   cs_str(vm, "hello"); /* and convert to C with cs_to_cstr(out_val); */
   ```

---

## Key Types and API

- **Value types** (`cs_type`, exposed via `cupidscript.h`):
  - `CS_T_NIL`, `CS_T_BOOL`, `CS_T_INT`, `CS_T_STR`, `CS_T_FUNC`, `CS_T_NATIVE`
- **Core value wrapper:**  
  ```c
  cs_value { type; union { int b; int64_t i; void* p; } as; }
  ```
- **Public helpers:**
  - `cs_vm_new`, `cs_vm_free`
  - `cs_vm_run_file`, `cs_vm_run_string`
  - `cs_register_native`, `cs_call`
  - `cs_last_error`, `cs_to_cstr`, `cs_nil`, `cs_bool`, `cs_int`, `cs_str`

---

## Design Notes

- The runtime is split into:
  - **cs_value.c/h**: value representation and helpers for scalar and heap objects.
  - **cs_lexer.c/h** and **cs_parser.c/h**: tokenization and parsing into an AST.
  - **cs_vm.c/h**: execution engine with a small call/stack model and a global environment (`cs_env`) with lexical scoping via closures.
  - **cs_stdlib.c**: small host-facing API (print, typeof, getenv) exposed to CupidScript via `cs_register_native`.
- **Strings are ref-counted:** The code uses a dedicated `cs_string` type (see `cs_value.h`).  
  *Note:* Ensure you consistently use the public API for string lifetimes.

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

---

## Notes

- This project is a small, embeddable scripting VM with a tiny standard library and some example natives.
- **License:** This project is licensed under the GNU General Public License version 3 (GPLv3).  
  See the `COPYING` file or https://www.gnu.org/licenses/gpl-3.0.html for details.
- For a quick reference or usage guidance, check the header API in `cupidscript.h` and the implementation in the `src` directory.

---

## License

- **GPLv3.** This repository is licensed under the GNU General Public License version 3.  
  See [`LICENSE`](LICENSE) or visit [https://www.gnu.org/licenses/gpl-3.0.html](https://www.gnu.org/licenses/gpl-3.0.html) for details.

---

## Known Issues

- A type mismatch was found between `cs_string` and `cs_str` usage in a stdlib implementation. The code defines `cs_string` in `cs_value.h` and uses its `data` member; adjust code accordingly when wiring new natives.
- Building in this environment requires make and a C compiler. If both are not present, a local build cannot be executed.

---

## Contributing

- If you’d like to contribute:
  - Start by adding a README-style doc
  - Improve the build/tests
  - Provide small example scripts in the `examples` directory

---

## TODO

- [ ] Add cupidfm lib support


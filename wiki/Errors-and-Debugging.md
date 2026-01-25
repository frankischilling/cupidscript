# Errors & Debugging

CupidScript has two major error classes:

* Parse errors (lexer/parser)
* Runtime errors (VM)

## Parse Errors

When parsing fails, the parser produces an allocated error string like:

```
Parse error at <input>:line:col: message
```

Examples:

* "expected expression"
* "expected ')'"
* "expected '{'"
* "expected function name"
* "invalid assignment target"

## Runtime Errors

Runtime errors include source location:

```
Runtime error at source:line:col: message
```

Common runtime errors:

* undefined variable
* type error: arithmetic expects int
* division by zero
* mod by zero
* comparisons require both ints or both strings
* attempted to call non-function
* wrong argument count
* indexing expects list[int] or map[string]
* field access expects map
* uncaught exception (from `throw`)

## Exception Handling

CupidScript supports exception handling via `throw` and `try/catch`:

```c
try {
  if (x < 0) {
    throw "Invalid input: negative number";
  }
  process(x);
} catch (e) {
  print("Error caught:", e);
}
```

Thrown values can be any type - commonly strings or maps with error details:

```c
throw {"msg": "file not found", "code": 404, "path": p};
```

## Error Objects

For standardized error handling, use the `error()` helper to create error objects:

* `error(msg)` - Creates a map with `{msg, stack}` fields
* `error(msg, code)` - Creates a map with `{msg, code, stack}` fields
* `is_error(value)` - Checks if value is an error object (has `msg` and `stack` fields)

The `stack` field contains a list of stack frame strings captured at the time of error creation.

Example:

```c
fn divide(a, b) {
  if (b == 0) {
    throw error("Division by zero", "DIV_ZERO");
  }
  return a / b;
}

try {
  let result = divide(10, 0);
} catch (e) {
  if (is_error(e)) {
    print("Error:", e.msg);
    print("Code:", e.code);
    print("Stack trace:");
    for frame in e.stack {
      print("  ", frame);
    }
  } else {
    print("Caught:", e);
  }
}
```

Benefits of error objects:

* Consistent structure across all errors
* Automatic stack trace capture
* Optional error codes for categorization
* Easy to check with `is_error()`

## Stack Traces

Your VM maintains a call frame stack for:

* native function calls
* user-defined function calls
* dotted "method/global fallback" calls

When an error occurs, the VM appends:

```
Stack trace:
  at func (file:line:col)
  at ...
```

This makes it practical to debug scripts without a debugger attached.

## Tips

* When building larger scripts, wrap assumptions with `assert(...)`.
* Use `typeof(x)` when debugging dynamic type issues.
* Use `fmt("%v", x)` or `print(x)` to inspect values.

# Modules (load/require) & Paths

CupidScript implements basic file execution helpers in the stdlib.

## Current Directory Stack

The VM tracks a directory stack. When you call `load("rel.cs")` or `require("rel.cs")`:

* If the path is absolute, it's used directly.
* If the path is relative, it's joined with the VM's current directory.

This is used so scripts can load other scripts relative to themselves.

## `load(path)`

Runs a file immediately (every time you call it).

```c
load("util.cs");
load("util.cs"); // runs again
```

## `require(path)`

Runs a file only once per resolved path.

```c
let util = require("util.cs");
let util2 = require("util.cs"); // cached, returns same exports
```

`require()` returns the module's `exports` map.

Inside a required file, the VM provides:

* `exports` (map) - values to export
* `__file__` (string) - resolved module path
* `__dir__` (string) - directory containing the module

## `require_optional(path)`

Like `require()`, but returns `nil` if the file doesn't exist instead of throwing an error.

```c
// Try to load optional configuration
let config = require_optional("config.cs");
if (config == nil) {
  print("No config file found, using defaults");
  config = default_config();
}
```

Useful for:

* Optional plugins or extensions
* Configuration files that may not exist
* Conditional feature loading based on file presence

Returns:

* module exports if file exists
* `nil` if file doesn't exist

## Path Helpers

Use:

* `path_join(a,b)`
* `path_dirname(p)`
* `path_basename(p)`
* `path_ext(p)`

Example:

```c
let p = path_join("src", "main.cs");
print(path_dirname(p));
print(path_basename(p));
print(path_ext(p));
```

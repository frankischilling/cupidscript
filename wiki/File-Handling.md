# File Handling

CupidScript provides comprehensive file handling capabilities including pattern matching, file watching, temporary files, and archive operations.

> **Note:** Advanced file handling features (glob, file watching, archives) are currently available on Linux only.

## Table of Contents

- [Glob Patterns](#glob-patterns)
- [File Watching](#file-watching)
- [Temporary Files](#temporary-files)
- [Archive Operations](#archive-operations)
- [Common Patterns](#common-patterns)
- [Performance Considerations](#performance-considerations)

---

## Glob Patterns

Glob patterns allow you to match multiple files using wildcards and patterns.

### Basic Usage

```cs
glob(pattern: string, path?: string) → list
```

**Parameters:**
- `pattern` - The glob pattern to match
- `path` - Optional base directory (defaults to current directory)

**Returns:** List of matching file paths

### Pattern Syntax

| Pattern | Description | Example |
|---------|-------------|---------|
| `*` | Match any characters except `/` | `*.txt` matches `file.txt` |
| `?` | Match single character | `file?.txt` matches `file1.txt` |
| `**` | Match recursively | `**/*.cs` matches all `.cs` files in subdirectories |
| `[abc]` | Match any char in set | `file[123].txt` matches `file1.txt`, `file2.txt` |
| `{a,b,c}` | Match any alternative | `*.{cs,txt}` matches `.cs` or `.txt` files |

### Examples

```cs
// Find all text files in current directory
let txt_files = glob("*.txt");

// Find all CupidScript files recursively
let cs_files = glob("**/*.cs", "src");

// Find test files
let tests = glob("test_*.cs", "tests");

// Find all source files (C and headers)
let c_files = glob("**/*.{c,h}", "src");

// Process matched files
for file in glob("**/*.json", "data") {
    let content = read_file(file);
    print("Processing:", file);
}
```

### Use Cases

- **Code analysis:** Find all source files for processing
- **Batch operations:** Apply operations to matching files
- **File discovery:** Locate files matching patterns
- **Cleanup:** Find temporary or old files for removal

---

## File Watching

Monitor files and directories for changes using Linux inotify.

### Functions

#### watch_file

```cs
watch_file(path: string, callback: function) → int
```

Watch a single file for changes.

**Parameters:**
- `path` - Path to file to watch
- `callback` - Function called on changes: `fn(event_type: string, path: string)`

**Returns:** Watch handle (integer) for use with `unwatch()`

**Event Types:**
- `"created"` - File was created
- `"modified"` - File was modified
- `"deleted"` - File was deleted
- `"renamed"` - File was renamed

#### watch_dir

```cs
watch_dir(path: string, callback: function, recursive?: bool) → int
```

Watch a directory for changes.

**Parameters:**
- `path` - Path to directory to watch
- `callback` - Function called on changes
- `recursive` - Optional: watch subdirectories (default: false)

**Returns:** Watch handle

#### unwatch

```cs
unwatch(handle: int) → bool
```

Stop watching a file or directory.

**Returns:** `true` if successfully unwatched, `false` otherwise

#### process_file_watches

```cs
process_file_watches() → nil
```

Process pending file system events and trigger callbacks. Call this periodically to receive events.

### Examples

```cs
// Watch a configuration file
let config_handle = watch_file("config.yaml", fn(event, path) {
    print("Config changed:", event);
    // Reload configuration
    let new_config = read_file(path);
    // ... apply new config
});

// Watch a directory for new files
let dir_handle = watch_dir("uploads", fn(event, path) {
    if event == "created" {
        print("New upload:", path);
        // Process uploaded file
    }
});

// Main loop with file watching
while running {
    // Do work...
    
    // Process file system events
    process_file_watches();
    
    sleep(100);
}

// Cleanup
unwatch(config_handle);
unwatch(dir_handle);
```

### Recursive Directory Watching

```cs
// Watch directory tree for changes
let handle = watch_dir("src", fn(event, path) {
    print("Source changed:", event, path);
    // Trigger rebuild, tests, etc.
}, true);

// Process events in your main loop
process_file_watches();
```

### Use Cases

- **Auto-reload:** Reload configuration when files change
- **Build systems:** Trigger builds when source files change
- **File processing:** Process new files as they arrive
- **Synchronization:** Sync changes to remote systems

---

## Temporary Files

Create temporary files and directories with automatic cleanup.

### Functions

#### temp_file

```cs
temp_file(prefix?: string, suffix?: string) → string
```

Create a temporary file.

**Parameters:**
- `prefix` - Optional prefix for filename (default: `"cs_"`)
- `suffix` - Optional suffix/extension (default: none)

**Returns:** Path to temporary file

#### temp_dir

```cs
temp_dir(prefix?: string) → string
```

Create a temporary directory.

**Parameters:**
- `prefix` - Optional prefix for directory name (default: `"cs_"`)

**Returns:** Path to temporary directory

### Automatic Cleanup

Temporary files and directories are automatically cleaned up when the process exits. The cleanup handles:
- Removing files created with `temp_file()`
- Removing directories created with `temp_dir()`
- Recursive cleanup of directory contents

### Examples

```cs
// Create simple temp file
let temp = temp_file();
write_file(temp, "temporary data");
let data = read_file(temp);
// Automatically cleaned up on exit

// Create temp file with custom naming
let log_file = temp_file("myapp_", ".log");
write_file(log_file, "Log entry 1\nLog entry 2");

// Create temp directory for workspace
let workspace = temp_dir("build_");
mkdir(workspace + "/obj");
mkdir(workspace + "/bin");
write_file(workspace + "/obj/main.o", "...");
write_file(workspace + "/bin/program", "...");
// Entire tree cleaned up on exit

// Atomic file write pattern
let target = "important_config.json";
let temp_write = temp_file("config_", ".tmp");
write_file(temp_write, '{"version": "2.0"}');
// Validate...
rename(temp_write, target); // Atomic rename
```

### Use Cases

- **Temporary storage:** Cache data during processing
- **Atomic writes:** Write to temp, then rename
- **Build artifacts:** Store intermediate build files
- **Download cache:** Cache downloaded files
- **Testing:** Create isolated test environments

---

## Archive Operations

Create and extract tar and gzip archives.

### GZIP Functions

#### gzip_compress

```cs
gzip_compress(input_path: string, output_path: string) → bool
```

Compress a file using gzip.

**Returns:** `true` on success, `false` on failure

#### gzip_decompress

```cs
gzip_decompress(input_path: string, output_path: string) → bool
```

Decompress a gzip file.

**Returns:** `true` on success, `false` on failure

### TAR Functions

#### tar_create

```cs
tar_create(archive_path: string, files: list, compress?: string) → bool
```

Create a tar archive from a list of files.

**Parameters:**
- `archive_path` - Output archive path
- `files` - List of file paths to include
- `compress` - Optional: `"gzip"` to create `.tar.gz`

**Returns:** `true` on success

#### tar_list

```cs
tar_list(archive_path: string) → list
```

List contents of a tar archive.

**Returns:** List of filenames in archive, or `nil` on error

#### tar_extract

```cs
tar_extract(archive_path: string, dest_path: string) → bool
```

Extract a tar archive to a directory.

**Returns:** `true` on success

### Examples

```cs
// GZIP compression
gzip_compress("large_file.txt", "large_file.txt.gz");
gzip_decompress("large_file.txt.gz", "restored.txt");

// Create tar archive
let files = [
    "docs/readme.md",
    "docs/guide.md",
    "docs/api.md"
];
tar_create("docs.tar", files);

// List archive contents
let contents = tar_list("docs.tar");
for file in contents {
    print("Archive contains:", file);
}

// Extract archive
tar_extract("docs.tar", "extracted_docs");

// Create compressed archive (tar.gz)
tar_create("backup.tar.gz", files, "gzip");

// Extract compressed archive
tar_extract("backup.tar.gz", "restored");
```

### Backup Example

```cs
// Create timestamped backup
let timestamp = to_str(unix_s());
let backup_name = "backup_" + timestamp + ".tar.gz";

// Find files to backup
let source_files = glob("**/*.cs", "src");
let config_files = glob("*.yaml", "config");
let all_files = extend(source_files, config_files);

// Create compressed backup
tar_create(backup_name, all_files, "gzip");
print("Backup created:", backup_name);

// Verify backup
let backup_contents = tar_list(backup_name);
print("Backed up", len(backup_contents), "files");
```

---

## Common Patterns

### Auto-Reload Configuration

```cs
let config = read_file("config.yaml");

watch_file("config.yaml", fn(event, path) {
    if event == "modified" {
        config = read_file(path);
        print("Configuration reloaded");
    }
});

while running {
    // Use config...
    process_file_watches();
    sleep(100);
}
```

### Build System with File Watching

```cs
// Watch source directory for changes
watch_dir("src", fn(event, path) {
    if str_endswith(path, ".cs") {
        print("Source changed:", path);
        // Trigger rebuild
        let result = subprocess("make", []);
        print("Build:", result.code == 0 ? "success" : "failed");
    }
}, true);
```

### Batch File Processing

```cs
// Process all JSON files in directory
let json_files = glob("**/*.json", "data");
let results = temp_dir("processing_");

for file in json_files {
    let data = json_parse(read_file(file));
    // Process data...
    let output = results + "/" + path_basename(file);
    write_file(output, json_stringify(processed));
}

// Create archive of results
tar_create("results.tar.gz", glob("*", results), "gzip");
```

### Atomic Configuration Update

```cs
fn update_config(new_config) {
    let temp = temp_file("config_", ".yaml");
    
    // Write to temp file
    write_file(temp, new_config);
    
    // Validate
    let parsed = yaml_parse(read_file(temp));
    if parsed == nil {
        print("Invalid config");
        return false;
    }
    
    // Atomic rename
    rename(temp, "config.yaml");
    return true;
}
```

---

## Performance Considerations

### Glob Patterns

- **Recursive patterns (`**`):** Can be slow on large directory trees
- **Limit depth:** Use specific patterns instead of `**` when possible
- **Cache results:** Store glob results if pattern doesn't change
- **Filter early:** Use specific patterns to reduce matching

```cs
// Slow: searches entire tree
let all = glob("**/*");

// Faster: specific pattern
let cs_files = glob("**/*.cs", "src");

// Fastest: non-recursive when possible
let tests = glob("*.cs", "tests");
```

### File Watching

- **Event processing:** Call `process_file_watches()` regularly
- **Watch granularity:** Watch specific files/directories, not root
- **Cleanup watches:** Always `unwatch()` when done
- **Batch processing:** Process multiple events together

```cs
// Good: specific watches
watch_file("config.yaml", handler);
watch_dir("uploads", handler);

// Bad: watching too much
watch_dir("/", handler, true); // Don't do this!
```

### Temporary Files

- **Cleanup:** Automatic cleanup happens on exit
- **Disk space:** Monitor temp space usage for long-running processes
- **Manual cleanup:** Clean up manually if creating many temp files
- **Temp directory:** All temp files go to system temp (`/tmp`)

```cs
// Long-running process: manual cleanup
for i in range(10000) {
    let temp = temp_file();
    // Use temp...
    rm(temp); // Clean up manually
}
```

### Archives

- **Compression overhead:** tar.gz is slower but smaller
- **File size:** Large files take more time to compress
- **Memory usage:** Archives processed in memory-efficient chunks
- **Validation:** tar_list reads entire archive

```cs
// Fast: uncompressed tar
tar_create("backup.tar", files);

// Slower but smaller: compressed
tar_create("backup.tar.gz", files, "gzip");

// Separate compression for control
tar_create("temp.tar", files);
gzip_compress("temp.tar", "backup.tar.gz");
```

---

## Platform Support

| Feature | Linux | macOS | Windows |
|---------|-------|-------|---------|
| glob | ✓ | ✗ | ✗ |
| watch_file | ✓ (inotify) | ✗ | ✗ |
| watch_dir | ✓ (inotify) | ✗ | ✗ |
| temp_file | ✓ | ✗ | ✗ |
| temp_dir | ✓ | ✗ | ✗ |
| gzip | ✓ | ✗ | ✗ |
| tar | ✓ | ✗ | ✗ |

> **Note:** These advanced features are currently Linux-only. Basic file operations (`read_file`, `write_file`, `exists`, etc.) are cross-platform.

---

## See Also

- [Standard Library](Standard-Library.md) - Complete function reference
- [Data Formats](Data-Formats.md) - JSON, YAML, CSV, XML handling
- [Strings and Strbuf](Strings-and-Strbuf.md) - String processing
- [Collections](Collections.md) - Lists, maps, sets

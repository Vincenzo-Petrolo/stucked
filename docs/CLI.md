# Command-Line Interface

This document details the CLI introduced in this version.

## Synopsis

```
stucked [OPTIONS] <callgraph.txt> [stackmap1.txt ...]
```

- The first positional argument is the call graph file.
- Subsequent positional arguments (zero or more) are stack map files.

## Output Modes

- **Default**: Prints a sentence with the maximum stack usage, then the path
  (function names) that realize that maximum.
- **Quiet (`-q`)**: Prints only the numeric maximum, suitable for scripting.
- **JSON (`-j`)**: Prints a JSON document with `max_stack` and the `path` array.

## Verbose Mode

`-v / --verbose` writes progress messages to `stderr`, including which files
are being parsed and when the graph is being read.


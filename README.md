# stucked — Worst-Case Stack Analyzer

`stucked` reads a call graph plus one or more *stack map* files (mapping
function names to stack frame sizes) and reports the **maximum stack usage**
along a path in the graph. This version adds a friendly command-line
interface and a portable `./configure` + `make install` flow — without
modifying the existing graph and dictionary data structures.

## Build & Install

```sh
./configure            # accepts --prefix=/usr/local, CC=..., CFLAGS=...
make
sudo make install      # installs to $prefix/bin and man page to $prefix/share/man/man1
```

To uninstall:

```sh
sudo make uninstall
```

## Usage

```text
stucked [OPTIONS] <callgraph.txt> [stackmap1.txt ...]
```

Options:
- `-h, --help`      Show help and exit
- `-V, --version`   Show version and exit
- `-v, --verbose`   Verbose logging to stderr
- `-q, --quiet`     Only print the maximum stack usage (number only)
- `-j, --json`      Output JSON (`{"max_stack": N, "path": ["f1", ...]}`)
- `-p, --print-path`  Print the function chain (default)

### Examples

Compute from a call graph and two stack maps:

```sh
stucked callgraph.txt stacks_1.txt stacks_2.txt
```

JSON output suitable for tooling:

```sh
stucked -j callgraph.txt frames.txt
```

Quiet numeric output (handy in scripts):

```sh
stucked -q callgraph.txt frames.txt
```

## Input Formats

- **Call graph**: must be readable by the existing `read_file()` function in `graph.c`.
- **Stack maps**: Each line should contain a function name and a stack size value;
  `parse_line()` in `stucked.c` expects a colon-delimited format like:
  
  ```
  <func>:...:...:<stack_size>
  ```

## Project Layout

```
.
├── configure
├── Makefile.in
├── man/
│   └── stucked.1
├── docs/
│   └── CLI.md
└── src/
    ├── stucked.c       # CLI-polished main (includes graph.c)
    ├── graph.c         # (unchanged)
    └── dictionary.c    # (unchanged)
```
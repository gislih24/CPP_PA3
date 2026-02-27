# CPP_PA3

Expression parser/evaluator using an Abstract Syntax Tree (AST).

## Build

Run in `CPP_PA3/`:

```bash
make
```

This builds `bin/ast_program` with `g++ -std=c++20` and without using any
external libraries.

## Base Version

### Part 1: Build an AST file from an expression

```bash
./bin/ast_program build <ast_output_file> [expression_input_file]
```

- If `expression_input_file` is not included, the expression is read from
  `stdin`.
- Input expressions support integers, variables (`[a-z]+`), parentheses, and
  operators `+ - * /` (including unary minus).
- Whitespace is ignored.

Example:

```bash
./bin/ast_program build tree.txt input.txt
```

### Part 2: Evaluate an AST file

```bash
./bin/ast_program eval <ast_input_file> [variable_values_file]
```

- If the AST contains any variables, please pass a variable file with one
  assignment per line, like: `name=value` (e.g. `x=7`, or `currentyear = 2026`,
  etc.).

Examples:

```bash
./bin/ast_program eval tree.txt
./bin/ast_program eval tree.txt vars.txt # If the tree contains variables.
```

## AST file format (reading + writing)

ASTs are written and read as a space-separated preorder token stream:

- A number leaf: a signed decimal `int64_t` (example: `-7`)
- A variable leaf: a lowercase ASCII name (example: `x`, `value`)
- An operator node: one of these operators: `+ - * /`, followed by a left
  subtree then a right subtree.

Examples:

- `2+3*4` gives `+ 2 * 3 4`
- `5 + (x * (-7)) + y` gives `+ + 5 * x -7 y`

## Implemented extra features

- Whitespace-insensitive parsing.
- Modern C++ literal tree (`std::unique_ptr`, no manual `new`/`delete`).
- Variables + optional variable bindings file in eval mode.
- Extra operations: binary minus, unary minus, binary division.
- Efficient parsing/evaluation for very large inputs using linear-time
  tokenizing + "shunting-yard algorithm"-style tree construction.
- Error handling for common cases, like bad files, syntax errors, malformed
  preorder, missing/duplicate variable assignments, missing variable values,
  division by zero, and integer overflow.

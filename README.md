# CPP_PA3

## TODO for this README
- [ ] Explain how to compile.
- [ ] Explain our AST format clearly, how it is read and written.
  - [ ] Mention that tokens are space-separated.
  - [ ] Mention that operators are either `+`, `-`, or `*`
  - [ ] Mention that numbers are decimal `int64_t`
  - [ ] Provide an example input and output, e.g. `2+3*4` becomes `+ 2 * 3 4`
- [ ] Mention extra features we implemented
  - [ ] Mention that it's whitespace insensitive (we skip `isspace`)
  - [ ] Mention that the tree is modern C++, since we use `unique_ptr` instead
    of `new`/`delete`.
  - [ ] 

## Summary

This project is an expression parser and evaluator using an Abstract Syntax Tree(AST). It generates an AST file from input equation and, in a seperate run, evaluates a solution for it.

## OS assumptions

linux is recommended
compiler version g++ with C++20 is recommended
no external libraries

## How to run

Compile:
```bash
make
```

Create a file called `input.txt` and write an equation to be parsed into a tree.

Build tree: 
```bash
./bin/ast_program build tree.txt input.txt
```

Evaluate solution for syntax tree: 
```bash
./bin/ast_program eval tree.txt
```

Evaluate solution for syntax tree if you had any variables: 
```bash
./bin/ast_program eval tree.txt variables.txt
```

## Input formats

input.txt contains an equation consisting of integers and the operators: `+, -, *, /`, whitespace is ignored and parentheses allowed. additionally strings are considered variables in this equation.

example: 
```bash
5 + (x * (-7)) + y
```

variables.txt allows defining variables to be used in evaluation of input.txt. each variable is to be defined in its own line.

example: 
```bash
x = 10
y = 5
```

variable names are lowercase ASCII letters, values are signed 64 bit integers

## AST storage format

tree.txt should store the tree in a recursive preorder format. Numbers and variables are literal tokens. Operators are `+ - * /`.

example(for previous example of input):
```bash
+ + 5 * x -7 y
```

## Implemented features

input is whitespace insensitive. 

Our AST is a modern C++ tree, with `unique_ptr`

Variables are implemented

Extra operations implemented, unary/binary minus (`-`) and binary division (`/`)

High speed approach implemented. AST is able to build end evaluate equations of around 1.000.000 characters within a second using single-pass tokenization

## Complexity and performance

Tokenization and tree construction are linear in input size, evaluation uses preorder processing

## Error handling

TODO


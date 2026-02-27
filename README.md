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
`variables.txt` should contain an assignment of the form `x=7` or `variable=9`
or etc., one on each line.
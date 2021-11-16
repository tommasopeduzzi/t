# The t Programming Language
(Don't judge the name, I'm not creative)

Small compiler for my own programming language called t.
This project is mostly based on the official llvm tutorial with some modifications made to it.

Currently known bugs/problems:
- [ ] If there is no terminator instruction in the outermost layer of a function, the JIT will crash
- [ ] Lexer currently doesn't have a way to handle negative numbers and multiple characacter operators

TODO:
- [x] Functions
- [x] Calls
- [x] Mutable Variables
- [x] Scopes
- [x] Control flow:
  - [x] If-Else-Statements
  - [x] For-Loops
  - [x] While-Loops
- [x] Importing of other files
- [ ] Global Variables
- [x] Types
  - [x] Numbers
  - [x] Strings
  - [x] Booleans
  - [ ] (Lists)
- [ ] Standard Library
  - [ ] Call into C-Functions
  - [ ] Math
  - [ ] String
  - [ ] IO
  - [ ] Random
  - [ ] Time
- [ ] Better Errors
- [ ] Documentation
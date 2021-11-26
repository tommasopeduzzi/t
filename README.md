# The t Programming Language
(Don't judge the name, I'm not creative)

Small compiler for my own programming language called t.
This project is mostly based on the official llvm tutorial with some modifications made to it.

Currently known bugs/problems:
- If there is no terminator instruction in the outermost layer of a function, the JIT will crash
- It's currently a bit of a manual a pain in the ass to add new core functions and they have to be specifically declared as external in the code 

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
- [x] Types
  - [x] Numbers
  - [x] Strings
  - [x] Booleans
  - [ ] (Lists)
- [ ] Standard Library
  - [x] Call into C-Functions
  - [ ] Math
  - [ ] String
  - [ ] IO
  - [ ] Random
  - [ ] Time
- [ ] Better Errors
- [ ] Documentation
- [ ] Type checking
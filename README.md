# The t Programming Language

I was curious Because I want the language to be more or less platform-independent and because of the ease-of-use, the compiler uses llvm as its backend.
The [Kaleidoscope Tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/) served as a basis for this project. I added features to it and used my own syntax.
My goal with this language is to be able to teach others how to solve problems instead of writing code, so it should be as simple as possible.

## Documenation
You can find the docs in the [Docs.md](docs/index.md) file.

## Currently known bugs/problems:
- It's currently a bit of a manual a pain in the ass to add new core functions and they have to be specifically declared as external in the code 

## TODO:
- [x] Functions
- [x] Calls
- [x] Scopes
- [x] Control flow:
  - [x] If-Else-Statements
  - [x] For-Loops
  - [x] While-Loops
- Types
  - [x] Numbers
  - [x] Strings
  - [x] Booleans
  - [x] Lists
  - [ ] Variably sized lists
  - [ ] Robust Type Checking
- General Features:
  - [x] Comments
  - [x] Importing of other files
  - [ ] ASM Block
  - [ ] Pointers
  - [x] Structures
- Standard Library
  - [ ] Math
  - [ ] String
  - [ ] IO
  - [ ] Random
  - [ ] Time
- [x] Better Errors
- [x] Documentation
- [x] Type checking (kinda)
# Overview of the ent Programming Language

This document provides an overview of "ent," a programming language inspired by C, but enhanced with modern syntax and improved usability features. We will cover the core concepts, including retained C features as well as the new language additions that improve functionality and developer experience. Below, you will find details on function syntax, data structures, control flow, and other innovative aspects of the language.

## Table of Contents
1. [Core Syntax and Function Definitions](#core-syntax-and-function-definitions)
2. [Header Blocks and Modular Includes](#header-blocks-and-modular-includes)
3. [Data Types and Structures](#data-types-and-structures)
4. [Pointers and Memory Management](#pointers-and-memory-management)
5. [Uniform Function Call Syntax (UFCS)](#uniform-function-call-syntax-ufcs)
6. [Control Flow](#control-flow)
    - [If-Else Statements](#if-else-statements)
    - [Switch Statements](#switch-statements)
    - [For Loops](#for-loops)
7. [Global and Local Variables](#global-and-local-variables)
8. [Extern Keyword](#extern-keyword)

---

## Core Syntax and Function Definitions

In the ent language, functions are defined using a modernized, explicit syntax that borrows ideas from other contemporary programming languages like Rust and Swift. This provides a more readable and expressive way to define functions.

### Function Syntax:
```c
fn <name>(type arg1, type arg2) -> returntype {
    // function body
}
```

### Key Differences from C:
- The `fn` keyword replaces `returntype function_name(type arg)`.
- The return type comes after the parameter list using the `->` syntax, making the signature more readable.
- The function body follows inside curly braces `{}`.

**Example**:
```c
fn add(dword a, dword b) -> dword {
    return a + b;
}
```
This format improves the visual distinction between parameters and the return type, making function signatures easier to comprehend.

## Header Blocks and Modular Includes

Instead of using separate header files (`.h`), each ent source file can contain a `header {}` block, which allows you to define typedefs, structs, and function prototypes. This new approach simplifies management of declarations, providing a cohesive codebase without the need for traditional `.h` files.

### `header {}` Block Example:
```c
header {
    struct Point {
        word x;
        word y;
    };

    fn add_points(Point a, Point b) -> Point;
}

fn add_points(Point a, Point b) -> Point {
    return Point{a.x + b.x, a.y + b.y};
}
```

- The `header {}` block contains definitions such as typedefs, structs, and function prototypes.
- When a file is included using `include "filename.e"` or `include <filename.e>`, only the contents inside `header {}` are imported.

### Benefits:
- No need for separate header files, reducing redundancy.
- Keeps declarations close to their implementation, simplifying maintenance.

## Data Types and Structures

Ent retains the familiar data types from C while expanding on them to provide more flexibility:
- **Primitive Types**: `byte`, `word`, `dword`, `qword` (unsigned by default).
- **Signed Types**: `sbyte`, `sword`, `sdword`, `sqword`.
- **Derived Types**: Pointers, arrays, and user-defined types (via `typedef`).

### Primitive Type Overview:
- **`byte`**: 8-bit unsigned integer.
- **`word`**: 16-bit unsigned integer.
- **`dword`**: 32-bit unsigned integer.
- **`qword`**: 64-bit unsigned integer.
- **Signed versions** are available by prefixing with `s` (e.g., `sword` for a 16-bit signed integer).

### Structures:
Structures (`struct`) in ent allow you to group different data types just like in C:

```c
struct Employee {
    dword id;
    byte name[50];
};

fn print_employee(Employee e) -> void {
    printf("ID: %u, Name: %s", e.id, e.name);
}
```
Structures can be declared inside `header {}` blocks, making them shareable across multiple files.

## Pointers and Memory Management

Pointers in ent work similarly to those in C, providing direct memory access and manipulation, which is critical for low-level programming and efficient data handling.

### Example:
```c
fn main() -> void {
    word a = 10;
    word* p = &a; // Pointer to variable 'a'
    *p = 20;    // Modify the value of 'a' through the pointer
}
```
- `word* p` declares a pointer to a `word` type.
- `&a` returns the address of the variable `a`.
- `*p = 20` changes the value at the address `p` points to.

Pointers are used for managing memory directly, passing arguments by reference, and creating dynamic data structures.

## Uniform Function Call Syntax (UFCS)

Uniform Function Call Syntax (UFCS) in ent allows functions to be called as if they were methods on the first argument. This can make code more intuitive and improve readability, especially for those working with complex data types.

### How UFCS Works:
- Functions that take a struct or object as their first argument can be called directly on the instance.

**Example**:
```c
// Traditional way:
process_data(my_struct);

// With UFCS:
my_struct.process_data();
```
This feature retains procedural semantics but provides a more natural, object-like syntax, enhancing code readability and reducing verbosity.

## Control Flow

Control flow in ent includes familiar constructs like `if-else`, `switch`, and `for` loops, allowing developers to create complex logic.

### If-Else Statements
The `if-else` statement works similarly to C, providing conditional branching:

```c
fn check_value(word x) -> void {
    if (x > 10) {
        printf("x is greater than 10");
    } else if (x == 10) {
        printf("x is equal to 10");
    } else {
        printf("x is less than 10");
    }
}
```
- Conditions are enclosed in parentheses `()`.
- The body of each condition is enclosed in `{}`.

### Switch Statements
The switch statement in ent maintains a familiar C-style control flow:

```c
switch (expression) {
    case (INDX):
        // body
    case (INDX):
        // body
    default:
        // body
}
```
- Cases are written using `case (INDX)`.

### While Loops
The `while` loop in ent is used for iteration and follows a similar syntax to C:

```c
fn print_numbers() -> void {
   word i = 0;
   while (i < 10) {
        printf("%u ", i++);
   }
}
```
- There are no for loops in ent (bloat)
- The loop body is enclosed in `{}`.

## Global and Local Variables

In ent, variables can be declared either globally or locally:
- **Global Variables** are declared outside of functions and are accessible throughout the entire file.
- **Local Variables** are declared within functions and are only accessible within the scope of that function.

**Example**:
```c
dword global_var = 100; // Global variable

fn use_variables() -> void {
    word local_var = 10; // Local variable
    printf("Global: %u, Local: %u", global_var, local_var);
}
```
Global variables help maintain state across function calls, while local variables help encapsulate state within a function.

## Extern Keyword

The `extern` keyword is used in ent to declare variables or functions that are defined in another file, allowing for modular code:

**Example**:
```c
header {
    extern dword shared_value;
}

fn print_shared_value() -> void {
    printf("Shared Value: %u", shared_value);
}
```
The `extern` keyword ensures that the linker knows the variable or function exists elsewhere, enabling cross-file usage.

---

The ent programming language builds on the foundations of C while enhancing syntax and usability, promoting more readable and maintainable code. The addition of features like UFCS, modular `header {}` blocks, improved function syntax, and familiar control flow constructs aims to streamline development without compromising on the power and efficiency that C programmers value.

Naming mangeling; all functions undergo very simple name mangeling, there is no name mangeling for global symbols like variables.
For external function calling, there is name manageled external function using a normal declaration:
```
fn external_name_manageled_function() -> void;
```
And for non-namemangeled functions (commonly called C style) the function declaration can be marked extern"
```
extern fn external_c_style_function() -> void;
```

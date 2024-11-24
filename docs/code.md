# Overview of the ent Language

This documentation provides an overview of the design and features of our new programming language, inspired by C but enhanced with modern syntax and improved usability features. Below, we cover both the original C concepts that remain and the new additions that enhance functionality and developer experience.

## Table of Contents
1. [Function Syntax](#function-syntax)
2. [Uniform Function Call Syntax (UFCS)](#uniform-function-call-syntax-ufcs)
3. [`header {}` Block](#header-block)
4. [Pointers](#pointers)
5. [Data Types and Structures](#data-types-and-structures)
6. [Includes and Header Usage](#includes-and-header-usage)

---

## Function Syntax

The language introduces a modernized, explicit function syntax:

```c
fn <name>(type arg1, type arg2) -> returntype {
    // function body
}
```

### Key Differences from C:
- The `fn` keyword replaces `returntype function_name(type arg)`.
- The return type comes after the parameter list using the `->` syntax, similar to Rust and Swift.
- The function body follows after `{}`.

**Example**:
```c
fn add(int a, int b) -> int {
    return a + b;
}
```
This approach clearly distinguishes between the function parameters and return type, making code more readable.

## Uniform Function Call Syntax (UFCS)

Uniform Function Call Syntax (UFCS) allows you to call a function as if it were a method on the first argument. This can make code much more intuitive, especially for those working with complex data types.

### How UFCS Works:
- Functions that take a struct or object as their first argument can be called directly on the instance.

**Example**:

```c
// Traditional way:
process_data(my_struct);

// With UFCS:
my_struct.process_data();
```
This feature provides a method-like syntax while retaining the procedural style of C. It improves readability and makes it easier to reason about functions operating on a particular data type.

## `header {}` Block

In this language, instead of using separate header files (`.h`), each source file can contain a `header {}` block. This block defines typedefs, structs, and function prototypes, simplifying the management of declarations and keeping the codebase cohesive.

### `header {}` Block Example:
```c
header {
    struct Point {
        int x;
        int y;
    };

    fn add_points(Point a, Point b) -> Point;
}

fn add_points(Point a, Point b) -> Point {
    return Point{a.x + b.x, a.y + b.y};
}
```
- The `header {}` block contains definitions like typedefs, structs, and function prototypes.
- When a file is included using `include "filename.e"` or `include <filename.e>`, only the content inside `header {}` is imported.

### Benefits:
- No need for separate header files.
- Reduces redundancy between `.h` and `.c` files.
- Keeps declarations close to their implementations.

## Pointers

Pointers work just like they do in C, allowing direct memory access and manipulation. This feature is essential for low-level programming and efficient data handling.

### Example:
```c
fn main() -> void {
    int a = 10;
    int* p = &a; // Pointer to variable 'a'
    *p = 20;    // Modifies the value of 'a' through the pointer
}
```
- `int* p` declares a pointer to an integer.
- `&a` gets the address of variable `a`.
- `*p = 20` changes the value at the address `p` points to.

Pointers are fundamental to understanding memory, passing arguments by reference, and working with dynamic data structures.

## Data Types and Structures

The language supports familiar data types structures:
- **Primitive Types**: `byte`, `word`, `dword`, `qword` (all unsigned by default).
- **Signed Types**: `sbyte`, `sword`, `sdword`, `sqword`.
- **Derived Types**: Pointers, arrays, and user-defined types (using `typedef`).

### Primitive Type Overview:
- **`byte`**: 8-bit unsigned integer.
- **`word`**: 16-bit unsigned integer.
- **`dword`**: 32-bit unsigned integer.
- **`qword`**: 64-bit unsigned integer.
- **`sbyte`**: 8-bit signed integer.
- **`sword`**: 16-bit signed integer.
- **`sdword`**: 32-bit signed integer.
- **`sqword`**: 64-bit signed integer.

### Structures:
Structures (`struct`) allow grouping different data types together, similar to C:

```c
struct Employee{
    dword id;
    byte name[50];
};

fn print_employee(Employee e) -> void {
    printf("ID: %u, Name: %s", e.id, e.name);
}
```
Structures can be declared inside the `header {}` block to be shared across multiple files.

## Includes and Header Usage

The language provides an enhanced include system that uses:
- `include "filename.e"` for including files within the project.
- `include <filename.e>` for library files.

When an `include` statement is used, the preprocessor copies only the contents of the `header {}` block from the included file. This avoids common issues with duplicate inclusion and keeps the scope clean.

### Example:
File: `math.e`
```c
header {
    fn add(dword a, dword b) -> dword;
    fn subtract(dword a, dword b) -> dword;
}

fn add(dword a, dword b) -> dword {
    return a + b;
}

fn subtract(dword a, dword b) -> dword {
    return a - b;
}

fn multiply(dword a, dword b) -> dword {
    return a * b; // this will not be visible when file is included
}
```

File: `main.e`
```c
include "math.e"

fn main() -> void {
    dword sum = add(5, 3);
    printf("Sum: %u", sum);
}
```
Only the function prototypes from `header {}` in `math.e` are included in `main.e`, while keeping the implementation separate.

---

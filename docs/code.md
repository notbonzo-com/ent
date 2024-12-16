# Overview of the ent programmming language

This documentation provides an overview of `ent`, a *C-inspired low level programing language*, with several enhancements to both functionality and syntax to fit the needs of a system-level developer (me). The core conceptual similarities to languages like *C* will help developers transition and adapt this language more naturally. `ent` is a whitespace independent language.

## Table of contents
1. [Core Syntax](#core-syntax)
1.1 [Function Definitions](#function-definitions)
    1.2 [Return Statement](#return-statement)
    1.3 [Global Variables](#core-global-variables)
    1.4 [Name Mangeling](#name-mangeling)
    1.5 [Calling Foreign Functions](#foreign-calls)
2. [Preprocessor](#preprocessor)
    2.1 [Core Preprocessor Syntax](#core-preprocessor-syntax)
    2.2 [Defines and Constants](#defines-and-constants)
    2.3 [HB (Header Blocks) and Modular Includes](#header-blocks)
    2.4 [Preprocessor conditions](#preprocessor-conditions)
    2.5 [File Embeding](#file-embeding)
3. [Variables](#variables)
    3.1 [Global Variables](#global-variables)
        3.1.1 [Definition](#global-variable-definitions)
        3.1.2 [Definition with Default Value](#global-variable-definitions-with-initialiser-value)
        3.1.3 [Declaration of External Variable](#external-global-variables)
    3.2 [Local Variables](#local-variables)
    3.4 [Passing Variables into Functions](#passing-variables-into-functions)
    3.5 [Function Return Values](#function-return-values)
4. [Control Flow](#control-flow)
    4.1 [If-Else Statements](#if-else-statements)
    4.2 [Switch Statements](#switch-statements)
    4.4 [Loops](#loops)
        4.4.1 [While Loops](#while-loops)
        4.4.2 [For Loops](#for-loops)
        4.4.3 [Times Loops](#times-loops)
    4.5 [Calling Functions]
5. [Pointers and Memory Management](#pointers)
    5.1 [Declaring Pointers](#declaring-pointers)
    5.2 [Accessing Pointed-to Memory](#accessing-pointer-memory)
    5.3 [Modifying Pointers Destination](#modifying-pointers)
    5.4 [Pointer Arythmetic](#pointer-arythmetic)
    5.5 [Arrays](#arrays)
    5.6 [Type Casts](#type-cast)
6. [Structures](#structures)
    6.1 [Declaration](#declaration)
        6.1.1 [Nested Declarations](#nested-declarations)
    6.2 [Instantiation](#struct-instantiation)
    6.3 [Passing into/from functions](#struct-passing-into-functions)
    6.4 [Pointers to Structs](#pointers-to-structs)
    6.5 [Member Access](#struct-member-access)
    6.6 [Global Structs](#global-structs)
    6.7 [Struct Attributes](#struct-attributes)
7. [Uniform Function Call Syntax](#UFCS)
    7.1 [Core Syntax](#ufcs-core-syntax)
    7.2 [Unwraping the Syntactical Sugar](#unwraping-ufcs)
    7.3 [Chaining Function Calls](#chaining-ufcs-calls)
8. [Typedefs](#typedefs)
    8.1 [Typedefing Base Types](#typedefing-base-types)
    8.2 [Typedefing Pointers](#typedefing-pointers)
    8.3 [Typedefing Structs](#typedefing-structs)

## Core Syntax

In the `ent` language, functions are defined using an explicit syntax which allows for a more readeble and expressive way. Each function definition and declaration starts with the `fn` keyword (with the exception of [foreign function calls](#foreign-calls)) followed by the name of the function. Names of functions, just like all symbols in `ent` are allowed to be alpha-numeric with underscores, but are **not** allowed to start with a number. The symbol name is followed by a set of parenthesis in which function arguments are defined. Arguments follow the `type name` notation and are separated by commas. After the closing parenthesis, there is a mandatory `->` followed by a return type of the function. If the function returns nothing, an explicit `-> void` must be specified. On function declaration, the return type is followed by a semicolon. On function definition it is followed by the body of the function enclosed in block braces.

### Function Definition

    Function declaration
```rust
fn functionName(type1 arg1, type2 arg2) -> returnType;
```

    Function definition
```rust
fn functionName(type1 arg1, type2 arg2) -> returnType {
    // function body
}
```

### Return Statement

In programing, there will be situation where the programmer will want to return a value from a function, or preemptively exit the function. For this the `return` keyword is used. Either with an expression returning its evaluation `return <expression>;` or in functions return void as a standalone statement `return;`
If the compile-time evaluated type of the expression does not match the given return type of a function, a compiler error will be throw. To bypass this, please use a [cast](#type-cast).

### Core Global Variables

Global variables are variables defined outside of a scope of a function. They are statically embeded into the binary (increasing its size) and available either default initialised to some value or to zero. All string literals used, even inside of functions will be evaluated into a global variable and the use of it replaced with a pointer to the string's first byte.
More information about global variables can be read in the section [global variables](#global-variables)

### Name Mangeling

The `ent` language uses a very basic form of name mangeling, designed to balance reducing conflicts and keeping the algorythm easily reproducable and undoable. The following describes the name mangeling process:

1. The original function name will be taken as the base.
2. The total number of arguments will be appended as a number. If none, nothing is appended 
3. For each argument an `__` separated string will be appended, following the lookup table below. If there are none, nothing is appended.
4. The return type symbol is appended after a `___`. If void, neither the argument type nor `___` is appended.

| Base Type Name | Symbol |
| -------------- | ------ |
| byte, sbyte    | b, B   |
| word, sword    | w, W   |
| dword, sdword  | d, D   |
| qword, sqword  | q, Q   |
| struct         | s      |

Pointers and arrays have special handling, where for each layer of a pointer a single `p` is appended before the type, and if type is an array, an `a` followed by the array size is appended before the type.

#### Example Name Mangeling

```rust
fn exampleFunction(dword** a, struct mystruct* b, sqword c, byte d[5]) -> sdword;
```
->

```
exampleFunction__ppd__ps__Q__a5b___D
```

```rust
fn start() -> void;
```
->
```
start
```

### Foreign Calls

As `ent` uses name mangeling, in order to interface with code written in other languages, `ent` supports disabling of name mangling for certain functions, using the `extern` keyword. Use of the `extern` keyword does not disable overloading or UFCS and the programmer is responsible for no conflicts.
As global variables do not use name mangeling, the `extern` keyword has a different effect on them, for more information see [declaration of external variables](#external-global-variables).
Both function declarations and definitions can be prefixed by `extern` to disable name mangeling, for both externally defined functions which code will call into and defining non-name-mangeled functions.

## Preprocessor



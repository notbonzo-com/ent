# Overview of the ent programming language

This documentation provides an overview of `ent`, a *C-inspired low level programing language*, with several enhancements to both functionality and syntax to fit the needs of a system-level developer (me). The core conceptual similarities to languages like *C* will help developers transition and adapt this language more naturally. `ent` is a whitespace independent language.

## Table of contents
1. [Core Syntax](#core-syntax)
    1. [Function Definitions](#function-definition)
    2. [Return Statement](#return-statement)
    3. [Global Variables](#core-global-variables)
    4. [Name Mangeling](#name-mangeling)
    5. [Calling Foreign Functions](#foreign-calls)

2. [Preprocessor](#preprocessor)
    1. [Core Preprocessor Syntax](#core-preprocessor-syntax)
    2. [Defines and Constants](#defines-and-constants)
    3. [HB (Header Blocks) and Modular Includes](#header-blocks)
    4. [Preprocessor Conditions](#preprocessor-conditions)
    5. [File Embeding](#file-embeding)

3. [Variables](#variables)
    1. [Global Variables](#global-variables)
        1. [Definition](#global-variable-definitions)
        2. [Definition with Default Value](#global-variable-definitions-with-initializer-value)
        3. [Declaration of External Variable](#external-global-variables)
    2. [Local Variables](#local-variables)
    3. [Passing Variables into Functions](#passing-variables-into-functions)
        1. [Variadic Functions Vaargs](#variadic-functions-vaargs)
    4. [Function Return Values](#function-return-values)

4. [Control Flow](#control-flow)
    1. [If-Else Statements](#if-else-statements)
    2. [Switch Statements](#switch-statements)
    3. [Loops](#loops)
        1. [While Loops](#while-loops)
        2. [For Loops](#for-loops)
        3. [Times Loops](#times-loops)
    4. [Calling Functions](#calling-functions)

5. [Pointers and Memory Management](#pointers)
    1. [Declaring Pointers](#declaring-pointers)
    2. [Accessing Pointed-to Memory](#accessing-pointer-memory)
    3. [Modifying Pointers Destination](#modifying-pointers)
    4. [Pointer Arythmetic](#pointer-arythmetic)
    5. [Arrays](#arrays)
    6. [Type Casts](#type-cast)

6. [Structures](#structures)
    1. [Declaration](#declaration)
        1. [Nested Declarations](#nested-declarations)
    2. [Instantiation](#instantiation)
    3. [Passing into/from Functions](#passing-intofrom-functions)
    4. [Pointers to Structs](#pointers-to-structs)
    5. [Member Access](#member-access)
    6. [Global Structs](#global-structs)
    7. [Struct Attributes](#struct-attributes)
    8. [Bit Fields](#bit-fields)

7. [Uniform Function Call Syntax (UFCS)](#ufcs)
    1. [Core Syntax](#ufcs-core-syntax)
    2. [Unwraping the Syntactical Sugar](#unwraping-ufcs)
    3. [Chaining Function Calls](#chaining-ufcs-calls)

8. [Typedefs](#typedefs)
    1. [Typedefing Base Types](#typedefing-base-types)
    2. [Typedefing Pointers](#typedefing-pointers)
    3. [Typedefing Structs](#typedefing-structs)

9. [Unions](#unions)

10. [Enums](#enums)
    1. [Declaring Enums](#declaring-enums)

11. [Attributes](#attributes)

12. [Inline Assembly](#inline-assembly)

13. [sizeof and alignof](#sizeof-and-alignof)


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

Global variables are variables defined outside of a scope of a function. They are statically embeded into the binary (increasing its size) and available either default initialized to some value or to zero. All string literals used, even inside of functions will be evaluated into a global variable and the use of it replaced with a pointer to the string's first byte.
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
| single, double | f, F   |
| struct         | s      |
| vaargs         | v      |

Pointers and arrays have special handling, where for each layer of a pointer a single `p` is appended before the type, and if type is an array, an `a` followed by the array size is appended before the type.

#### Example Name Mangeling

```rust
fn example_function(dword** a, struct mystruct* b, sqword c, byte d[5]) -> sdword;
```
->

```
example_function__ppd__ps__Q__a5b___D
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

The `ent` language provides a preprocessor phase similar to C, but with a more streamlined syntax. Preprocessor directives allow you to define constants, conditionally compile code, include external files, and embed binary data directly into the compiled binary.
Another important syntax difference is that in `ent` preprocessor directives do not begin with a `#` like in C.
Preprocessor directives, if placed into the header block will be preprocessor copied and evaluated upon inclusion.

### Core Preprocessor syntax

Preprocessor directive can only be used in top-level scope, with the exception of those prefixed with an @, which can be used anywhere. The preprocessor directive takes up the entire line and no code is allowed to follow it (comments may).
In `ent`, the main preprocessor directives are as follows:

- `include <filename>`: includes the header block of the specified file, relative to the root of the project or libraries.
- `include "filename"`: includes the header block of the specified file, relative to the current file's location.
- `define`: a compile time constant which can be assigned a name and a value. They can be used in preprocessor conditional statements or copy pasted by the preprocessor if used in code.
- conditional statements prefixed with a `@`
    - `@if condition`: evaluates the condition on compiletime, using defined preprocessor constants and comparison operators: == < > <= >= !=
    - `@ifdef`: passes true if the given symbol is a valid preprocessor constant, false if not.
    - `@elif condition` evaluates the condition on compiletime, only if the if condition above failed. Has to be placed logicially under a if or elif statement.
    - `@else`: only if the if right above failed, the else will be enabled.
    - `@endif`: ends the conditional block above.
- `embed "filename" variablename`: embeds the given file into .rodata section and creates a global variable of type `byte**` which is default initialized to point to the start of the embeded data.

### Defines and Constants

The `define` preprocessor directive allowes the programmer to create symbolic constants that the preprocessor replaces at compile time. This is usefull for values that may vary by platform or environment, and for setting feature flags or toggling functionality without altering runtime behaviour. They can also be evaluated on compile time using preprocessor conditional statement allowing for more fine grained control.
```rust
define DEBUG_MODE 1 
define PI 3.14159

fn circle_area(dword radius) -> double {
@ifdef DEBUG_MODE
    radius.print();
@endif
    double r = (double)radius;
    return PI * r * r;
}
```

### Header Blocks

In `ent`, the separation of header files and translation units. Instead, each translation unit can define a header block, which defines what the translation unit exposes. This is then preprocessor copied upon being `include`ed by the preprocessor from another translation unit.
The header block contains declarations, such as structs, function declarations, typedefs and external global variables that can be shared across files.
The header block, unlike preprocessor directives follows a whitespace independent scheme where symbols are use to specify its beggining and end.


```rust
// config.e
header { // code can be placed here
    typedef dword config_t;
    struct config {
        config_t settings_a,
        config_t settings_b
    };
    fn load_config() -> struct config;
    extern struct config g_config;
/* code can be placed here */ }

// main.e
include "config.e"

fn main() -> dword {
    struct config cfg = load_config;
    g_config->settings_a = cfg->settings_a;
    config_t b = cfg->settings_b;
}
```

In this example, the `config.e` file contains a `header {}` block (exposing `struct config`, `config_t`, `load_config` and `g_config`) and the actually function and variable definition. When `include "config.e"` is processed, only the `header {}` block is imported, ensuring the main file knows about all the given declarations.

### Preprocessor Conditions

Similar to C, `ent`'s preprocessor allows conditional compilation using the `@if`, `@elif`, `@else`, `@endif` and `@ifdef` preprocessor directives. These directives evaluate compile timed defined constants and when the conditions are met the enclosed code is compiled; otherwise it is skipped.

```rust
@ifdef DEBUG_MODE
fn debug_log(byte* msg) -> void {
    printf("[DEBUG]: %s\n", msg);
}
@else
fn debug_log(byte* msg) -> void {
    // Release mode: do nothing or log to file
}
@endif
```

This mechanism allows building the same codebase in multiple configurations (e.g., debug vs. release or architecture specific) without manually editing code segments.

### File Embeding 

The `embed` directive allows programmers to simply embed resources such as but not limited to; images, textures, audio, dynamically loaded modules or other binary files. 
When a valid `embed` directive is being proecessed, the preprocessor reads the binary files and places it into the .rodata section of the binary output. Then, a variable of type `byte**` and name specified as the third argument of the `embed` directive is created and default initialized with the address of the binary file's start in the .rodata section.
The second argument of the `embed` directive is a `"` enclose filename of the resource, **relative to the project root**.

```rust
embed "data.bin" data_array // this creates a `byte** data_array;`

fn process_data() -> void {
    printf("%s\n" *data_array); // print the data as if it was a string
}
```

## Variables 

Variables in `ent` hold data that can be manipulated during program execution. There are several types of variables based on scope and lifecycle: global variables, local variables, and external variables.

### Global Variables

Global variables are declared outside any function, making them accessible from any part of the program after their declaration. They are also visible as link-time symbols so they can be used externally.

#### Global Variable Definitions 

To define a global variable, you specify the type followed by the variable name outside of any function body.

```
dword global_variable;
dword* global_variable_pointer;
byte global_variable_array[20];
struct mystruct global_variable_struct;
```

#### Global Variable Definitions with Initializer Value

Global variables can be initialized with a value at the time of definition. This value must be a constant expression since it is evaluated at compile time and statically embeded into the binary, available to use immediately on program start.

```
dword global_variable = 278;
byte* global_string = "Hello World\n";
byte global_variable_array[5] = {0, 1, 2, 3 , 4};
struct mystruct global_variable_struct = {.a = 2, .b = 892934, .c = "Hello World\n"};
```

#### External Global variables

The `extern` keyword is used to declare a global variable that is defined in another file or module. This is especially useful header blocks to allow other translation units to access global variables defined in one, or to interface with foreign global variables.

```
extern dword external_global_variable;
extern byte* external_string;
extern byte externally_defined_array[8];
extern struct mystruct externally_defined_global_struct;
```

### Local Variables 

Local variables are declared within a function and can only be accessed within that function's scope. They are created when the function is called and destroyed when it returns. Local variables are also scoped inside of other blocks, such as the body of `if/else`, `switch` and loops.

```rust
fn example_function() -> void {
    dword localVar = 5;
    // code that uses localVar
}
```

#### Passing variables into Functions

When passing variables into functions, `ent` uses a pass-by-value by default, meaning each function gets a copy of each of the arguments. If the function needs to modify the original, it can explicitly take a pointer to a variable. More on [pointers](#pointers).

```rust
fn pass_by_value(dword a) -> dword {
    a += 10; // original a value does not change
    return a;
}
```
```rust
fn pass_by_reference(dword* a) -> void {
    *a += 10; // original value does change
}
```

##### Variadic Functions (Vaargs)

While C uses a manual mechanism (`va_list`, `va_start`, `va_arg`, etc.) for handling variable argument lists, `ent` provides a **more streamlined** approach via a built-in `varargs` type. This design aims to be safer and more intuitive than the classic C solution.

###### Declaring a Variadic Function

To declare a variadic function, you replace the final parameter(s) with a `varargs` parameter. For instance:

```rust
fn my_printf(byte* format, varargs args) -> dword {
    // function body
}
```

###### Accessing Variadic Arguments

Within the function, you can retrieve arguments from `args` through built-in methods that handle stepping through the parameters. Conceptually, the API looks like:

```rust
// this API is defined in the freestanding standart library
fn next_byte(varargs* args) -> byte;
fn next_word(varargs* args) -> word;
fn next_dword(varargs* args) -> dword;
fn next_qword(varargs* args) -> qword;
fn next_sbyte(varargs* args) -> sbyte;
fn next_sword(varargs* args) -> sword;
fn next_sdword(varargs* args) -> sdword;
fn next_sqword(varargs* args) -> sqword;
fn next_single(varargs* args) -> single;
fn next_double(varargs* args) -> double;
```

As you can see, pointers and structs are not natively suppored, and should be passed as the corresponding type. (on 32 bit architectures, pointers should be passed as `dword` and on 64 bit as `qword`)

Under the hood, the `varargs` mechanism manages pointer offsets and type retrieval This design helps prevent common mistakes in classic C’s variadic functions and centralizes argument management through a standard, typed interface,
while still providing freestanding flexibility.

> **Note**: The final specifics belong to the freestanding standard library for ent, which is platform-agnostic. The language design ensures easy access to typed parameters.

#### Function Return Values 

Functions in `ent` can return values. If a function is supposed to return a value, this must be explicitly mentioned in the function signature with a `->` followed by the type of the return value instead of `void`. The return value can either by used by caller or discarded.

```rust
fn increment(dword value) -> dword {
    return value + 1;
}
```

## Control Flow

`ent` supports various control flow constructs that allow for dynamic reactions to events that occur in the program.

### If-Else Statements 

`If-Else` statements are used to execute code conditionally. Blocks are defined using braces.

```rust
fn check_value(dword value) -> void {
    if (value > 10) {
        printf("Value is greater than 10.\n");
    } else {
        printf("Value is 10 or less.\n");
    }
}
```

### Switch Statements 

`Switch` statements provide a method of executing code based on the value of an expression.

```rust
fn evaluate(dword value) -> void {
    switch (value) {
        case (1) {
            printf("One\n");
        }
        case (2) {
            printf("Two\n");
        }
        case (3, 4, 5, 6) {
            printf("3 <= value <= 6\n");
        }
        default {
            printf("Other\n");
        }
    }
}
```

### Loops

`ent` supports `while`, `for` and `times` loops for repeated and conditionally repeated code execution.

#### While Loops

Executes the block as long as the condition is true.

```rust
fn count_to_ten() -> void {
    dword count = 1;
    while (count <= 10) {
        printf("%d ", count);
        count++;
    }
}
```

#### For Loops 

Used for more advanced looping with fine-grained control over the initializer, condition and step. Like in C they are made out of three main parts, the initializer, the condition and the step.

```rust
fn count_to_ten() -> void {
    for (dword i = 1; i <= 10; i++) {
        printf("%d ", i);
    }
}
```

#### Times Loops 

A simplified loop construct unique to `ent`, executing a block a specific number of times, with an optionally exposed index. The basic syntax without an index is:

```rust
fn print_ten_times() -> void {
    times (10) {
        printf("ding!\n");
    }
}
```

And with an exposed index, the index vairbale name and type is user specified, as long as the type is integral (not a pointer, and not a struct)

```rust
fn count_to_ten() -> void {
    times (10 : dword number) {
        printf("%d ", number);
    }
}
```

> Note the difference of this example from the while and for example, where using the `times` loop always starts with index 1 and end on N inclusively.

### Calling Function

Another important change of control flow can be achived by calling functions. This can be done in two main ways; classical calls and [UFCS](#UFCS)
A classical function call can be achived using the syntax `functionName(argument1, argument2 ...)` and can be used either as a standalone expression discarding its return value (if not void) or as a part of an expression where its return value can be operated on and used as arguments for more functions.
When a function call is starting to be evaluated, both standalone and inside an expression, cpu control immediately jumps to the function passing arguments in the way specified in [passing variables to functions](#passing-variables-into-functions).

## Pointers

Pointers in `ent` are variables that hold the address of another variable or memory location. They allow for powerful and flexible memory manipulation but must be used with care to avoid memory corruption or undefined behavior.

### Declaring Pointers

Pointers are declared by specifying the base type of the data they will point to, followed by an asterisk (`*`). The declaration syntax is as follows:

```rust
dword* ptr; // Pointer to a dword
byte** ptr_to_ptr; // Pointer to a pointer to a byte
```

To assign a pointer, you can use the address-of operator (`&`) to obtain the address of a variable:

```rust
dword value = 42;
dword* ptr = &value; // ptr now points to value
```

### Accessing Pointed-to Memory

The dereference operator (`*`) is used to access or modify the value stored at the address a pointer holds:

```rust
fn example() -> void {
    dword value = 42;
    dword* ptr = &value;
    *ptr = 100; // Modifies the value at the address pointed to by ptr
    printf("%d", value); // Outputs: 100
}
```

### Modifying Pointer Destinations

You can change the address stored in a pointer to point to a different memory location:

```rust
fn swap_pointers(dword** ptr1, dword** ptr2) -> void {
    dword* temp = *ptr1;
    *ptr1 = *ptr2;
    *ptr2 = temp;
}
```

Pointers to arrays, structs, or other types follow the same principles but may require additional syntax for accessing members or elements.

### Pointer Arithmetic

Pointer arithmetic allows you to perform operations like addition or subtraction directly on pointers. These operations account for the size of the data type the pointer refers to:

```rust
fn iterate_array(dword* array, dword size) -> void {
    for (dword i = 0; i < size; i++) {
        printf("%d ", *(array + i));
    }
}
```

### Arrays

In `ent`, arrays are contiguous blocks of memory, and array names are implicitly converted to pointers to their first element when passed to functions.

```rust
fn example_array() -> void {
    dword array[5] = {1, 2, 3, 4, 5};
    dword* ptr = array; // array is implicitly converted to a pointer
    printf("%d", *(ptr + 2)); // Access the third element: Outputs 3
    printf("%d", array[2]); // Access the third element: Outputs 3
}
```

### Type Casts

Type casting allows you to treat a pointer as if it points to a different type. Use casts when required but ensure compatibility to avoid undefined behavior:

```rust
fn cast_example(byte* raw_data) -> dword {
    return *((dword*)raw_data); // Treat raw_data as a pointer to dword
}
```

They can also be used on primitive non-pointer types:

```rust
fn cast_example2(dword value) -> byte {
    return (byte)value; // Treat value as a byte, will be truncated
}
fn cast_example3(byte value) -> dword {
    return (dword)value; // Treat value as a dword, will be zero-extended
}
```

## Structures

Structures in `ent` are user-defined composite types that group multiple variables under a single type. They are useful for modeling entities with multiple attributes.

### Declaration

Structures are declared using the struct keyword, followed by the structure name and a block that defines its members. Members can be of any type, including other structs.

```c
struct mystruct {
    dword a,
    qword b,
    byte* c
};
```

#### Nested Declarations

Structures can contain other structures as members, allowing hierarchical organization of data.

```rust
struct point {
    dword x,
    dword y
};

struct rectangle {
    struct point top_left,
    struct point bottom_right
};
```

Or they can be even directly defined inside of existing structures

```rust
struct rectangle {
    struct { dword x, dword y } top_left,
    struct { dword x, dword y } bottom_right
};
```

### Instantiation

To create an instance of a struct, declare a variable of the struct type. Initialization can be done manually for each member or using designated initializers.

```rust
struct mystruct instance = {1, 9223372036854775808, "Hello"};
struct rectangle rect = {
    .top_left = {x = 0, y = 0},
    .bottom_right = { x = 10, y = 5}
};
```

### Passing into/from Functions

Structures can be passed to functions either by value or by reference. Passing by value copies the structure, while passing by reference allows direct modification.
When passing by value, the entire structure is pushed onto the stack and is accessed from there, with possible overflows corrupting the stack.

```rust
fn move_rectangle(struct rectangle* rect, dword dx, dword dy) -> void {
    rect->top_left->x += dx;
    rect->top_left->y += dy;
    rect->bottom_right->x += dx;
    rect->bottom_right->y += dy;
}
```

### Pointers to Structs

You can create pointers to structs just like any other type. Use the `->` operator to access members through a pointer, just like with structs themselves. The compiler will ensure that the correct operation is done based on the type.

```rust
fn print_rectangle(struct rectangle* rect) -> void {
    printf("Top-left: (%d, %d)\n", rect->top_left->x, rect->top_left->y);
    printf("Bottom-right: (%d, %d)\n", rect->bottom_right->x, rect->bottom_right->y);
}
```

### Member Access

Access members of a struct using the `->` operator for both direct instances and pointers.

```rust
rect->top_left->x = 1; // Direct instance
rect_pointer->top_left->x = 1; // Through pointer
```

### Global Structs

Struct instances can also be defined as global variables, making them accessible throughout the program.

```rust
struct rectangle screen_bounds = {
    .top_left = {.x = 0, .y = 0},
    .bottom_right = {.x = 1920, .y = 1080}
};
```

### Struct Attributes

Attributes can be used to provide additional information about struct members for optimization or special behavior.

// TODO

### Bit Fields

Bit fields in `ent` allow you to pack multiple integer sub-fields into a single underlying integer type. This is particularly useful for manipulating registers, protocol headers, or other low-level data where precise control over bit layout is required.

#### Declaring Bit Fields

Bit fields must be declared inside a struct. Each field is followed by a colon (`:`) and the number of bits it occupies. The underlying base type (e.g., `dword`, `qword`) dictates how the fields are stored in memory.

```rust
struct status_register {
    dword carry : 1,
    dword overflow : 1,
    dword direction : 1,
    dword interrupt : 1,
    dword reserved : 12,
    dword data : 16
};
```

#### Memory Alignment and [packed]

By default, bit fields follow the same alignment rules as other struct members. You can override alignment by applying struct attributes such as `[packed]` or `[aligned(N)]`. Marking the struct as `[packed]` removes extra padding between members:

> **Note**: Be mindful of endianness—bit field layouts can vary across architectures. For cross-platform data, consider masks and shifts over bit fields.

## UFCS

Uniform Function Call Syntax (UFCS) allows invoking functions as if they were methods on their first argument. This enhances readability and supports chaining.

### UFCS Core Syntax

A function call can be made using either classical or UFCS syntax, and they are interchangeable.

```rust
fn classical_call(dword a) -> void {
    a.print(); // UFCS
    print(a);  // Classical
}
```

### Unwraping UFCS

UFCS is essentially syntactical sugar where the first argument becomes the "caller."

```rust
a.print(); // UFCS
print(a);  // Equivalent classical call
```

This step is done inside the AST Parser so for the code-gen these two notations are equivalent.

### Chaining UFCS Calls

UFCS allows chaining multiple calls for cleaner code.

```rust
fn chain_example(dword a) -> void {
    a.increment().double().print(); // Chained UFCS
    print(double(increment(a))); // Chained classical
}
```

## Typedefs

The `typedef` keyword in `ent` provides a way to define new type aliases, making code easier to read and maintain. It is particularly useful for creating shorthand names for complex types or improving code clarity.

### Typedefing Base Types

You can create an alias for any existing base type using `typedef`. This is often used to enhance semantic meaning or simplify repetitive type declarations.

```rust
typedef dword size_t;
typedef byte char_t;

fn example() -> void {
    size_t length = 42;
    char_t first_letter = 'A';
}
```

In this example, `size_t` and `char_t` act as semantic aliases, even though they resolve to `dword` and `byte`, respectively.

### Typedefing Pointers

Pointer typedefs simplify declarations of frequently used pointer types, especially when working with complex data structures.

```rust
typedef dword* dword_ptr;
typedef byte** string_ptr;

fn example(dword_ptr ptr, string_ptr data) -> void {
    *ptr = 10;
    printf("%s", *data);
}
```
Using pointer typedefs reduces visual clutter in function signatures and makes the purpose of each pointer clearer.

### Typedefing Structs

You can use `typedef` with structs to create shorthand names for complex or frequently used struct types. This eliminates the need to repeatedly write `struct` in declarations.

```rust
struct point {
    dword x,
    dword y
};

typedef struct point Point;

fn move_point(Point* p, dword dx, dword dy) -> void {
    p->x += dx;
    p->y += dy;
}

fn main() -> void {
    Point p = {10, 20};
    move_point(&p, 5, 5);
    printf("Point: (%d, %d)\n", p->x, p->y);
}
```

## Unions

A `union` in `ent` is a type similar to a struct, except all union members share the same memory location. Only one member is valid at a time, but this allows multiple “views” of the same underlying data.

### Declaring and Using Unions

```rust
union number_value {
    dword integer_val,
    single float_val,
    byte bytes[4]
};

fn show_union_values(union number_value val) -> void {
    printf("As integer: %d\n", val.integer_val);
    printf("As float: %f\n", val.float_val);
    printf("As bytes: %02x %02x %02x %02x\n",
        val.bytes[0], val.bytes[1], val.bytes[2], val.bytes[3]);
}
```

- **Size**: The union’s size is that of its largest member.
- **Initialization**: You can initialize by designating a specific member:

```rust
union number_value nv = {.integer_val = 42};
```
- **Overwriting**: Storing a new value in one member overwrites the data for all others.

## Enums

Enums group together named integer constants under a single type, improving code readability over a series of unrelated defines or constants.

### Declaring Enums

```rust
enum color {
    COLOR_RED = 0,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW
};
```

If no explicit initializer is given, enum values default to ascending integers starting at `0`. Internally, `ent` typically stores enums as `dword` unless negative values or other constraints imply otherwise.

## Attributes

`ent` offers a unified attribute syntax that can be applied to functions, variables, parameters, and more. Attributes annotate the compiler with metadata or instructions (e.g. inline, deprecated, etc.).

| Attribute       | Description                                                                 |
|------------------|-----------------------------------------------------------------------------|
| `null`          | No attribute specified.                                                    |
| `const`         | Specifies that a variable or object is constant and cannot be modified.    |
| `volatile`      | Indicates that a variable can be changed unexpectedly, often by hardware.  |
| `static`        | Declares a static variable or function, with scope limited to its context. |
| `inline`        | Suggests that the compiler should inline the function for optimization.    |
| `packed`        | Ensures the structure has minimal memory alignment, without padding.       |
| `aligned(N)`    | Specifies a particular memory alignment for variables or structures.       |
| `noreturn`      | Declares that a function does not return to the caller.                    |
| `deprecated("message")` | Marks a function or variable as deprecated, with an optional message.|

You can place attributes:
- After a function’s return type
- After a variable name
- After a struct declaration keyword
- After a function parameter

The syntax of attributes is comma separated in `[` brackets.

## Inline Assembly

For scenarios requiring direct hardware or instruction-level access, `ent` supports inline assembly blocks. The exact syntax can vary by target architecture and compiler toolchain; a conceptual approach might look like:

```rust
fn do_some_asm(dword value) -> dword {
    asm {
        mov eax, [value]
        add eax, 5
        // ...
    }
    return value;
}
```

Inline assembly allows you to perform operations not directly expressible in high-level code.
By default, the compiler performs an expencive save of the register state before each block, which can be disabled using the `[nosave]` attribute in the block.

```rust
asm [nosave] {
        push eax // manually save the register to prevent undefined behaviour
        mov eax, [value]
        add eax, 5
        pop eax // restore the register
    }
```

## sizeof and alignof

Like in C, `ent` provides `sizeof` and `alignof` operators to query the size and alignment requirements of types or expressions.

- `sizeof`
Yields the size in bytes of a type or variable. Evaluated at compile time:

```rust
dword size_int = sizeof(dword); // Typically 4 on many architectures
dword size_struct = sizeof(struct gdt_descriptor);
```

- `alignof`
Yields the alignment (in bytes) required by a type. Also evaluated at compile time:

```rust
dword align_int = alignof(dword);
dword align_struct = alignof(struct gdt_descriptor);
```

Attributes such as `[packed]` or `[aligned(N)]` can influence these values.


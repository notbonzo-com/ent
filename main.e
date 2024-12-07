header {
    //struct Point {
    //    word x;
    //    word y;
    //};

    //struct Employee {
    //    dword id;
    //    byte name[50];
    //};

    extern dword shared_value;

    fn add(word a, word b) -> word;
    //fn create_point(word x, word y) -> Point;
    //fn add_points(Point a, Point b) -> Point;
    //fn print_employee(Employee e) -> void;
    fn calculate_sum(word* arr, word size) -> word;
}

dword shared_value = 42;

fn add(word a, word b) -> word {
    return a + b;
};

//fn create_point(word x, word y) -> Point {
//    return Point{x, y};
//}

//fn add_points(Point a, Point b) -> Point {
//    return Point{a.x + b.x, a.y + b.y};
//}

//fn print_employee(Employee e) -> void {
//    printf("Employee ID: %u, Name: %s\n", e.id, e.name);
//}

fn calculate_sum(word* arr, word size) -> word {
    word sum = 0;
    while (size > 0) {
        sum = sum.add(*arr); // UFCS: arr points to the current element
        arr++;
        size--;
    }
    return sum;
};

fn main() -> void {
    // Test global and local variables
    dword local_var = 10;
    printf("Global Variable: %u, Local Variable: %u\n", shared_value, local_var);

    // Test primitive data types and addition
    word a = 15;
    word b = 20;
    word result = add(a, b);
    printf("Result of add(%u, %u): %u\n", a, b, result);

    // Test structures and UFCS
    //Point p1 = create_point(5, 10);
    //Point p2 = create_point(15, 20);
    //Point p3 = p1.add_points(p2); // UFCS
    //printf("Point Addition: (%u, %u) + (%u, %u) = (%u, %u)\n", p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);

    // Test arrays, pointers, and dynamic memory
    // word arr[5] = {1, 2, 3, 4, 5};
    //word sum = calculate_sum(&arr[0], 5);
    //printf("Sum of array elements: %u\n", sum);

    // Test structs and printing
    //Employee emp = Employee{101, "Alice"};
    //emp.print_employee(); // UFCS

    // Test control flow
    word x = 15;
    if (x > 10) {
        printf("x is greater than 10\n");
    } else if (x == 10) {
        printf("x is equal to 10\n");
    } else {
        printf("x is less than 10\n");
    }

    switch (x) {
        case (5):
            printf("x is 5\n");
        case (10):
            printf("x is 10\n");
        default:
            printf("x is something else\n");
    }
};

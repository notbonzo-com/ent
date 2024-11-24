header {
    fn add(dword a, dword b) -> dword;
    fn increment(dword* a) -> void;
    include <float.e>
}

fn add(dword a, dword b) -> dword {
    return a + b;
}

fn increment(dword* a) -> void {
    *a++;
}
header {
    include "math.e"
}

fn callMe() -> void {
    while (1 == 1) {
        dword baab = 82;
        byte* str = "Hello World\n";
        dword cak = 82 * add(78, 2);
    }
}

fn main(dword argc, byte** argv) -> dword {
    dword a = 289;
    a = a.add(78);
    // Now `a` is a lot
    /*

    Funny

    */
    if (a == 289 || a != 78) {
        switch (a) {
            case 0x289: continue;
            case 0x290: break;
            case 0x300: callMe();
        }
    }

    return 0;
}
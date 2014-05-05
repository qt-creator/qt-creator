struct xyz {
    int a;
    int b;
    int c;
} klm = { .a = 99, .c = 100 };
struct a {
    struct b {
        int c;
        int d;
    } e;
    float f;
} g = {.e.c = 3 };

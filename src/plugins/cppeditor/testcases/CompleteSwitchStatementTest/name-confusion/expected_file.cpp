enum test { TEST_1, TEST_2 };

void f() {
    enum test test;
    switch (test) {
    case TEST_1:
        break;
    case TEST_2:
        break;
    }
}

enum class test { TEST_1, TEST_2 };

void f() {
    enum test test;
    switch (test) {
    case test::TEST_1:
        break;
    case test::TEST_2:
        break;
    }
}

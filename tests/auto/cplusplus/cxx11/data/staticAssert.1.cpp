struct S {
    static_assert(sizeof(char) == 1, "Some message");

    void f() {
        static_assert(sizeof(int) == 4, "Another message");
    }
};

static_assert(sizeof(char) == 1, "One more");

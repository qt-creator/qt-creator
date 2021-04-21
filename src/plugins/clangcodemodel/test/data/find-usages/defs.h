struct S {
    S() : value2(value) {}
    static int value;
    int value2 : 2;
    static const int value3 = 0;
    static void *p;
    static const void *p2;
    struct Nested {
        int constFunc() const;
        void constFunc(int) const;
        void nonConstFunc();
    } n;
    Nested constFunc() const;
    void nonConstFunc();
    static void staticFunc1() {}
    static void staticFunc2();
    virtual void pureVirtual() = 0;
};

struct S2 : public S {
    void pureVirtual() override {}
};

void func1(int &);
void func2(const int &);
void func3(int *);
void func4(const int *);
void func5(int);

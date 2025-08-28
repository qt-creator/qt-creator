class Foo {
    void f1();
    inline int f2@() const
    {
        return 1;
    }
    void f3();
    void f4();
};

void Foo::f4() {}

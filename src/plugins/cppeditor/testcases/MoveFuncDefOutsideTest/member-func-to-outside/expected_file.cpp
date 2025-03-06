class Foo {
    void f1();
    int f2() const;
    void f3();
    void f4();
};

int Foo::f2() const
{
    return 1;
}

void Foo::f4() {}

class Foo {
    int f1();
    int f2() const;
};

int Foo::f1()
{
    return 1;
}

int Foo::f2() const
{
    return 2;
}

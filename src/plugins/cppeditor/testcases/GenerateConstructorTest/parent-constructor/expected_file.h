struct Bar{
    Bar(int i);
};
class Foo : public Bar{
    int test;
public:
    Foo(int test, int i) : Bar(i),
        test(test)
    {}
};

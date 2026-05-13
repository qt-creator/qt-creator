struct Bar{
    Bar(int use_i = 6);
};
class Foo : public Bar{
    int test;
public:
    Foo(int test, int use_i = 6) : Bar(use_i),
        test(test)
    {}
};

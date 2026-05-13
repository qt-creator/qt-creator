class Foo{
    CustomType test;
public:
    Foo(CustomType test) : test(std::move(test))
    {}
};

struct Bar{
    Bar(int use_i = L'A', int use_i2 = u8"B");
};
class Foo : public Bar{
public:
    Foo(int use_i = L'A', int use_i2 = u8"B") : Bar(use_i, use_i2)
    {}
};

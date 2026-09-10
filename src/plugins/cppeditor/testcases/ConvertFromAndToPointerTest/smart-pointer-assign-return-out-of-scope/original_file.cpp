class Foo
{
public:
    Foo(int) {}
};

std::unique_ptr<Foo> func()
{
    std::unique_ptr<Foo> @f(new Foo(0));
    return f = std::make_unique<Foo>(1);
}

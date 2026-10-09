class Foo
{
public:
    void bar();
};

void func()
{
    std::unique_ptr<Foo> other(new Foo);
    std::unique_ptr<Foo> @f(new Foo);
    f->bar();
    f = std::move(other);
}

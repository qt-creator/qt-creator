class Foo
{
public:
    void bar();
};

void func()
{
    std::unique_ptr<Foo> other(new Foo);
    std::unique_ptr<Foo> @f = std::move(other);
    f->bar();
}
